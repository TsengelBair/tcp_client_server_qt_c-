#include "packethandler.h"
#include <QDataStream>

uint32_t PacketHandler::extractDataSizeFromPacket(const QByteArray &packet)
{
    QByteArray headerPartWithDataSize = packet.left(4);
    uint32_t dataSize;

    QDataStream ds(&headerPartWithDataSize, QIODevice::ReadOnly);
    ds >> dataSize;

    return dataSize;
}

uint8_t PacketHandler::extractCrcFromPacket(const QByteArray &packet)
{
    return static_cast<uint8_t>(packet.at(4));
}

uint8_t PacketHandler::calcCrc(const QByteArray &packetData)
{
    uint8_t crc = 0;
    for (const char c: packetData) {
        crc ^= static_cast<uint8_t>(c);
    }

    return crc;
}

MessageType::RequestType PacketHandler::extractRequestTypeFromPacket(const QByteArray &packet)
{
    return static_cast<MessageType::RequestType>(packet.at(5));
}
