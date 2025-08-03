#include "serializer.h"

#include "../protofiles/IAuthRequest.pb.h"
#include "../protofiles/IAuthResponse.pb.h"

#include <QDebug>

QPair<QString, QString> Serializer::deserializeAuthRequest(const QByteArray &packetData)
{
    IAuthRequest authRequest;

    if (!authRequest.ParseFromString(packetData.toStdString())) {
        qDebug() << "Error while deserialize auth request";
        return QPair<QString, QString>();
    }

    QString login = QString::fromStdString(authRequest.login());
    QString password = QString::fromStdString(authRequest.password());

    return qMakePair(login, password);
}

QByteArray Serializer::serializeRegisterResponse(const QPair<Db::RegisterStatus, int> &dbResponse)
{
    IAuthResponse authResponse;

    authResponse.set_status_code(dbResponse.first);
    authResponse.set_id(dbResponse.second);

    std::string serializedData;
    if (!authResponse.SerializeToString(&serializedData)) {
        qDebug() << "Error while serialize";
        return QByteArray();
    }

    QByteArray data(serializedData.c_str(), serializedData.size());;
    return data;
}

QByteArray Serializer::serializeAuthResponse(const QPair<Db::AuthStatus, int> &dbResponse)
{
    IAuthResponse authResponse;

    authResponse.set_status_code(dbResponse.first);
    authResponse.set_id(dbResponse.second);

    std::string serializedData;
    if (!authResponse.SerializeToString(&serializedData)) {
        qDebug() << "Error while serialize";
        return QByteArray();
    }

    QByteArray data(serializedData.c_str(), serializedData.size());;
    return data;
}
