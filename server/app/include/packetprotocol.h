#ifndef PACKETPROTOCOL_H
#define PACKETPROTOCOL_H

#include <QByteArray>
#include <QDataStream>
#include <QString>
#include <QTime>

/*!
 * \struct RequestPacket
 * \brief Пакет клиентского запроса.
 */
struct RequestPacket
{
    quint32 number = 0; /*!< Числовое поле пакета. */
    QString text;       /*!< Строковое поле пакета. */
};

/*!
 * \struct ResponsePacket
 * \brief Пакет серверного ответа.
 */
struct ResponsePacket
{
    quint32 number = 0; /*!< Числовое поле пакета. */
    QString text;       /*!< Строковое поле пакета. */
    QTime serverTime;   /*!< Время сервера. */
};

/*!
 * \class PacketProtocol
 * \brief Вспомогательный класс для пакетного TCP-протокола поверх QDataStream.
 *
 * \details
 * Класс инкапсулирует:
 * - сборку бинарных кадров запросов и ответов;
 * - извлечение полного кадра из потокового TCP-буфера;
 * - разбор полезной нагрузки запроса и ответа.
 *
 * Каждый кадр имеет формат:
 * - \c quint32 payloadSize
 * - полезная нагрузка, сериализованная через \c QDataStream
 */
class PacketProtocol
{
public:
    /*!
     * \brief Размер заголовка кадра.
     */
    static constexpr int HeaderSize = static_cast<int>(sizeof(quint32));

    /*!
     * \brief Максимально допустимый размер полезной нагрузки.
     */
    static constexpr quint32 MaxPayloadSize = 1024 * 1024;

    /*!
     * \brief Версия QDataStream для сериализации и десериализации.
     */
    static constexpr QDataStream::Version StreamVersion = QDataStream::Qt_5_12;

public:
    /*!
     * \brief Формирует бинарный кадр клиентского запроса.
     * \param packet Пакет запроса.
     * \return Готовый бинарный кадр.
     */
    static QByteArray buildRequestFrame(const RequestPacket &packet);

    /*!
     * \brief Формирует бинарный кадр серверного ответа.
     * \param packet Пакет ответа.
     * \return Готовый бинарный кадр.
     */
    static QByteArray buildResponseFrame(const ResponsePacket &packet);

    /*!
     * \brief Пытается извлечь один полный кадр из накопленного TCP-буфера.
     * \param buffer Буфер входных байтов.
     * \param pendingBlockSize Ожидаемый размер полезной нагрузки текущего кадра.
     * \param payload Выходная полезная нагрузка извлечённого кадра.
     * \return true, если полный кадр извлечён, иначе false.
     */
    static bool tryExtractFrame(QByteArray &buffer,
                                quint32 &pendingBlockSize,
                                QByteArray &payload);

    /*!
     * \brief Разбирает полезную нагрузку клиентского запроса.
     * \param payload Полезная нагрузка.
     * \param packet Выходной пакет запроса.
     * \return true, если разбор успешен, иначе false.
     */
    static bool parseRequestPayload(const QByteArray &payload, RequestPacket &packet);

    /*!
     * \brief Разбирает полезную нагрузку серверного ответа.
     * \param payload Полезная нагрузка.
     * \param packet Выходной пакет ответа.
     * \return true, если разбор успешен, иначе false.
     */
    static bool parseResponsePayload(const QByteArray &payload, ResponsePacket &packet);
};

#endif // PACKETPROTOCOL_H
