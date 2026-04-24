#include "packetprotocol.h"

QByteArray PacketProtocol::buildRequestFrame(const RequestPacket &packet)
{
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setVersion(StreamVersion);
    payloadStream << packet.number << packet.text;

    QByteArray frame;
    QDataStream frameStream(&frame, QIODevice::WriteOnly);
    frameStream.setVersion(StreamVersion);
    frameStream << static_cast<quint32>(payload.size());

    frame.append(payload);

    return frame;
}

//--------------------------------------------------------------------------

QByteArray PacketProtocol::buildResponseFrame(const ResponsePacket &packet)
{
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setVersion(StreamVersion);
    payloadStream << packet.number << packet.text << packet.serverTime;

    QByteArray frame;
    QDataStream frameStream(&frame, QIODevice::WriteOnly);
    frameStream.setVersion(StreamVersion);
    frameStream << static_cast<quint32>(payload.size());

    frame.append(payload);

    return frame;
}

//--------------------------------------------------------------------------

bool PacketProtocol::tryExtractFrame(QByteArray &buffer,
                                     quint32 &pendingBlockSize,
                                     QByteArray &payload)
{
    payload.clear();

    if (pendingBlockSize == 0) {
        if (buffer.size() < HeaderSize) {
            return false;
        }

        QByteArray headerData = buffer.left(HeaderSize);
        QDataStream headerStream(&headerData, QIODevice::ReadOnly);
        headerStream.setVersion(StreamVersion);
        headerStream >> pendingBlockSize;

        if (pendingBlockSize == 0 || pendingBlockSize > MaxPayloadSize) {
            buffer.clear();
            pendingBlockSize = 0;
            return false;
        }
    }

    const int fullFrameSize = HeaderSize + static_cast<int>(pendingBlockSize);

    if (buffer.size() < fullFrameSize) {
        return false;
    }

    payload = buffer.mid(HeaderSize, static_cast<int>(pendingBlockSize));
    buffer.remove(0, fullFrameSize);
    pendingBlockSize = 0;

    return true;
}

//--------------------------------------------------------------------------

bool PacketProtocol::parseRequestPayload(const QByteArray &payload, RequestPacket &packet)
{
    QByteArray payloadCopy = payload;
    QDataStream payloadStream(&payloadCopy, QIODevice::ReadOnly);
    payloadStream.setVersion(StreamVersion);

    payloadStream >> packet.number >> packet.text;

    return payloadStream.status() == QDataStream::Ok;
}

//--------------------------------------------------------------------------

bool PacketProtocol::parseResponsePayload(const QByteArray &payload, ResponsePacket &packet)
{
    QByteArray payloadCopy = payload;
    QDataStream payloadStream(&payloadCopy, QIODevice::ReadOnly);
    payloadStream.setVersion(StreamVersion);

    payloadStream >> packet.number >> packet.text >> packet.serverTime;

    return payloadStream.status() == QDataStream::Ok;
}
