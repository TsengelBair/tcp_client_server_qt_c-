#include "server.h"
#include "postgresdb.h"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    PostgresDb db("localhost", "test_db", "postgres");

    Server server(QHostAddress::LocalHost, 5000, db);

    return a.exec();
}
