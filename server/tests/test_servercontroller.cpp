#include <QtTest>

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

#include "packetprotocol.h"
#include "servercontroller.h"

/*!
 * \class TestServerController
 * \brief Набор unit/integration-тестов для ServerController.
 *
 * \details
 * Тесты проверяют:
 * - начальное состояние контроллера;
 * - запуск и остановку прослушивания;
 * - ошибку запуска на занятом порту;
 * - подключение одного клиента и отклонение второго;
 * - приём request-пакета и отправку response-пакета;
 * - корректный shutdown().
 */
class TestServerController : public QObject
{
    Q_OBJECT

private:
    /*!
     * \brief Возвращает свободный локальный TCP-порт.
     * \return Свободный TCP-порт или 0 при ошибке.
     */
    quint16 acquireFreePort() const;

    /*!
     * \brief Проверяет наличие подстроки среди сообщений QSignalSpy.
     * \param spy Spy для сигнала logMessage(QString).
     * \param expectedSubstring Ожидаемая подстрока.
     * \return true, если подходящее сообщение найдено.
     */
    static bool spyContainsMessage(const QSignalSpy &spy, const QString &expectedSubstring);

private slots:
    /*!
     * \brief Проверяет начальное состояние контроллера.
     */
    void constructor_setsExpectedInitialState();

    /*!
     * \brief Проверяет успешный запуск и остановку прослушивания.
     */
    void startListening_andStopListening_updateStateAndLog();

    /*!
     * \brief Проверяет ошибку запуска на занятом порту.
     */
    void startListening_returnsFalseWhenPortIsBusy();

    /*!
     * \brief Проверяет подключение одного клиента и отклонение второго.
     */
    void acceptsSingleClientAndRejectsSecondClient();

    /*!
     * \brief Проверяет обработку request-пакета и отправку response-пакета.
     */
    void receivesRequestAndSendsResponsePacket();

    /*!
     * \brief Проверяет, что stopListening() отключает активного клиента.
     */
    void stopListening_disconnectsActiveClient();

    /*!
     * \brief Проверяет корректное завершение контроллера через shutdown().
     */
    void shutdown_closesServerAndDisconnectsClient();
};

//--------------------------------------------------------------------------

quint16 TestServerController::acquireFreePort() const
{
    QTcpServer probeServer;

    if (!probeServer.listen(QHostAddress::LocalHost, 0)) {
        return 0;
    }

    const quint16 port = probeServer.serverPort();
    probeServer.close();

    return port;
}

//--------------------------------------------------------------------------

bool TestServerController::spyContainsMessage(const QSignalSpy &spy, const QString &expectedSubstring)
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

void TestServerController::constructor_setsExpectedInitialState()
{
    ServerController controller;

    QVERIFY(!controller.isListening());
    QVERIFY(!controller.hasActiveClient());
    QVERIFY(controller.lastErrorString().isEmpty());
}

//--------------------------------------------------------------------------

void TestServerController::startListening_andStopListening_updateStateAndLog()
{
    const quint16 port = acquireFreePort();
    QVERIFY(port != 0);

    ServerController controller;
    QSignalSpy logSpy(&controller, &ServerController::logMessage);
    QSignalSpy stateSpy(&controller, &ServerController::stateChanged);

    QVERIFY(controller.startListening(QHostAddress::LocalHost, port));
    QVERIFY(controller.isListening());
    QVERIFY(!controller.hasActiveClient());
    QVERIFY(controller.lastErrorString().isEmpty());

    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Сервер начал прослушивание")));
    QVERIFY(stateSpy.count() >= 1);

    controller.stopListening();

    QVERIFY(!controller.isListening());
    QVERIFY(!controller.hasActiveClient());
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Прослушивание остановлено")));
}

//--------------------------------------------------------------------------

void TestServerController::startListening_returnsFalseWhenPortIsBusy()
{
    const quint16 port = acquireFreePort();
    QVERIFY(port != 0);

    QTcpServer occupiedServer;
    QVERIFY(occupiedServer.listen(QHostAddress::LocalHost, port));

    ServerController controller;

    QVERIFY(!controller.startListening(QHostAddress::LocalHost, port));
    QVERIFY(!controller.isListening());
    QVERIFY(!controller.lastErrorString().isEmpty());
}

//--------------------------------------------------------------------------

void TestServerController::acceptsSingleClientAndRejectsSecondClient()
{
    const quint16 port = acquireFreePort();
    QVERIFY(port != 0);

    ServerController controller;
    QSignalSpy logSpy(&controller, &ServerController::logMessage);

    QVERIFY(controller.startListening(QHostAddress::LocalHost, port));

    QTcpSocket firstClient;
    firstClient.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(firstClient.waitForConnected(3000));

    QTRY_VERIFY_WITH_TIMEOUT(controller.hasActiveClient(), 3000);
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Клиент подключился")));

    QTcpSocket secondClient;
    secondClient.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(secondClient.waitForConnected(3000));

    QTRY_COMPARE_WITH_TIMEOUT(secondClient.state(), QAbstractSocket::UnconnectedState, 3000);
    QVERIFY(controller.hasActiveClient());
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Дополнительное подключение")));

    firstClient.disconnectFromHost();
    QTRY_VERIFY_WITH_TIMEOUT(!controller.hasActiveClient(), 3000);
}

//--------------------------------------------------------------------------

void TestServerController::receivesRequestAndSendsResponsePacket()
{
    const quint16 port = acquireFreePort();
    QVERIFY(port != 0);

    ServerController controller;
    QSignalSpy logSpy(&controller, &ServerController::logMessage);

    QVERIFY(controller.startListening(QHostAddress::LocalHost, port));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client.waitForConnected(3000));

    QTRY_VERIFY_WITH_TIMEOUT(controller.hasActiveClient(), 3000);

    const RequestPacket requestPacket { 7u, QStringLiteral("ping") };
    const QByteArray requestFrame = PacketProtocol::buildRequestFrame(requestPacket);

    QCOMPARE(client.write(requestFrame), static_cast<qint64>(requestFrame.size()));
    QVERIFY(client.flush());

    QTRY_VERIFY_WITH_TIMEOUT(
        spyContainsMessage(logSpy, QStringLiteral("Получен пакет")),
        3000
    );

    QTRY_VERIFY_WITH_TIMEOUT(
        spyContainsMessage(logSpy, QStringLiteral("Отправлен пакет-ответ")),
        3000
    );

    QByteArray rawBuffer;
    quint32 pendingBlockSize = 0;
    QByteArray payload;

    const QDeadlineTimer deadline(3000);
    bool frameExtracted = false;

    while (!deadline.hasExpired()) {
        if (client.bytesAvailable() > 0) {
            rawBuffer.append(client.readAll());
        }

        if (PacketProtocol::tryExtractFrame(rawBuffer, pendingBlockSize, payload)) {
            frameExtracted = true;
            break;
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QTest::qWait(10);
    }

    QVERIFY(frameExtracted);
    QCOMPARE(pendingBlockSize, 0u);

    ResponsePacket responsePacket;
    QVERIFY(PacketProtocol::parseResponsePayload(payload, responsePacket));

    QCOMPARE(responsePacket.number, requestPacket.number);
    QCOMPARE(responsePacket.text, requestPacket.text);
    QVERIFY(responsePacket.serverTime.isValid());
}

//--------------------------------------------------------------------------

void TestServerController::stopListening_disconnectsActiveClient()
{
    const quint16 port = acquireFreePort();
    QVERIFY(port != 0);

    ServerController controller;
    QSignalSpy logSpy(&controller, &ServerController::logMessage);

    QVERIFY(controller.startListening(QHostAddress::LocalHost, port));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client.waitForConnected(3000));

    QTRY_VERIFY_WITH_TIMEOUT(controller.hasActiveClient(), 3000);

    controller.stopListening();

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isListening(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.hasActiveClient(), 3000);
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::UnconnectedState, 3000);

    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Активный клиент будет отключён")));
    QVERIFY(spyContainsMessage(logSpy, QStringLiteral("Прослушивание остановлено")));
}

//--------------------------------------------------------------------------

void TestServerController::shutdown_closesServerAndDisconnectsClient()
{
    const quint16 port = acquireFreePort();
    QVERIFY(port != 0);

    ServerController controller;

    QVERIFY(controller.startListening(QHostAddress::LocalHost, port));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client.waitForConnected(3000));

    QTRY_VERIFY_WITH_TIMEOUT(controller.hasActiveClient(), 3000);

    controller.shutdown();

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isListening(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.hasActiveClient(), 3000);
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::UnconnectedState, 3000);
    QVERIFY(controller.lastErrorString().isEmpty());
}

QTEST_GUILESS_MAIN(TestServerController)

#include "test_servercontroller.moc"
