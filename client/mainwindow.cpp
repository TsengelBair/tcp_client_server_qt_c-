#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "serializer.h"
#include "packetbuilder.h"
#include "packethandler.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_socket = new QTcpSocket(this);
    m_socket->connectToHost("127.0.0.1", 5000);

    connect(ui->pbRegistr, &QPushButton::clicked, this, [this](){sendToServer(MessageType::REQUEST_REGISTR);});
    connect(ui->pbLogin, &QPushButton::clicked, this, [this](){sendToServer(MessageType::REQUEST_LOGIN);});
    connect(m_socket, &QTcpSocket::readyRead, this, &MainWindow::slotHandleServerResponse);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::sendToServer(const MessageType::RequestType &requestType)
{
    QPair<QString, QString> credentials = getDataFromUI();
    if (credentials.first.isEmpty() || credentials.second.isEmpty()) return;

    QByteArray serializedData = Serializer::serializeAuthRequest(credentials.first, credentials.second);
    if (serializedData.isEmpty()) return;

    QByteArray packet = PacketBuilder::createPacket(serializedData, requestType);
    m_socket->write(packet);
}

QPair<QString, QString> MainWindow::getDataFromUI()
{
    QString login = ui->leLogin->text();
    QString password = ui->lePassword->text();

    if (login.isEmpty() || password.isEmpty()) {
        ui->statusbar->showMessage("Заполните все поля");
        return QPair<QString, QString>();
    }

    return qMakePair(login, password);
}

void MainWindow::slotHandleServerResponse()
{
    QByteArray newData = m_socket->readAll();
    if (newData.isEmpty()) return;

    m_buffer += newData;
    slotProccesBuffer();
}

void MainWindow::slotProccesBuffer()
{
    while (m_buffer.size() >= PacketHandler::headerSize) {
        uint32_t expectedDataSize = PacketHandler::extractDataSizeFromPacket(m_buffer);
        uint32_t totalPacketSize = expectedDataSize + PacketHandler::headerSize;

        if (m_buffer.size() < totalPacketSize) {
            qDebug() << "Ожидаем данные. Текущий размер:" << m_buffer.size()
                     << "Ожидаемый:" << totalPacketSize;
            break;
        }

        /// данных в буффере достаточно либо даже больше, поэтому извлекаем ровно длину пакета
        QByteArray fullPacket = m_buffer.left(totalPacketSize);
        m_buffer.remove(0, totalPacketSize);

        /// извлекаем контрольную сумму из пятого байта пакета, т.к. ниже сравним ее с фактической (посчитанной заново)
        uint8_t expectedCrc = PacketHandler::extractCrcFromPacket(fullPacket);
        uint8_t factualCrc = PacketHandler::calcCrc(fullPacket.mid(6));

        /// произошло искажение, отправляем соответствующее сообщение в очищаем буффер полностью
        if (expectedCrc != factualCrc) {
            slotSendCrcError();
            m_buffer.remove(0, totalPacketSize);
            return;
        }

        MessageType::ResponseType responseType = PacketHandler::extractResponseTypeFromPacket(fullPacket);
        switch(responseType) {
        case MessageType::ResponseType::RESPONSE_REGISTR:
            slotHandleRegisterResponse(fullPacket.mid(6));
            break;

        case MessageType::ResponseType::RESPONSE_LOGIN:
            slotHandleAuthResponse(fullPacket.mid(6));
            break;

        default:
            qDebug() << "Неизвестный тип запроса";
            break;
        }
    }
}

void MainWindow::slotSendCrcError()
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);

    /// формируем пустой пакет
    stream << uint32_t(0);
    stream << uint8_t(0);
    stream << uint8_t(MessageType::REQUEST_CRC_ERROR);

    m_socket->write(packet);
}

void MainWindow::slotHandleRegisterResponse(const QByteArray &packetData)
{
    QPair<Db::RegisterStatus, int> serverRegisterResponse = Serializer::deserializeRegisterResponse(packetData);

    Db::RegisterStatus registerStatus = serverRegisterResponse.first;
    switch(registerStatus) {
        case Db::RegisterStatus::RegisterDatabaseError:
            ui->statusbar->showMessage("Server error, try again later");
            break;

        case Db::RegisterStatus::RegisterUnknownError:
            ui->statusbar->showMessage("Unknown error, try again later");
            break;

        case Db::RegisterStatus::RegisterUserExists:
            ui->statusbar->showMessage("User already exists");
            break;

        case Db::RegisterStatus::RegisterSuccess:
            ui->statusbar->showMessage("Registration success!");
            break;

        default:
            qDebug() << "Unexpected register status code:" << static_cast<int>(registerStatus);
            ui->statusbar->showMessage("Unknown error, try again later");
            break;
    }
}

void MainWindow::slotHandleAuthResponse(const QByteArray &packetData)
{
    QPair<Db::AuthStatus, int> serverAuthResponse = Serializer::deserializeAuthResponse(packetData);

    Db::AuthStatus authStatus = serverAuthResponse.first;
    switch(authStatus) {
        case Db::AuthStatus::AuthSuccess:
            ui->statusbar->showMessage("Login successful!");
            break;

        case Db::AuthStatus::AuthUserNotFound:
            ui->statusbar->showMessage("User not found");
            break;

        case Db::AuthStatus::AuthInvalidPassword:
            ui->statusbar->showMessage("Invalid password");
            break;

        case Db::AuthStatus::AuthDatabaseError:
            ui->statusbar->showMessage("Database error, try again later");
            break;

        case Db::AuthStatus::AuthUnknownError:
            ui->statusbar->showMessage("Unknown error, try again later");
            break;

        default:
            qDebug() << "Unexpected auth status code:" << static_cast<int>(authStatus);
            ui->statusbar->showMessage("Unexpected server response");
            break;
    }
}
