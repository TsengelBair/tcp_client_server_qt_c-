### Протокол поверх TCP

Первые 4 байта резервируются под длину сообщения, пятый байт - это контрольная сумма, которая считается через XOR и шестой байт зарезервирован для хранения типа запроса/ответа.

Вшивание шестого байта необходимо для идентификации пришедшего запроса/ответа поскольку общение клиента и сервера осуществляется через protobuf.

### Создание сервера

Создадим класс Server, унаследовавшись от класса QTcpServer для переопределения виртуального метода обработки входящего подключения incomingConnection

Также создадим две хэш таблицы для хранения объектов сокетов по их дескриптору и для хранения буфферов под каждый сокет (про буфферы будет сказано немного позже)

```c++
#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>

class Server : public QTcpServer
{
public:
    explicit Server(const QHostAddress &address, unsigned short port);
    ~Server();

private slots:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QHash<qintptr, QTcpSocket*> m_sockets;
    QHash<qintptr, QByteArray> m_buffers;
};

#endif 
```

В конструкторе выполним запуск сервера 

```c++
Server::Server(const QHostAddress &address, unsigned short port, IDbHandler &db) : m_db(db)
{
    if (!this->listen(address, port)) {
        qDebug() << "Error while starting server: " << errorString();
    }
    else {
        qDebug() << "Listen";
    }
}
```

И переопределим метод обработки входящего подключения (данный метод срабатывает автоматически, когда клиент подключается к серверу)

Внутри метода создадим объект сокета, установим ему переданный дескриптор и сохраним в хэш таблицу, после чего выполним коннект. 

```c++
void Server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    m_sockets.insert(socketDescriptor, socket);
    qDebug() << "Client connected: " << socketDescriptor;

    connect(socket, &QTcpSocket::readyRead, this, &Server::slotHandleDataFromClient);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}
```

Сигнал &QTcpSocket::readyRead срабатывает каждый раз когда в сокете появляются данные для чтения, новые данные обработаем в созданном слоте slotHandleDataFromClient

Внутри метода получим объект сокета отправившего сигнал, проверим является ли сокет одним из подключенных клиентов и прочитаем новые данные из сокета в массив байт newData, в случае если данных нет дальнейшая обработка в слоте slotProccesBuffer не имеет смысла, в противном случае добавляем в буффер новые данные и запускаем слот обработчик slotProccesBuffer

```c++
void Server::slotHandleDataFromClient()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        qDebug() << "Error while cast to socket*";
        return;
    }

    if (!m_sockets.contains(socketDescriptor)) {
        qDebug() << "Request from unknown client:" << socketDescriptor;
        return;
    }

    QByteArray newData = socket->readAll();
    if (newData.isEmpty()) return;

    m_buffers[socket->socketDescriptor()] += newData;
    slotProccesBuffer(socket->socketDescriptor());
}
```

Внутри метода получим нужный буффер по дескриптору сокета и начнем обработку, но только в том случае, если данных в буффере больше 6 байт, поскольку в созданном протоколе первые 6 байт являются заголовком, где первые 4 байта это длина пакета (не считая заголовка из 6 байт), 5 байт зарезервирован под контрольную сумму, 6 байт под тип запроса/ответа и наконец уже после 6 байта идут непосредственно передаваемые данные.

Извлечем ожидаемый размер пакета из первых 4 байт и посчитаем сколько байт должен весить пакет вместе с заголовком.

Как только данных в буффере станет достаточно, извлечем необходимое количество байт в fullPacket и провалидируем его, сравнив контрольную сумму вшитую в пятый байт с контрольной суммой которую посчитаем заново, пройдясь XOR оператором по данным после 6 байта.

В случае если контрольная сумма не совпала, данные являются битыми и необходимо очистить буффер от битого пакета, отправив на клиент соответствующее сообщение

Если пакет валиден, извлекаем из 6 байта тип запроса и в зависимости от типа запроса вызываем соответствующий обработчик

```c++
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
        }
    }
}
```

Рассмотрим обработку запроса на регистрацию 

Извлекаем переданный с клиента логин и пароль, десериализовав сообщение.

Извлеченные данные передадим в класс Db, метод вернет статус код и id пользователя (в случае успешной регистрации, во всех остальных случаях id = -1)

Сериализуем статус код и айди пользователя, после чего сериализованные данные обернем в пакет, который отправим соответствующему клиенту.

```c++
void Server::slotHandleRegisterRequest(const QByteArray &packetData, qintptr socketDescriptor)
{
    QPair<QString, QString> credentials = Serializer::deserializeAuthRequest(packetData);
    QPair<Db::RegisterStatus, int> dbResponse = m_db.registerUser(credentials.first, credentials.second);
    QByteArray serializedResponse = Serializer::serializeRegisterResponse(dbResponse);
    QByteArray packet = PacketBuilder::createPacket(serializedResponse, MessageType::RESPONSE_REGISTR);
    slotSendToClient(packet, socketDescriptor);
}
```

Отправка на клиент
```c++
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
```