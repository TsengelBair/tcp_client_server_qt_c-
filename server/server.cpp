#include "server.h"
#include "packethandler.h"
#include "packetbuilder.h"
#include "structs.h"
#include "serializer.h"
#include "postgresdb.h"

#include <QTcpSocket>
#include <QDataStream>

Server::Server(const QHostAddress &address, unsigned short port, IDbHandler &db) : m_db(db)
{
    runServer(address, port);
}

void Server::runServer(const QHostAddress &address, unsigned short port)
{
    if (!this->listen(address, port)) {
        qDebug() << "Error while starting server: " << errorString();
    }
    else {
        qDebug() << "Listen";
    }
}

Server::~Server()
{
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    m_sockets.insert(socketDescriptor, socket);
    qDebug() << "Client connected: " << socketDescriptor;

    connect(socket, &QTcpSocket::readyRead, this, &Server::slotHandleDataFromClient);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void Server::slotHandleDataFromClient()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        qDebug() << "Error while cast to socket*";
        return;
    }

    if (!isConnected(socket->socketDescriptor())) return;

    QByteArray newData = socket->readAll();
    if (newData.isEmpty()) return;

    m_buffers[socket->socketDescriptor()] += newData;
    slotProccesBuffer(socket->socketDescriptor());
}

void Server::slotProccesBuffer(qintptr socketDescriptor)
{
    QByteArray &buffer = m_buffers[socketDescriptor];

    while (buffer.size() >= PacketHandler::headerSize) {
        uint32_t expectedDataSize = PacketHandler::extractDataSizeFromPacket(buffer);
        uint32_t totalPacketSize = expectedDataSize + PacketHandler::headerSize;

        if (buffer.size() < totalPacketSize) {
            qDebug() << "Ожидаем данные. Текущий размер:" << buffer.size()
                     << "Ожидаемый:" << totalPacketSize;
            break;
        }

        /// данных в буффере достаточно либо даже больше, поэтому извлекаем ровно длину пакета
        QByteArray fullPacket = buffer.left(totalPacketSize);
        buffer.remove(0, totalPacketSize);

        /// извлекаем контрольную сумму из пятого байта пакета, т.к. ниже сравним ее с фактической (посчитанной заново)
        uint8_t expectedCrc = PacketHandler::extractCrcFromPacket(fullPacket);
        uint8_t factualCrc = PacketHandler::calcCrc(fullPacket.mid(6));

        /// произошло искажение, отправляем соответствующее сообщение в очищаем буффер полностью
        if (expectedCrc != factualCrc) {
            slotSendCrcError(socketDescriptor);
            buffer.remove(0, totalPacketSize);
            return;
        }

        MessageType::RequestType requestType = PacketHandler::extractRequestTypeFromPacket(fullPacket);
        switch(requestType) {
            case MessageType::RequestType::REQUEST_REGISTR:
                slotHandleRegisterRequest(fullPacket.mid(6), socketDescriptor);
                break;

            case MessageType::RequestType::REQUEST_LOGIN:
                slotHandleAuthRequest(fullPacket.mid(6), socketDescriptor);
                break;

            default:
                qDebug() << "Неизвестный тип запроса";
                break;
        }
    }
}

void Server::slotSendCrcError(qintptr socketDescriptor)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);

    /// формируем пустой пакет
    stream << uint32_t(0);
    stream << uint8_t(0);
    stream << uint8_t(MessageType::RESPONSE_CRC_ERROR);

    slotSendToClient(packet, socketDescriptor);
}

void Server::slotSendToClient(const QByteArray &packet,qintptr socketDescriptor)
{
    if (!m_sockets.contains(socketDescriptor)) {
        qDebug() << "Unknown client";
        return;
    }

    QTcpSocket* socket = m_sockets[socketDescriptor];
    if (socket->state() != QTcpSocket::ConnectedState) {
        qDebug() << "Client is not connected";
        return;
    }

    m_sockets[socketDescriptor]->write(packet);
}

void Server::slotHandleRegisterRequest(const QByteArray &packetData, qintptr socketDescriptor)
{
    QPair<QString, QString> credentials = Serializer::deserializeAuthRequest(packetData);
    QPair<Db::RegisterStatus, int> dbResponse = m_db.registerUser(credentials.first, credentials.second);
    QByteArray serializedResponse = Serializer::serializeRegisterResponse(dbResponse);
    QByteArray packet = PacketBuilder::createPacket(serializedResponse, MessageType::RESPONSE_REGISTR);
    slotSendToClient(packet, socketDescriptor);
}

void Server::slotHandleAuthRequest(const QByteArray &packetData, qintptr socketDescriptor)
{
    QPair<QString, QString> credentials = Serializer::deserializeAuthRequest(packetData);
    QPair<Db::AuthStatus, int> dbResponse = m_db.authUser(credentials.first, credentials.second);
    QByteArray serializedResponse = Serializer::serializeAuthResponse(dbResponse);
    QByteArray packet = PacketBuilder::createPacket(serializedResponse, MessageType::RESPONSE_LOGIN);
    slotSendToClient(packet, socketDescriptor);
}

bool Server::isConnected(qintptr socketDescriptor) const
{
    if (!m_sockets.contains(socketDescriptor)) {
        qDebug() << "Request from unknown client:" << socketDescriptor;
        return false;
    }

    return true;
}
