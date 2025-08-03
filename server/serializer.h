#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "structs.h"
#include <QPair>

class Serializer
{
public:
    Serializer() = delete;
    Serializer(const Serializer&) = delete;
    Serializer& operator=(const Serializer&) = delete;

    static QPair<QString, QString> deserializeAuthRequest(const QByteArray &packetData);
    static QByteArray serializeRegisterResponse(const QPair<Db::RegisterStatus, int> &dbResponse);
    static QByteArray serializeAuthResponse(const QPair<Db::AuthStatus, int> &dbResponse);
};

#endif // SERIALIZER_H
