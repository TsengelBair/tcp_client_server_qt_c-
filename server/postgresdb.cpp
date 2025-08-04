#include "postgresdb.h"

#include <QSqlError>
#include <QSqlQuery>

#include <QDebug>

PostgresDb::PostgresDb(const QString &hostName, const QString &dbName, const QString &userName)
{
    m_db = QSqlDatabase::addDatabase("QPSQL");
    m_db.setHostName(hostName);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(userName);

    if (!m_db.open()) {
        qDebug() << "Database connection error:" << m_db.lastError().text();
    }
}

PostgresDb::~PostgresDb()
{
    if (m_db.isOpen()) m_db.close();
}

QPair<Db::RegisterStatus, int> PostgresDb::registerUser(const QString &login, const QString &password)
{
    if (!m_db.isOpen()) return {Db::RegisterDatabaseError, -1};

    try {
        if (userExist(login)) return {Db::RegisterUserExists, -1};

        QSqlQuery query(m_db);
        query.prepare("INSERT INTO users (login, password) VALUES (:login, :password)");
        query.bindValue(":login", login);
        query.bindValue(":password", password);

        executeQuery(query);
        if (query.next()) {
            int userId = query.value(0).toInt();
            return {Db::RegisterSuccess, userId};
        }

        return {Db::RegisterDatabaseError, -1};

    } catch (const std::exception &e) {
        qDebug() << "Registration error:" << e.what();
        return {Db::RegisterDatabaseError, -1};
    }
}

QPair<Db::AuthStatus, int> PostgresDb::authUser(const QString &login, const QString &password)
{
    if (!m_db.isOpen()) return {Db::AuthDatabaseError, -1};

    QSqlQuery query(m_db);
    try {
        query.prepare("SELECT id, password FROM users WHERE login = :login");
        query.bindValue(":login", login);

        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }

        if (!query.next()) {
            qDebug() << "User not found:" << login;
            query.finish();
            return {Db::AuthUserNotFound, -1};
        }

        int userId = query.value(0).toInt();
        QString storedPassword = query.value(1).toString();
        query.finish();

        if (!checkPasswords(password, storedPassword)) {
            qDebug() << "Invalid password for user:" << login;
            return {Db::AuthInvalidPassword, -1};
        }

        qDebug() << "User authenticated successfully:" << login;
        return {Db::AuthSuccess, userId};
    }
    catch (const std::exception &e) {
        qDebug() << "Authentication error:" << e.what();
        if (query.isActive()) query.finish();
        return {Db::AuthDatabaseError, -1};
    }
}
bool PostgresDb::userExist(const QString &login)
{
    if (!m_db.isOpen()) {
        throw std::runtime_error("Database connection is not open");
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM users WHERE login = :login");
    query.bindValue(":login", login);

    executeQuery(query);  /// Будет бросать исключение при ошибке
    return query.next();
}

bool PostgresDb::executeQuery(QSqlQuery &query)
{
    if (!query.exec()) {
        throw std::runtime_error(
            "Query execution failed: " +
            query.lastError().text().toStdString()
            );
    }
    return true;
}

bool PostgresDb::checkPasswords(const QString &password, const QString &passwordFromDb)
{
    /// в реальном проекте в бд хранится захэшированный пароль
    /// в этом методе необходимо захэшировать переданный, поэтому он вынесен в отдельный метод
    return password == passwordFromDb;
}
