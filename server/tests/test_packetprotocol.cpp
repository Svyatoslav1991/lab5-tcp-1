#include <QtTest>

#include "packetprotocol.h"

/*!
 * \class TestPacketProtocol
 * \brief Набор unit-тестов для PacketProtocol на серверной стороне.
 *
 * \details
 * Тесты покрывают:
 * - сборку и разбор request-frame;
 * - сборку и разбор response-frame;
 * - извлечение кадра из буфера по частям;
 * - извлечение нескольких кадров из одного буфера;
 * - обработку некорректных заголовков;
 * - отказ разбора невалидной полезной нагрузки.
 */
class TestPacketProtocol : public QObject
{
    Q_OBJECT

private slots:
    /*!
     * \brief Проверяет корректный round-trip для request-пакета.
     */
    void buildRequestFrame_thenExtractAndParse_returnsOriginalRequest();

    /*!
     * \brief Проверяет корректный round-trip для response-пакета.
     */
    void buildResponseFrame_thenExtractAndParse_returnsOriginalResponse();

    /*!
     * \brief Проверяет, что кадр не извлекается, пока в буфере недостаточно данных.
     */
    void tryExtractFrame_returnsFalseForIncompleteHeaderAndPayload();

    /*!
     * \brief Проверяет извлечение кадра, если данные приходят частями.
     */
    void tryExtractFrame_extractsFrameWhenBufferBecomesComplete();

    /*!
     * \brief Проверяет последовательное извлечение двух кадров из одного буфера.
     */
    void tryExtractFrame_extractsMultipleFramesSequentially();

    /*!
     * \brief Проверяет сброс буфера при нулевом размере payload.
     */
    void tryExtractFrame_returnsFalseAndClearsBufferForZeroPayloadSize();

    /*!
     * \brief Проверяет сброс буфера при слишком большом размере payload.
     */
    void tryExtractFrame_returnsFalseAndClearsBufferForTooLargePayloadSize();

    /*!
     * \brief Проверяет отказ разбора request payload при усечённых данных.
     */
    void parseRequestPayload_returnsFalseForCorruptedPayload();

    /*!
     * \brief Проверяет отказ разбора response payload при усечённых данных.
     */
    void parseResponsePayload_returnsFalseForCorruptedPayload();
};

//--------------------------------------------------------------------------

void TestPacketProtocol::buildRequestFrame_thenExtractAndParse_returnsOriginalRequest()
{
    const RequestPacket sourcePacket { 42u, QStringLiteral("hello request") };
    const QByteArray frame = PacketProtocol::buildRequestFrame(sourcePacket);

    QByteArray buffer = frame;
    quint32 pendingBlockSize = 0;
    QByteArray payload;

    QVERIFY(PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QVERIFY(buffer.isEmpty());
    QCOMPARE(pendingBlockSize, 0u);

    RequestPacket parsedPacket;
    QVERIFY(PacketProtocol::parseRequestPayload(payload, parsedPacket));

    QCOMPARE(parsedPacket.number, sourcePacket.number);
    QCOMPARE(parsedPacket.text, sourcePacket.text);
}

//--------------------------------------------------------------------------

void TestPacketProtocol::buildResponseFrame_thenExtractAndParse_returnsOriginalResponse()
{
    ResponsePacket sourcePacket;
    sourcePacket.number = 17u;
    sourcePacket.text = QStringLiteral("hello response");
    sourcePacket.serverTime = QTime(12, 34, 56);

    const QByteArray frame = PacketProtocol::buildResponseFrame(sourcePacket);

    QByteArray buffer = frame;
    quint32 pendingBlockSize = 0;
    QByteArray payload;

    QVERIFY(PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QVERIFY(buffer.isEmpty());
    QCOMPARE(pendingBlockSize, 0u);

    ResponsePacket parsedPacket;
    QVERIFY(PacketProtocol::parseResponsePayload(payload, parsedPacket));

    QCOMPARE(parsedPacket.number, sourcePacket.number);
    QCOMPARE(parsedPacket.text, sourcePacket.text);
    QCOMPARE(parsedPacket.serverTime, sourcePacket.serverTime);
}

//--------------------------------------------------------------------------

void TestPacketProtocol::tryExtractFrame_returnsFalseForIncompleteHeaderAndPayload()
{
    QByteArray buffer;
    quint32 pendingBlockSize = 0;
    QByteArray payload;

    buffer.append(char(0x01));
    buffer.append(char(0x02));

    QVERIFY(!PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QCOMPARE(buffer.size(), 2);
    QCOMPARE(pendingBlockSize, 0u);
    QVERIFY(payload.isEmpty());

    const RequestPacket packet { 5u, QStringLiteral("abc") };
    const QByteArray frame = PacketProtocol::buildRequestFrame(packet);

    buffer = frame.left(PacketProtocol::HeaderSize + 1);
    pendingBlockSize = 0;
    payload.clear();

    QVERIFY(!PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QVERIFY(!buffer.isEmpty());
    QVERIFY(pendingBlockSize > 0);
    QVERIFY(payload.isEmpty());
}

//--------------------------------------------------------------------------

void TestPacketProtocol::tryExtractFrame_extractsFrameWhenBufferBecomesComplete()
{
    const RequestPacket packet { 99u, QStringLiteral("partial frame") };
    const QByteArray frame = PacketProtocol::buildRequestFrame(packet);

    QByteArray buffer = frame.left(3);
    quint32 pendingBlockSize = 0;
    QByteArray payload;

    QVERIFY(!PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QCOMPARE(pendingBlockSize, 0u);

    buffer.append(frame.mid(3, 4));
    QVERIFY(!PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QVERIFY(pendingBlockSize > 0);

    buffer.append(frame.mid(7));
    QVERIFY(PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QVERIFY(buffer.isEmpty());
    QCOMPARE(pendingBlockSize, 0u);

    RequestPacket parsedPacket;
    QVERIFY(PacketProtocol::parseRequestPayload(payload, parsedPacket));
    QCOMPARE(parsedPacket.number, packet.number);
    QCOMPARE(parsedPacket.text, packet.text);
}

//--------------------------------------------------------------------------

void TestPacketProtocol::tryExtractFrame_extractsMultipleFramesSequentially()
{
    const RequestPacket firstPacket { 1u, QStringLiteral("first") };
    const RequestPacket secondPacket { 2u, QStringLiteral("second") };

    QByteArray buffer;
    buffer.append(PacketProtocol::buildRequestFrame(firstPacket));
    buffer.append(PacketProtocol::buildRequestFrame(secondPacket));

    quint32 pendingBlockSize = 0;
    QByteArray firstPayload;
    QByteArray secondPayload;

    QVERIFY(PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, firstPayload));
    QVERIFY(PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, secondPayload));
    QVERIFY(buffer.isEmpty());
    QCOMPARE(pendingBlockSize, 0u);

    RequestPacket parsedFirstPacket;
    RequestPacket parsedSecondPacket;

    QVERIFY(PacketProtocol::parseRequestPayload(firstPayload, parsedFirstPacket));
    QVERIFY(PacketProtocol::parseRequestPayload(secondPayload, parsedSecondPacket));

    QCOMPARE(parsedFirstPacket.number, firstPacket.number);
    QCOMPARE(parsedFirstPacket.text, firstPacket.text);

    QCOMPARE(parsedSecondPacket.number, secondPacket.number);
    QCOMPARE(parsedSecondPacket.text, secondPacket.text);
}

//--------------------------------------------------------------------------

void TestPacketProtocol::tryExtractFrame_returnsFalseAndClearsBufferForZeroPayloadSize()
{
    QByteArray buffer;
    quint32 pendingBlockSize = 0;
    QByteArray payload;

    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream.setVersion(PacketProtocol::StreamVersion);
    stream << static_cast<quint32>(0);

    QVERIFY(!PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QVERIFY(buffer.isEmpty());
    QCOMPARE(pendingBlockSize, 0u);
    QVERIFY(payload.isEmpty());
}

//--------------------------------------------------------------------------

void TestPacketProtocol::tryExtractFrame_returnsFalseAndClearsBufferForTooLargePayloadSize()
{
    QByteArray buffer;
    quint32 pendingBlockSize = 0;
    QByteArray payload;

    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream.setVersion(PacketProtocol::StreamVersion);
    stream << static_cast<quint32>(PacketProtocol::MaxPayloadSize + 1);

    QVERIFY(!PacketProtocol::tryExtractFrame(buffer, pendingBlockSize, payload));
    QVERIFY(buffer.isEmpty());
    QCOMPARE(pendingBlockSize, 0u);
    QVERIFY(payload.isEmpty());
}

//--------------------------------------------------------------------------

void TestPacketProtocol::parseRequestPayload_returnsFalseForCorruptedPayload()
{
    QByteArray corruptedPayload;
    QDataStream stream(&corruptedPayload, QIODevice::WriteOnly);
    stream.setVersion(PacketProtocol::StreamVersion);
    stream << static_cast<quint32>(123u);

    RequestPacket packet;
    QVERIFY(!PacketProtocol::parseRequestPayload(corruptedPayload, packet));
}

//--------------------------------------------------------------------------

void TestPacketProtocol::parseResponsePayload_returnsFalseForCorruptedPayload()
{
    QByteArray corruptedPayload;
    QDataStream stream(&corruptedPayload, QIODevice::WriteOnly);
    stream.setVersion(PacketProtocol::StreamVersion);
    stream << static_cast<quint32>(123u) << QStringLiteral("only text without time");

    ResponsePacket packet;
    QVERIFY(!PacketProtocol::parseResponsePayload(corruptedPayload, packet));
}

QTEST_APPLESS_MAIN(TestPacketProtocol)

#include "test_packetprotocol.moc"
