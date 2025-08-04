#include "serializer.h"
#include "../protofiles/IAuthRequest.pb.h"
#include "../protofiles/IAuthResponse.pb.h"

#include <QByteArray>
#include <QDebug>

QByteArray Serializer::serializeAuthRequest(const QString &login, const QString &password)
{
    IAuthRequest authRequest;

    authRequest.set_login(login.toStdString());
    authRequest.set_password(password.toStdString());

    std::string serializedData;
    if (!authRequest.SerializeToString(&serializedData)) {
        qDebug() << "Error while serialize";
        return QByteArray();
    }

    QByteArray data(serializedData.c_str(), serializedData.size());
    return data;
}

QPair<Db::RegisterStatus, int> Serializer::deserializeRegisterResponse(const QByteArray &data)
{
    IAuthResponse authResponse;

    if (!authResponse.ParseFromString(data.toStdString())) {
        qDebug() << "Error: Failed to parse IAuthResponse";
        return {Db::RegisterUnknownError, -1};
    }

    int statusCode = authResponse.status_code();
    int userId = authResponse.id();

    Db::RegisterStatus status;
    switch (statusCode) {
        case 0:  status = Db::RegisterSuccess;       break;
        case 1:  status = Db::RegisterUserExists;    break;
        case 2:  status = Db::RegisterDatabaseError; break;
        default: status = Db::RegisterUnknownError;
    }

    return {status, userId};
}

QPair<Db::AuthStatus, int> Serializer::deserializeAuthResponse(const QByteArray &data)
{
    IAuthResponse authResponse;

    if (!authResponse.ParseFromString(data.toStdString())) {
        qDebug() << "Error: Failed to parse IAuthResponse";
        return {Db::AuthUnknownError, -1};
    }

    int statusCode = authResponse.status_code();
    int userId = authResponse.id();

    Db::AuthStatus status;
    switch (statusCode) {
        case 0:  status = Db::AuthSuccess;         break;
        case 1:  status = Db::AuthUserNotFound;    break;
        case 2:  status = Db::AuthInvalidPassword; break;
        case 3:  status = Db::AuthDatabaseError;   break;
        default: status = Db::AuthUnknownError;
    }

    return {status, userId};
}
