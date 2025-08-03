#ifndef IDBHANDLER_H
#define IDBHANDLER_H

#include "structs.h"
#include <QPair>

class IDbHandler
{
public:
    virtual QPair<Db::RegisterStatus, int> registerUser(const QString &login, const QString &password) = 0;
    virtual QPair<Db::AuthStatus, int> authUser(const QString &login, const QString &password) = 0;

    virtual ~IDbHandler(){};
};

#endif // IDBHANDLER_H
