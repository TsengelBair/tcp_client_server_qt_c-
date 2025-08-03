#ifndef POSTGRESDB_H
#define POSTGRESDB_H

#include "IDbHandler.h"
#include "structs.h"
#include <QSqlDatabase>

class PostgresDb : public IDbHandler
{
public:
    PostgresDb(const QString &hostName, const QString &dbName, const QString &userName);
    ~PostgresDb();

    QPair<Db::RegisterStatus, int> registerUser(const QString &login, const QString &password) override;
    QPair<Db::AuthStatus, int> authUser(const QString &login, const QString &password) override;

private:
    bool userExist(const QString &login);
    bool executeQuery(QSqlQuery &query);
    bool checkPasswords(const QString &password, const QString &passwordFromDb);

private:
    QSqlDatabase m_db;
};

#endif // POSTGRESDB_H
