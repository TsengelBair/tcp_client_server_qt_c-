#ifndef STRUCTS_H
#define STRUCTS_H

/// тип сообщения вшиваемый в 6 байт пакета
/// для идентификации на основе какого прото файла выполнять десериализацию на стороне получателя
namespace MessageType
{
    enum RequestType : char
    {
        REQUEST_REGISTR,
        REQUEST_LOGIN
    };

    enum ResponseType : char
    {
        RESPONSE_REGISTR,
        RESPONSE_LOGIN,
        RESPONSE_CRC_ERROR
    };
};

namespace Db
{
    enum RegisterStatus : char
    {
        RegisterSuccess,
        RegisterUserExists,
        RegisterDatabaseError,
        RegisterUnknownError
    };

    enum AuthStatus : char
    {
        AuthSuccess,
        AuthUserNotFound,
        AuthInvalidPassword,
        AuthDatabaseError,
        AuthUnknownError
    };
}

#endif // STRUCTS_H
