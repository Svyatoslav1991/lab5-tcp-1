#include <QtTest>

#include <QCoreApplication>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QStatusBar>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "mainwindow.h"

/*!
 * \class TestMainWindow
 * \brief Набор widget/integration-тестов для MainWindow клиента.
 *
 * \details
 * Тесты покрывают:
 * - стартовое состояние UI;
 * - применение сохранённых настроек;
 * - обновление доступности контролов при выборе short-режима;
 * - сохранение настроек при закрытии окна;
 * - интеграцию Connect/Disconnect с локальным QTcpServer.
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
    void loadSettings_appliesSavedValuesAndMode();

    /*!
     * \brief Проверяет доступность элементов управления при выборе short-режима в unconnected state.
     */
    void selectingShortMode_updatesControlStates();

    /*!
     * \brief Проверяет сохранение настроек в closeEvent().
     */
    void closeEvent_savesCurrentSettings();

    /*!
     * \brief Проверяет интеграцию Connect/Disconnect с локальным TCP-сервером.
     */
    void connectAndDisconnect_withLocalServer_updatesUiAndLog();
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
    QCoreApplication::setApplicationName(QStringLiteral("TestMainWindow"));

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
    auto *timeoutLineEdit = findRequiredChild<QLineEdit>(window, "timeout_lineEdit");
    auto *messageLineEdit = findRequiredChild<QLineEdit>(window, "message_lineEdit");

    auto *clientLogPlainTextEdit = findRequiredChild<QPlainTextEdit>(window, "clientLog_plainTextEdit");

    auto *connectPushButton = findRequiredChild<QPushButton>(window, "connect_pushButton");
    auto *disconnectPushButton = findRequiredChild<QPushButton>(window, "disconnect_pushButton");
    auto *writePushButton = findRequiredChild<QPushButton>(window, "write_pushButton");
    auto *stopPushButton = findRequiredChild<QPushButton>(window, "stop_pushButton");

    auto *singleRadioButton = findRequiredChild<QRadioButton>(window, "single_radioButton");
    auto *longRadioButton = findRequiredChild<QRadioButton>(window, "long_radioButton");
    auto *shortRadioButton = findRequiredChild<QRadioButton>(window, "short_radioButton");

    QCOMPARE(addressLineEdit->text(), QStringLiteral("127.0.0.1"));
    QCOMPARE(portLineEdit->text(), QStringLiteral("8888"));
    QCOMPARE(timeoutLineEdit->text(), QStringLiteral("1000"));

    QVERIFY(clientLogPlainTextEdit->isReadOnly());

    QVERIFY(singleRadioButton->isChecked());
    QVERIFY(!longRadioButton->isChecked());
    QVERIFY(!shortRadioButton->isChecked());

    QVERIFY(connectPushButton->isEnabled());
    QVERIFY(!disconnectPushButton->isEnabled());
    QVERIFY(!writePushButton->isEnabled());
    QVERIFY(!stopPushButton->isEnabled());

    QVERIFY(messageLineEdit->isEnabled());
    QVERIFY(addressLineEdit->isEnabled());
    QVERIFY(portLineEdit->isEnabled());

    QVERIFY(singleRadioButton->isEnabled());
    QVERIFY(longRadioButton->isEnabled());
    QVERIFY(shortRadioButton->isEnabled());

    QVERIFY(!timeoutLineEdit->isEnabled());

    QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Клиент не подключен"));
}

//--------------------------------------------------------------------------

void TestMainWindow::loadSettings_appliesSavedValuesAndMode()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("client"));
    settings.setValue(QStringLiteral("address"), QStringLiteral("10.20.30.40"));
    settings.setValue(QStringLiteral("port"), QStringLiteral("55000"));
    settings.setValue(QStringLiteral("timeout"), QStringLiteral("2500"));
    settings.setValue(QStringLiteral("session_mode"), QStringLiteral("long"));
    settings.endGroup();
    settings.sync();

    MainWindow window;

    auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
    auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");
    auto *timeoutLineEdit = findRequiredChild<QLineEdit>(window, "timeout_lineEdit");

    auto *singleRadioButton = findRequiredChild<QRadioButton>(window, "single_radioButton");
    auto *longRadioButton = findRequiredChild<QRadioButton>(window, "long_radioButton");
    auto *shortRadioButton = findRequiredChild<QRadioButton>(window, "short_radioButton");

    QCOMPARE(addressLineEdit->text(), QStringLiteral("10.20.30.40"));
    QCOMPARE(portLineEdit->text(), QStringLiteral("55000"));
    QCOMPARE(timeoutLineEdit->text(), QStringLiteral("2500"));

    QVERIFY(!singleRadioButton->isChecked());
    QVERIFY(longRadioButton->isChecked());
    QVERIFY(!shortRadioButton->isChecked());

    QVERIFY(timeoutLineEdit->isEnabled());
}

//--------------------------------------------------------------------------

void TestMainWindow::selectingShortMode_updatesControlStates()
{
    MainWindow window;

    auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
    auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");
    auto *timeoutLineEdit = findRequiredChild<QLineEdit>(window, "timeout_lineEdit");
    auto *messageLineEdit = findRequiredChild<QLineEdit>(window, "message_lineEdit");

    auto *connectPushButton = findRequiredChild<QPushButton>(window, "connect_pushButton");
    auto *disconnectPushButton = findRequiredChild<QPushButton>(window, "disconnect_pushButton");
    auto *writePushButton = findRequiredChild<QPushButton>(window, "write_pushButton");
    auto *stopPushButton = findRequiredChild<QPushButton>(window, "stop_pushButton");

    auto *shortRadioButton = findRequiredChild<QRadioButton>(window, "short_radioButton");

    shortRadioButton->setChecked(true);
    QCoreApplication::processEvents();

    QVERIFY(!connectPushButton->isEnabled());
    QVERIFY(!disconnectPushButton->isEnabled());
    QVERIFY(writePushButton->isEnabled());
    QVERIFY(!stopPushButton->isEnabled());

    QVERIFY(messageLineEdit->isEnabled());
    QVERIFY(addressLineEdit->isEnabled());
    QVERIFY(portLineEdit->isEnabled());
    QVERIFY(timeoutLineEdit->isEnabled());

    QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Клиент не подключен"));
}

//--------------------------------------------------------------------------

void TestMainWindow::closeEvent_savesCurrentSettings()
{
    {
        MainWindow window;

        auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
        auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");
        auto *timeoutLineEdit = findRequiredChild<QLineEdit>(window, "timeout_lineEdit");
        auto *shortRadioButton = findRequiredChild<QRadioButton>(window, "short_radioButton");

        window.show();

        addressLineEdit->setText(QStringLiteral("192.168.1.77"));
        portLineEdit->setText(QStringLiteral("45678"));
        timeoutLineEdit->setText(QStringLiteral("3456"));
        shortRadioButton->setChecked(true);

        window.close();
        QCoreApplication::processEvents();
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("client"));

    QCOMPARE(settings.value(QStringLiteral("address")).toString(), QStringLiteral("192.168.1.77"));
    QCOMPARE(settings.value(QStringLiteral("port")).toString(), QStringLiteral("45678"));
    QCOMPARE(settings.value(QStringLiteral("timeout")).toString(), QStringLiteral("3456"));
    QCOMPARE(settings.value(QStringLiteral("session_mode")).toString(), QStringLiteral("short"));

    settings.endGroup();
}

//--------------------------------------------------------------------------

void TestMainWindow::connectAndDisconnect_withLocalServer_updatesUiAndLog()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    MainWindow window;

    auto *addressLineEdit = findRequiredChild<QLineEdit>(window, "address_lineEdit");
    auto *portLineEdit = findRequiredChild<QLineEdit>(window, "port_lineEdit");
    auto *clientLogPlainTextEdit = findRequiredChild<QPlainTextEdit>(window, "clientLog_plainTextEdit");
    auto *connectPushButton = findRequiredChild<QPushButton>(window, "connect_pushButton");
    auto *disconnectPushButton = findRequiredChild<QPushButton>(window, "disconnect_pushButton");

    addressLineEdit->setText(QStringLiteral("127.0.0.1"));
    portLineEdit->setText(QString::number(server.serverPort()));

    window.show();

    QTest::mouseClick(connectPushButton, Qt::LeftButton);

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 3000);

    QTcpSocket *serverSideSocket = server.nextPendingConnection();
    QVERIFY(serverSideSocket != nullptr);

    QTRY_VERIFY_WITH_TIMEOUT(!connectPushButton->isEnabled(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(disconnectPushButton->isEnabled(), 3000);
    QTRY_COMPARE_WITH_TIMEOUT(window.statusBar()->currentMessage(),
                              QStringLiteral("Клиент подключен к серверу"),
                              3000);

    QVERIFY(clientLogPlainTextEdit->toPlainText().contains(QStringLiteral("Попытка подключения")));
    QVERIFY(clientLogPlainTextEdit->toPlainText().contains(QStringLiteral("Подключение к серверу установлено")));

    QTest::mouseClick(disconnectPushButton, Qt::LeftButton);

    QTRY_VERIFY_WITH_TIMEOUT(connectPushButton->isEnabled(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(!disconnectPushButton->isEnabled(), 3000);
    QTRY_COMPARE_WITH_TIMEOUT(window.statusBar()->currentMessage(),
                              QStringLiteral("Клиент не подключен"),
                              3000);

    QVERIFY(clientLogPlainTextEdit->toPlainText().contains(QStringLiteral("Запрошено отключение от сервера")));
    QVERIFY(clientLogPlainTextEdit->toPlainText().contains(QStringLiteral("Соединение с сервером закрыто")));

    serverSideSocket->deleteLater();
}

QTEST_MAIN(TestMainWindow)

#include "test_mainwindow.moc"
