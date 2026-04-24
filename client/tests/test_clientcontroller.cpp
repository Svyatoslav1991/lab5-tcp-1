#include <QtTest>

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

#include "clientcontroller.h"
#include "packetprotocol.h"

/*!
 * \class TestClientController
 * \brief Набор unit/integration-тестов для ClientController.
 *
 * \details
 * Тесты проверяют публичный контракт контроллера:
 * - начальное состояние;
 * - подключение и отключение;
 * - отправку одного пакета;
 * - режим 2 с периодической отправкой;
 * - режим 3 с периодическим подключением;
 * - корректный shutdown.
 *
 * Для сетевых сценариев используется локальный QTcpServer на localhost.
 */
class TestClientController : public QObject
{
    Q_OBJECT

private:
    /*!
     * \brief Проверяет наличие подстроки среди сообщений QSignalSpy.
     * \param spy Spy для сигнала logMessage(QString).
     * \param expectedSubstring Ожидаемая подстрока.
     * \return true, если подходящее сообщение найдено.
     */
    static bool spyContainsMessage(const QSignalSpy &spy, const QString &expectedSubstring);

    /*!
     * \brief Подключает обработчик готовности чтения серверного сокета и собирает request-пакеты.
     * \param socket Сокет серверной стороны.
     * \param rawBuffer Буфер накопленных байтов.
     * \param pendingBlockSize Ожидаемый размер текущего входного кадра.
     * \param packets Коллекция распарсенных request-пакетов.
     */
    static void attachRequestCollector(QTcpSocket *socket,
                                       QByteArray &rawBuffer,
                                       quint32 &pendingBlockSize,
                                       QVector<RequestPacket> &packets);

private slots:
    /*!
     * \brief Проверяет начальное состояние контроллера.
     */
    void constructor_setsExpectedInitialState();

    /*!
     * \brief Проверяет disconnectFromHost() в состоянии UnconnectedState.
     */
    void disconnectFromHost_whenUnconnected_emitsOnlyStateChanged();

    /*!
     * \brief Проверяет обычное подключение и отключение через локальный сервер.
     */
    void connectToHost_andDisconnectFromHost_workWithLocalServer();

    /*!
     * \brief Проверяет отправку одного request-пакета после подключения.
     */
    void sendSinglePacket_whenConnected_sendsSerializedPacket();

    /*!
     * \brief Проверяет запуск режима 2 и периодическую отправку нескольких пакетов.
     */
    void startLongMode_whenConnected_startsTimerAndSendsMultiplePackets();

    /*!
     * \brief Проверяет запуск режима 3 и автоподключение с отправкой пакета.
     */
    void startShortMode_startsTimerAndAttemptsConnectionCycle();

    /*!
     * \brief Проверяет, что shutdown() останавливает таймер и закрывает соединение.
     */
    void shutdown_stopsPeriodicModeAndDisconnectsSocket();
};

//--------------------------------------------------------------------------

bool TestClientController::spyContainsMessage(const QSignalSpy &spy, const QString &expectedSubstring)
{
    for (const QList<QVariant> &arguments : spy) {
        if (arguments.isEmpty()) {
            continue;
        }

        const QString message = arguments.first().toString();

        if (message.contains(expectedSubstring)) {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------

void TestClientController::attachRequestCollector(QTcpSocket *socket,
                                                  QByteArray &rawBuffer,
                                                  quint32 &pendingBlockSize,
                                                  QVector<RequestPacket> &packets)
{
    QObject::connect(socket, &QTcpSocket::readyRead, [socket, &rawBuffer, &pendingBlockSize, &packets]() {
        rawBuffer.append(socket->readAll());

        while (true) {
            QByteArray payload;

            if (!PacketProtocol::tryExtractFrame(rawBuffer, pendingBlockSize, payload)) {
                break;
            }

            RequestPacket packet;

            if (PacketProtocol::parseRequestPayload(payload, packet)) {
                packets.push_back(packet);
            }
        }
    });
}

//--------------------------------------------------------------------------

void TestClientController::constructor_setsExpectedInitialState()
{
    ClientController controller;

    QCOMPARE(controller.socketState(), QAbstractSocket::UnconnectedState);
    QVERIFY(!controller.isPeriodicModeActive());
    QCOMPARE(controller.activePeriodicMode(), SessionMode::Single);
}

//--------------------------------------------------------------------------

void TestClientController::disconnectFromHost_whenUnconnected_emitsOnlyStateChanged()
{
    ClientController controller;

    QSignalSpy logSpy(&controller, &ClientController::logMessage);
    QSignalSpy stateSpy(&controller, &ClientController::stateChanged);

    controller.disconnectFromHost();

    QCOMPARE(controller.socketState(), QAbstractSocket::UnconnectedState);
    QVERIFY(!controller.isPeriodicModeActive());
    QCOMPARE(controller.activePeriodicMode(), SessionMode::Single);
    QCOMPARE(logSpy.count(), 0);
    QCOMPARE(stateSpy.count(), 1);
}

//--------------------------------------------------------------------------

void TestClientController::connectToHost_andDisconnectFromHost_workWithLocalServer()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    ClientController controller;

    QSignalSpy logSpy(&controller, &ClientController::logMessage);
    QSignalSpy stateSpy(&controller, &ClientController::stateChanged);

    controller.connectToHost(QHostAddress::LocalHost, server.serverPort());

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);

    QTcpSocket *serverSideSocket = server.nextPendingConnection();
    QVERIFY(serverSideSocket != nullptr);

    QTRY_COMPARE_WITH_TIMEOUT(controller.socketState(), QAbstractSocket::ConnectedState, 3000);
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Попытка подключения")));
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Подключение к серверу установлено")));
    QVERIFY(stateSpy.count() >= 2);

    controller.disconnectFromHost();

    QTRY_COMPARE_WITH_TIMEOUT(controller.socketState(), QAbstractSocket::UnconnectedState, 3000);
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Запрошено отключение от сервера")));
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Соединение с сервером закрыто")));

    serverSideSocket->deleteLater();
}

//--------------------------------------------------------------------------

void TestClientController::sendSinglePacket_whenConnected_sendsSerializedPacket()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    ClientController controller;
    QSignalSpy logSpy(&controller, &ClientController::logMessage);

    controller.connectToHost(QHostAddress::LocalHost, server.serverPort());

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);

    QTcpSocket *serverSideSocket = server.nextPendingConnection();
    QVERIFY(serverSideSocket != nullptr);

    QTRY_COMPARE_WITH_TIMEOUT(controller.socketState(), QAbstractSocket::ConnectedState, 3000);

    QByteArray rawBuffer;
    quint32 pendingBlockSize = 0;
    QVector<RequestPacket> packets;

    attachRequestCollector(serverSideSocket, rawBuffer, pendingBlockSize, packets);

    QVERIFY(controller.sendSinglePacket(QStringLiteral("hello packet")));

    QTRY_VERIFY_WITH_TIMEOUT(!packets.isEmpty(), 3000);

    QCOMPARE(packets.first().number, static_cast<quint32>(1));
    QCOMPARE(packets.first().text, QStringLiteral("hello packet"));
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Отправлен пакет: number=1, text=\"hello packet\"")));

    serverSideSocket->deleteLater();
}

//--------------------------------------------------------------------------

void TestClientController::startLongMode_whenConnected_startsTimerAndSendsMultiplePackets()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    ClientController controller;
    QSignalSpy logSpy(&controller, &ClientController::logMessage);

    controller.connectToHost(QHostAddress::LocalHost, server.serverPort());

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);

    QTcpSocket *serverSideSocket = server.nextPendingConnection();
    QVERIFY(serverSideSocket != nullptr);

    QTRY_COMPARE_WITH_TIMEOUT(controller.socketState(), QAbstractSocket::ConnectedState, 3000);

    QByteArray rawBuffer;
    quint32 pendingBlockSize = 0;
    QVector<RequestPacket> packets;

    attachRequestCollector(serverSideSocket, rawBuffer, pendingBlockSize, packets);

    QVERIFY(controller.startLongMode(QStringLiteral("tick"), 100));

    QVERIFY(controller.isPeriodicModeActive());
    QCOMPARE(controller.activePeriodicMode(), SessionMode::Long);
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Запущен режим 2")));

    QTRY_VERIFY_WITH_TIMEOUT(packets.size() >= 2, 3000);

    controller.stopPeriodicMode(QStringLiteral("Тестовая остановка режима 2"));

    QVERIFY(!controller.isPeriodicModeActive());
    QCOMPARE(controller.activePeriodicMode(), SessionMode::Single);
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Тестовая остановка режима 2")));

    serverSideSocket->deleteLater();
}

//--------------------------------------------------------------------------

void TestClientController::startShortMode_startsTimerAndAttemptsConnectionCycle()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QVector<QTcpSocket *> acceptedSockets;
    QByteArray rawBuffer;
    quint32 pendingBlockSize = 0;
    QVector<RequestPacket> packets;

    QObject::connect(&server, &QTcpServer::newConnection, [&server, &acceptedSockets, &rawBuffer, &pendingBlockSize, &packets]() {
        while (server.hasPendingConnections()) {
            QTcpSocket *socket = server.nextPendingConnection();
            acceptedSockets.push_back(socket);
            TestClientController::attachRequestCollector(socket, rawBuffer, pendingBlockSize, packets);
        }
    });

    ClientController controller;
    QSignalSpy logSpy(&controller, &ClientController::logMessage);

    QVERIFY(controller.startShortMode(QStringLiteral("short mode"), 200, QHostAddress::LocalHost, server.serverPort()));

    QVERIFY(controller.isPeriodicModeActive());
    QCOMPARE(controller.activePeriodicMode(), SessionMode::Short);
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Запущен режим 3")));
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Режим 3: попытка подключения")));

    QTRY_VERIFY_WITH_TIMEOUT(!acceptedSockets.isEmpty(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(!packets.isEmpty(), 3000);

    QCOMPARE(packets.first().number, static_cast<quint32>(1));
    QCOMPARE(packets.first().text, QStringLiteral("short mode"));

    controller.stopPeriodicMode(QStringLiteral("Тестовая остановка режима 3"));
    QVERIFY(!controller.isPeriodicModeActive());
    QCOMPARE(controller.activePeriodicMode(), SessionMode::Single);

    controller.disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(controller.socketState(), QAbstractSocket::UnconnectedState, 3000);
}

//--------------------------------------------------------------------------

void TestClientController::shutdown_stopsPeriodicModeAndDisconnectsSocket()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    ClientController controller;

    controller.connectToHost(QHostAddress::LocalHost, server.serverPort());

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);

    QTcpSocket *serverSideSocket = server.nextPendingConnection();
    QVERIFY(serverSideSocket != nullptr);

    QTRY_COMPARE_WITH_TIMEOUT(controller.socketState(), QAbstractSocket::ConnectedState, 3000);

    QVERIFY(controller.startLongMode(QStringLiteral("shutdown test"), 100));
    QVERIFY(controller.isPeriodicModeActive());
    QCOMPARE(controller.activePeriodicMode(), SessionMode::Long);

    controller.shutdown();

    QVERIFY(!controller.isPeriodicModeActive());
    QCOMPARE(controller.activePeriodicMode(), SessionMode::Single);
    QTRY_COMPARE_WITH_TIMEOUT(controller.socketState(), QAbstractSocket::UnconnectedState, 3000);

    serverSideSocket->deleteLater();
}

QTEST_GUILESS_MAIN(TestClientController)

#include "test_clientcontroller.moc"
