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

    static QByteArray serializeAuthRequest(const QString &login, const QString &password);
    static QPair<Db::RegisterStatus, int> deserializeRegisterResponse(const QByteArray &data);
    static QPair<Db::AuthStatus, int> deserializeAuthResponse(const QByteArray &data);
};

#endif // SERIALIZER_H
