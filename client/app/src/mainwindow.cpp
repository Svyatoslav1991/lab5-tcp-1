#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStatusBar>

namespace
{
const QString kDefaultAddress = QStringLiteral("127.0.0.1");
const QString kDefaultPort = QStringLiteral("8888");
const QString kDefaultTimeout = QStringLiteral("1000");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , clientController_(new ClientController(this))
{
    ui->setupUi(this);
    initializeUi();
    connectSignals();
}

//--------------------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}

//--------------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    clientController_->shutdown();

    QMainWindow::closeEvent(event);
}

//--------------------------------------------------------------------------

void MainWindow::initializeUi()
{
    const QString ipv4BytePattern =
        QStringLiteral("(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)");

    const QString ipv4AddressPattern =
        QStringLiteral("^%1\\.%1\\.%1\\.%1$").arg(ipv4BytePattern);

    const QString portPattern =
        QStringLiteral("^(6553[0-5]|655[0-2]\\d|65[0-4]\\d{2}|6[0-4]\\d{3}|[1-5]\\d{4}|[1-9]\\d{0,3})$");

    const QString timeoutPattern =
        QStringLiteral("^[1-9]\\d{0,5}$");

    auto *addressValidator = new QRegularExpressionValidator(
        QRegularExpression(ipv4AddressPattern),
        this
    );

    auto *portValidator = new QRegularExpressionValidator(
        QRegularExpression(portPattern),
        this
    );

    auto *timeoutValidator = new QRegularExpressionValidator(
        QRegularExpression(timeoutPattern),
        this
    );

    ui->address_lineEdit->setValidator(addressValidator);
    ui->port_lineEdit->setValidator(portValidator);
    ui->timeout_lineEdit->setValidator(timeoutValidator);

    ui->address_lineEdit->setText(kDefaultAddress);
    ui->port_lineEdit->setText(kDefaultPort);
    ui->timeout_lineEdit->setText(kDefaultTimeout);

    ui->clientLog_plainTextEdit->setReadOnly(true);
    ui->single_radioButton->setChecked(true);

    loadSettings();
    updateConnectionControls();

    statusBar()->showMessage(QStringLiteral("Клиент не подключен"));
}

//--------------------------------------------------------------------------

void MainWindow::connectSignals()
{
    connect(ui->address_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->port_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->timeout_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->single_radioButton, &QRadioButton::toggled,
            this, &MainWindow::saveSettings);

    connect(ui->long_radioButton, &QRadioButton::toggled,
            this, &MainWindow::saveSettings);

    connect(ui->short_radioButton, &QRadioButton::toggled,
            this, &MainWindow::saveSettings);

    connect(ui->single_radioButton, &QRadioButton::toggled,
            this, &MainWindow::updateConnectionControls);

    connect(ui->long_radioButton, &QRadioButton::toggled,
            this, &MainWindow::updateConnectionControls);

    connect(ui->short_radioButton, &QRadioButton::toggled,
            this, &MainWindow::updateConnectionControls);

    connect(ui->connect_pushButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);

    connect(ui->disconnect_pushButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectClicked);

    connect(ui->write_pushButton, &QPushButton::clicked,
            this, &MainWindow::onWriteClicked);

    connect(ui->stop_pushButton, &QPushButton::clicked,
            this, &MainWindow::onStopClicked);

    connect(ui->message_lineEdit, &QLineEdit::returnPressed,
            this, &MainWindow::onWriteClicked);

    connect(clientController_, &ClientController::logMessage,
            this, &MainWindow::appendLog);

    connect(clientController_, &ClientController::stateChanged,
            this, &MainWindow::updateConnectionControls);
}

//--------------------------------------------------------------------------

void MainWindow::loadSettings()
{
    applySettingsData(clientSettings_.load());

    if (!ui->address_lineEdit->hasAcceptableInput()) {
        ui->address_lineEdit->setText(kDefaultAddress);
    }

    if (!ui->port_lineEdit->hasAcceptableInput()) {
        ui->port_lineEdit->setText(kDefaultPort);
    }

    if (!ui->timeout_lineEdit->hasAcceptableInput()) {
        ui->timeout_lineEdit->setText(kDefaultTimeout);
    }
}

//--------------------------------------------------------------------------

void MainWindow::saveSettings()
{
    clientSettings_.save(buildSettingsData());
}

//--------------------------------------------------------------------------

ClientSettingsData MainWindow::buildSettingsData() const
{
    ClientSettingsData data;
    data.address = ui->address_lineEdit->text().trimmed();
    data.port = ui->port_lineEdit->text().trimmed();
    data.timeout = ui->timeout_lineEdit->text().trimmed();
    data.sessionMode = currentSessionMode();

    return data;
}

//--------------------------------------------------------------------------

void MainWindow::applySettingsData(const ClientSettingsData &data)
{
    ui->address_lineEdit->setText(data.address);
    ui->port_lineEdit->setText(data.port);
    ui->timeout_lineEdit->setText(data.timeout);
    setSessionMode(data.sessionMode);
}

//--------------------------------------------------------------------------

void MainWindow::appendLog(const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    ui->clientLog_plainTextEdit->appendPlainText(
        QStringLiteral("[%1] %2").arg(timestamp, message)
    );
}

//--------------------------------------------------------------------------

SessionMode MainWindow::currentSessionMode() const
{
    if (ui->long_radioButton->isChecked()) {
        return SessionMode::Long;
    }

    if (ui->short_radioButton->isChecked()) {
        return SessionMode::Short;
    }

    return SessionMode::Single;
}

//--------------------------------------------------------------------------

void MainWindow::setSessionMode(SessionMode mode)
{
    switch (mode) {
    case SessionMode::Single:
        ui->single_radioButton->setChecked(true);
        break;
    case SessionMode::Long:
        ui->long_radioButton->setChecked(true);
        break;
    case SessionMode::Short:
        ui->short_radioButton->setChecked(true);
        break;
    }
}

//--------------------------------------------------------------------------

void MainWindow::updateConnectionControls()
{
    const QAbstractSocket::SocketState socketState = clientController_->socketState();
    const SessionMode selectedMode = currentSessionMode();
    const SessionMode periodicMode = clientController_->activePeriodicMode();

    const bool isConnected = socketState == QAbstractSocket::ConnectedState;
    const bool isUnconnected = socketState == QAbstractSocket::UnconnectedState;
    const bool isConnecting = socketState == QAbstractSocket::ConnectingState;
    const bool isClosing = socketState == QAbstractSocket::ClosingState;
    const bool isPeriodicModeRunning = clientController_->isPeriodicModeActive();

    ui->connect_pushButton->setEnabled(
        selectedMode != SessionMode::Short &&
        isUnconnected &&
        !isPeriodicModeRunning
    );

    ui->disconnect_pushButton->setEnabled(!isUnconnected);

    ui->write_pushButton->setEnabled(
        !isPeriodicModeRunning &&
        ((selectedMode == SessionMode::Short && isUnconnected) ||
         (selectedMode != SessionMode::Short && isConnected))
    );

    ui->stop_pushButton->setEnabled(isPeriodicModeRunning);

    ui->message_lineEdit->setEnabled(!isPeriodicModeRunning);

    ui->address_lineEdit->setEnabled(isUnconnected && !isPeriodicModeRunning);
    ui->port_lineEdit->setEnabled(isUnconnected && !isPeriodicModeRunning);

    ui->single_radioButton->setEnabled(isUnconnected && !isPeriodicModeRunning);
    ui->long_radioButton->setEnabled(isUnconnected && !isPeriodicModeRunning);
    ui->short_radioButton->setEnabled(isUnconnected && !isPeriodicModeRunning);

    ui->timeout_lineEdit->setEnabled(
        isUnconnected &&
        !isPeriodicModeRunning &&
        selectedMode != SessionMode::Single
    );

    if (isPeriodicModeRunning && periodicMode == SessionMode::Long) {
        statusBar()->showMessage(QStringLiteral("Режим 2: периодическая отправка активна"));
        return;
    }

    if (isPeriodicModeRunning && periodicMode == SessionMode::Short) {
        statusBar()->showMessage(QStringLiteral("Режим 3: периодические подключения активны"));
        return;
    }

    if (isConnected) {
        statusBar()->showMessage(QStringLiteral("Клиент подключен к серверу"));
        return;
    }

    if (isConnecting) {
        statusBar()->showMessage(QStringLiteral("Подключение к серверу..."));
        return;
    }

    if (isClosing) {
        statusBar()->showMessage(QStringLiteral("Отключение от сервера..."));
        return;
    }

    statusBar()->showMessage(QStringLiteral("Клиент не подключен"));
}

//--------------------------------------------------------------------------

bool MainWindow::tryGetConnectionParameters(QHostAddress &address, quint16 &port) const
{
    if (!ui->address_lineEdit->hasAcceptableInput() ||
        !ui->port_lineEdit->hasAcceptableInput()) {
        return false;
    }

    const QString addressText = ui->address_lineEdit->text().trimmed();
    const QString portText = ui->port_lineEdit->text().trimmed();

    if (!address.setAddress(addressText)) {
        return false;
    }

    bool isOk = false;
    const uint parsedPort = portText.toUInt(&isOk);

    if (!isOk || parsedPort == 0 || parsedPort > 65535) {
        return false;
    }

    port = static_cast<quint16>(parsedPort);
    return true;
}

//--------------------------------------------------------------------------

bool MainWindow::tryGetTimeout(int &timeoutMs) const
{
    if (!ui->timeout_lineEdit->hasAcceptableInput()) {
        return false;
    }

    bool isOk = false;
    const int parsedTimeout = ui->timeout_lineEdit->text().trimmed().toInt(&isOk);

    if (!isOk || parsedTimeout <= 0) {
        return false;
    }

    timeoutMs = parsedTimeout;
    return true;
}

//--------------------------------------------------------------------------

void MainWindow::onConnectClicked()
{
    QHostAddress address;
    quint16 port = 0;

    if (!tryGetConnectionParameters(address, port)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Некорректные параметры"),
            QStringLiteral("Введите корректные адрес и порт сервера.")
        );
        return;
    }

    saveSettings();
    clientController_->connectToHost(address, port);
}

//--------------------------------------------------------------------------

void MainWindow::onDisconnectClicked()
{
    clientController_->disconnectFromHost();
}

//--------------------------------------------------------------------------

void MainWindow::onWriteClicked()
{
    const QString text = ui->message_lineEdit->text().trimmed();

    if (text.isEmpty()) {
        QMessageBox::information(
            this,
            QStringLiteral("Пустое сообщение"),
            QStringLiteral("Введите строку для отправки.")
        );
        return;
    }

    switch (currentSessionMode()) {
    case SessionMode::Single:
        if (clientController_->socketState() != QAbstractSocket::ConnectedState) {
            return;
        }

        if (clientController_->sendSinglePacket(text)) {
            ui->message_lineEdit->clear();
        }
        break;

    case SessionMode::Long:
    {
        if (clientController_->socketState() != QAbstractSocket::ConnectedState) {
            return;
        }

        int timeoutMs = 0;

        if (!tryGetTimeout(timeoutMs)) {
            QMessageBox::warning(
                this,
                QStringLiteral("Некорректный timeout"),
                QStringLiteral("Введите корректный timeout в миллисекундах.")
            );
            return;
        }

        saveSettings();
        clientController_->startLongMode(text, timeoutMs);
        break;
    }

    case SessionMode::Short:
    {
        int timeoutMs = 0;

        if (!tryGetTimeout(timeoutMs)) {
            QMessageBox::warning(
                this,
                QStringLiteral("Некорректный timeout"),
                QStringLiteral("Введите корректный timeout в миллисекундах.")
            );
            return;
        }

        QHostAddress address;
        quint16 port = 0;

        if (!tryGetConnectionParameters(address, port)) {
            QMessageBox::warning(
                this,
                QStringLiteral("Некорректные параметры"),
                QStringLiteral("Введите корректные адрес и порт сервера.")
            );
            return;
        }

        saveSettings();
        clientController_->startShortMode(text, timeoutMs, address, port);
        break;
    }
    }
}

//--------------------------------------------------------------------------

void MainWindow::onStopClicked()
{
    if (!clientController_->isPeriodicModeActive()) {
        return;
    }

    if (clientController_->activePeriodicMode() == SessionMode::Long) {
        clientController_->stopPeriodicMode(QStringLiteral("Режим 2 остановлен"));
        return;
    }

    if (clientController_->activePeriodicMode() == SessionMode::Short) {
        clientController_->stopPeriodicMode(QStringLiteral("Режим 3 остановлен"));

        if (clientController_->socketState() != QAbstractSocket::UnconnectedState) {
            appendLog(QStringLiteral("Режим 3: текущий цикл прерывается отключением"));
            clientController_->disconnectFromHost();
        }

        return;
    }
}
