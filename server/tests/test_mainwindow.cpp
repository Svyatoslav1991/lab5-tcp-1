#include <QtTest>

#include <QCoreApplication>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "mainwindow.h"

/*!
 * \class TestMainWindow
 * \brief Набор widget/integration-тестов для MainWindow сервера.
 *
 * \details
 * Тесты покрывают:
 * - стартовое состояние UI;
 * - применение сохранённых настроек;
 * - сохранение настроек при закрытии окна;
 * - запуск и остановку прослушивания через кнопки;
 * - отображение подключения клиента в логе.
 */
class TestMainWindow : public QObject
{
    Q_OBJECT

private:
    /*!
     * \brief Временная директория для изолированных ini-настроек тестов.
     */
    QTemporaryDir settingsDir_;

    /*!
     * \brief Ищет дочерний виджет по objectName и проверяет, что он найден.
     * \tparam T Тип искомого виджета.
     * \param window Окно, внутри которого идёт поиск.
     * \param objectName ObjectName искомого виджета.
     * \return Указатель на найденный виджет.
     */
    template <typename T>
    T *findRequiredChild(MainWindow &window, const char *objectName) const
    {
        T *widget = window.findChild<T *>(QString::fromLatin1(objectName));

        if (widget == nullptr) {
            QTest::qFail(objectName, __FILE__, __LINE__);
            return nullptr;
        }

        return widget;
    }

    /*!
     * \brief Очищает сохранённые настройки тестового приложения.
     */
    void clearSettings() const;

private slots:
    /*!
     * \brief Подготавливает окружение QSettings для тестов.
     */
    void initTestCase();

    /*!
     * \brief Очищает настройки перед каждым тестом.
     */
    void init();

    /*!
     * \brief Проверяет стартовое состояние интерфейса при пустых настройках.
     */
    void constructor_initializesDefaultUiState();

    /*!
     * \brief Проверяет применение сохранённых настроек при создании окна.
     */
    void loadSettings_appliesSavedValues();

    /*!
     * \brief Проверяет сохранение настроек в closeEvent().
     */
    void closeEvent_savesCurrentSettings();

    /*!
     * \brief Проверяет запуск и остановку прослушивания через кнопки UI.
     */
    void startAndStopListening_buttonsUpdateUiAndStatus();

    /*!
     * \brief Проверяет появление записи в логе при подключении клиента.
     */
    void clientConnect_updatesLog();
};

//--------------------------------------------------------------------------

void TestMainWindow::clearSettings() const
{
    QSettings settings;
    settings.clear();
    settings.sync();
}

//--------------------------------------------------------------------------

void TestMainWindow::initTestCase()
{
    QVERIFY2(settingsDir_.isValid(), "Не удалось создать временную директорию для настроек");

    QCoreApplication::setOrganizationName(QStringLiteral("TestOrg"));
    QCoreApplication::setApplicationName(QStringLiteral("TestServerMainWindow"));

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir_.path());
}

//--------------------------------------------------------------------------

void TestMainWindow::init()
{
    clearSettings();
}

//--------------------------------------------------------------------------

void TestMainWindow::constructor_initializesDefaultUiState()
{
    MainWindow window;

    auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
    auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");
    auto *serverLogPlainTextEdit = findRequiredChild<QPlainTextEdit>(window, "serverLog_plainTextEdit");
    auto *startListeningButton = findRequiredChild<QPushButton>(window, "startListening_button");
    auto *stopListeningButton = findRequiredChild<QPushButton>(window, "stopListening_button");

    QCOMPARE(addressLineEdit->text(), QStringLiteral("127.0.0.1"));
    QCOMPARE(portLineEdit->text(), QStringLiteral("8888"));

    QVERIFY(serverLogPlainTextEdit->isReadOnly());

    QVERIFY(startListeningButton->isEnabled());
    QVERIFY(!stopListeningButton->isEnabled());

    QVERIFY(addressLineEdit->isEnabled());
    QVERIFY(portLineEdit->isEnabled());

    QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Сервер не запущен"));
}

//--------------------------------------------------------------------------

void TestMainWindow::loadSettings_appliesSavedValues()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("server"));
    settings.setValue(QStringLiteral("address"), QStringLiteral("10.20.30.40"));
    settings.setValue(QStringLiteral("port"), QStringLiteral("55000"));
    settings.endGroup();
    settings.sync();

    MainWindow window;

    auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
    auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");

    QCOMPARE(addressLineEdit->text(), QStringLiteral("10.20.30.40"));
    QCOMPARE(portLineEdit->text(), QStringLiteral("55000"));
}

//--------------------------------------------------------------------------

void TestMainWindow::closeEvent_savesCurrentSettings()
{
    {
        MainWindow window;

        auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
        auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");

        window.show();

        addressLineEdit->setText(QStringLiteral("192.168.1.77"));
        portLineEdit->setText(QStringLiteral("45678"));

        window.close();
        QCoreApplication::processEvents();
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("server"));

    QCOMPARE(settings.value(QStringLiteral("address")).toString(), QStringLiteral("192.168.1.77"));
    QCOMPARE(settings.value(QStringLiteral("port")).toString(), QStringLiteral("45678"));

    settings.endGroup();
}

//--------------------------------------------------------------------------

void TestMainWindow::startAndStopListening_buttonsUpdateUiAndStatus()
{
    MainWindow window;

    auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
    auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");
    auto *startListeningButton = findRequiredChild<QPushButton>(window, "startListening_button");
    auto *stopListeningButton = findRequiredChild<QPushButton>(window, "stopListening_button");
    auto *serverLogPlainTextEdit = findRequiredChild<QPlainTextEdit>(window, "serverLog_plainTextEdit");

    QTcpServer probeServer;
    QVERIFY(probeServer.listen(QHostAddress::LocalHost, 0));
    const quint16 freePort = probeServer.serverPort();
    probeServer.close();

    addressLineEdit->setText(QStringLiteral("127.0.0.1"));
    portLineEdit->setText(QString::number(freePort));

    window.show();

    QTest::mouseClick(startListeningButton, Qt::LeftButton);

    QTRY_VERIFY_WITH_TIMEOUT(!startListeningButton->isEnabled(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(stopListeningButton->isEnabled(), 3000);

    QVERIFY(!addressLineEdit->isEnabled());
    QVERIFY(!portLineEdit->isEnabled());

    QTRY_COMPARE_WITH_TIMEOUT(window.statusBar()->currentMessage(),
                              QStringLiteral("Сервер прослушивает порт"),
                              3000);

    QVERIFY(serverLogPlainTextEdit->toPlainText().contains(QStringLiteral("Сервер начал прослушивание")));

    QTest::mouseClick(stopListeningButton, Qt::LeftButton);

    QTRY_VERIFY_WITH_TIMEOUT(startListeningButton->isEnabled(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(!stopListeningButton->isEnabled(), 3000);

    QVERIFY(addressLineEdit->isEnabled());
    QVERIFY(portLineEdit->isEnabled());

    QTRY_COMPARE_WITH_TIMEOUT(window.statusBar()->currentMessage(),
                              QStringLiteral("Сервер не запущен"),
                              3000);

    QVERIFY(serverLogPlainTextEdit->toPlainText().contains(QStringLiteral("Прослушивание остановлено")));
}

//--------------------------------------------------------------------------

void TestMainWindow::clientConnect_updatesLog()
{
    MainWindow window;

    auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
    auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");
    auto *startListeningButton = findRequiredChild<QPushButton>(window, "startListening_button");
    auto *serverLogPlainTextEdit = findRequiredChild<QPlainTextEdit>(window, "serverLog_plainTextEdit");

    QTcpServer probeServer;
    QVERIFY(probeServer.listen(QHostAddress::LocalHost, 0));
    const quint16 freePort = probeServer.serverPort();
    probeServer.close();

    addressLineEdit->setText(QStringLiteral("127.0.0.1"));
    portLineEdit->setText(QString::number(freePort));

    window.show();

    QTest::mouseClick(startListeningButton, Qt::LeftButton);

    QTRY_COMPARE_WITH_TIMEOUT(window.statusBar()->currentMessage(),
                              QStringLiteral("Сервер прослушивает порт"),
                              3000);

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, freePort);
    QVERIFY(client.waitForConnected(3000));

    QTRY_VERIFY_WITH_TIMEOUT(
        serverLogPlainTextEdit->toPlainText().contains(QStringLiteral("Клиент подключился")),
        3000
    );

    client.disconnectFromHost();

    QTRY_COMPARE_WITH_TIMEOUT(client.state(),
                              QAbstractSocket::UnconnectedState,
                              3000);
}

QTEST_MAIN(TestMainWindow)

#include "test_mainwindow.moc"
