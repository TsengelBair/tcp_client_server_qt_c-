#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>

class IDbHandler;

class Server : public QTcpServer
{
public:
    explicit Server(const QHostAddress &address, unsigned short port, IDbHandler &db);
    ~Server();

private slots:
    void incomingConnection(qintptr socketDescriptor) override;
    void slotHandleDataFromClient();
    void slotProccesBuffer(qintptr socketDescriptor);
    void slotSendCrcError(qintptr socketDescriptor);
    void slotSendToClient(const QByteArray &packet, qintptr socketDescriptor);
    void slotHandleRegisterRequest(const QByteArray &packetData, qintptr socketDescriptor);
    void slotHandleAuthRequest(const QByteArray &packetData, qintptr socketDescriptor);

private:
    void runServer(const QHostAddress &address, unsigned short port);
    bool isConnected(qintptr socketDescriptor) const;

private:
    IDbHandler &m_db;
    QHash<qintptr, QTcpSocket*> m_sockets;
    QHash<qintptr, QByteArray> m_buffers;
};

#endif // SERVER_H
