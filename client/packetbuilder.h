#ifndef PACKETBUILDER_H
#define PACKETBUILDER_H

#include "structs.h"
#include <QByteArray>

class PacketBuilder
{
public:
    PacketBuilder() = delete;
    PacketBuilder(const PacketBuilder&) = delete;
    PacketBuilder& operator=(const PacketBuilder&) = delete;

    static QByteArray createPacket(const QByteArray &data, MessageType::RequestType requestType);
};

#endif // PACKETBUILDER_H
