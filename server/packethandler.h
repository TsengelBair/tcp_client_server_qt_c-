#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include "structs.h"
#include <QByteArray>

class PacketHandler
{
public:
    PacketHandler() = delete;
    PacketHandler(const PacketHandler&) = delete;
    PacketHandler& operator=(const PacketHandler&) = delete;

    static uint32_t extractDataSizeFromPacket(const QByteArray &packet);
    static uint8_t extractCrcFromPacket(const QByteArray &packet);
    static uint8_t calcCrc(const QByteArray &packetData);
    static MessageType::RequestType extractRequestTypeFromPacket(const QByteArray &packet);

    static const int headerSize = 6;
};

#endif // PACKETHANDLER_H
