#include "packetbuilder.h"
#include "packethandler.h"
#include <QDataStream>

QByteArray PacketBuilder::createPacket(const QByteArray &data, MessageType::ResponseType responseType)
{
    /// пакет который создадим -> первые 4 байта размер данных, 5 байт - crc, 6 байт - тип запроса и после 6 идут данные - data
    QByteArray packet;

    uint32_t packetSize = data.size();
    QDataStream out(&packet, QIODevice::WriteOnly);
    out << packetSize;

    uint8_t crc = PacketHandler::calcCrc(data);
    packet.append(crc);
    packet.append(static_cast<char>(responseType));
    packet.append(data);

    return packet;
}
