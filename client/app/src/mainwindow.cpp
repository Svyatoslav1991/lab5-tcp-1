#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "packetprotocol.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStatusBar>
#include <QTcpSocket>
#include <QTimer>
#include <QDebug>

namespace
{
const QString kDefaultAddress = QStringLiteral("127.0.0.1");
const QString kDefaultPort = QStringLiteral("8888");
const QString kDefaultTimeout = QStringLiteral("1000");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket_(new QTcpSocket(this))
    , sendTimer_(new QTimer(this))
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
    stopPeriodicMode();

    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
        socket_->close();
    }

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

    sendTimer_->setSingleShot(false);

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

    connect(socket_, &QTcpSocket::connected,
            this, &MainWindow::onSocketConnected);

    connect(socket_, &QTcpSocket::disconnected,
            this, &MainWindow::onSocketDisconnected);

    connect(socket_, &QTcpSocket::readyRead,
            this, &MainWindow::onSocketReadyRead);

    connect(socket_, &QTcpSocket::stateChanged,
            this, &MainWindow::onSocketStateChanged);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket_, &QTcpSocket::errorOccurred,
            this, &MainWindow::onSocketErrorOccurred);
#else
    connect(socket_,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this,
            &MainWindow::onSocketErrorOccurred);
#endif

    connect(sendTimer_, &QTimer::timeout,
            this, &MainWindow::onSendTimerTimeout);
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
    const QAbstractSocket::SocketState socketState = socket_->state();
    const SessionMode mode = currentSessionMode();

    const bool isConnected = socketState == QAbstractSocket::ConnectedState;
    const bool isUnconnected = socketState == QAbstractSocket::UnconnectedState;
    const bool isConnecting = socketState == QAbstractSocket::ConnectingState;
    const bool isClosing = socketState == QAbstractSocket::ClosingState;
    const bool isPeriodicModeRunning = sendTimer_->isActive();

    ui->connect_pushButton->setEnabled(
        mode != SessionMode::Short &&
        isUnconnected &&
        !isPeriodicModeRunning
    );

    ui->disconnect_pushButton->setEnabled(!isUnconnected);

    ui->write_pushButton->setEnabled(
        !isPeriodicModeRunning &&
        ((mode == SessionMode::Short && isUnconnected) ||
         (mode != SessionMode::Short && isConnected))
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
        mode != SessionMode::Single
    );

    if (isPeriodicModeRunning && mode == SessionMode::Long) {
        statusBar()->showMessage(QStringLiteral("Режим 2: периодическая отправка активна"));
        return;
    }

    if (isPeriodicModeRunning && mode == SessionMode::Short) {
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

QString MainWindow::socketStateToString(QAbstractSocket::SocketState socketState) const
{
    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        return QStringLiteral("UnconnectedState");
    case QAbstractSocket::HostLookupState:
        return QStringLiteral("HostLookupState");
    case QAbstractSocket::ConnectingState:
        return QStringLiteral("ConnectingState");
    case QAbstractSocket::ConnectedState:
        return QStringLiteral("ConnectedState");
    case QAbstractSocket::BoundState:
        return QStringLiteral("BoundState");
    case QAbstractSocket::ListeningState:
        return QStringLiteral("ListeningState");
    case QAbstractSocket::ClosingState:
        return QStringLiteral("ClosingState");
    default:
        return QStringLiteral("UnknownState");
    }
}

//--------------------------------------------------------------------------

QString MainWindow::socketErrorToString(QAbstractSocket::SocketError socketError) const
{
    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError:
        return QStringLiteral("ConnectionRefusedError");
    case QAbstractSocket::RemoteHostClosedError:
        return QStringLiteral("RemoteHostClosedError");
    case QAbstractSocket::HostNotFoundError:
        return QStringLiteral("HostNotFoundError");
    case QAbstractSocket::SocketAccessError:
        return QStringLiteral("SocketAccessError");
    case QAbstractSocket::SocketResourceError:
        return QStringLiteral("SocketResourceError");
    case QAbstractSocket::SocketTimeoutError:
        return QStringLiteral("SocketTimeoutError");
    case QAbstractSocket::DatagramTooLargeError:
        return QStringLiteral("DatagramTooLargeError");
    case QAbstractSocket::NetworkError:
        return QStringLiteral("NetworkError");
    case QAbstractSocket::AddressInUseError:
        return QStringLiteral("AddressInUseError");
    case QAbstractSocket::SocketAddressNotAvailableError:
        return QStringLiteral("SocketAddressNotAvailableError");
    case QAbstractSocket::UnsupportedSocketOperationError:
        return QStringLiteral("UnsupportedSocketOperationError");
    default:
        return QStringLiteral("UnknownSocketError");
    }
}

//--------------------------------------------------------------------------

void MainWindow::resetIncomingFrameState()
{
    socketReadBuffer_.clear();
    pendingServerBlockSize_ = 0;
    shortModeWaitingForResponse_ = false;
}

//--------------------------------------------------------------------------

void MainWindow::processSocketBuffer()
{
    bool responseReceived = false;

    while (true) {
        QByteArray payload;

        if (!PacketProtocol::tryExtractFrame(socketReadBuffer_, pendingServerBlockSize_, payload)) {
            break;
        }

        ResponsePacket packet;

        if (!PacketProtocol::parseResponsePayload(payload, packet)) {
            appendLog(QStringLiteral("Не удалось разобрать пакет ответа сервера"));
            continue;
        }

        appendLog(QStringLiteral("Получен пакет-ответ: number=%1, text=\"%2\", time=%3")
                  .arg(packet.number)
                  .arg(packet.text)
                  .arg(packet.serverTime.toString(QStringLiteral("HH:mm:ss"))));

        responseReceived = true;
    }

    if (responseReceived &&
        sendTimer_->isActive() &&
        currentSessionMode() == SessionMode::Short &&
        socket_->state() == QAbstractSocket::ConnectedState) {
        shortModeWaitingForResponse_ = false;
        appendLog(QStringLiteral("Режим 3: ответ получен, выполняется отключение"));
        socket_->disconnectFromHost();
    }
}

//--------------------------------------------------------------------------

bool MainWindow::sendPacket(const QString &text)
{
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        return false;
    }

    const RequestPacket packet { nextRequestNumber_++, text };
    const QByteArray frame = PacketProtocol::buildRequestFrame(packet);

    const qint64 written = socket_->write(frame);

    if (written == -1) {
        appendLog(QStringLiteral("Не удалось отправить пакет: %1").arg(socket_->errorString()));
        return false;
    }

    appendLog(QStringLiteral("Отправлен пакет: number=%1, text=\"%2\"")
              .arg(packet.number)
              .arg(packet.text));

    return true;
}

//--------------------------------------------------------------------------

void MainWindow::startLongMode(int timeoutMs, const QString &text)
{
    if (!sendPacket(text)) {
        return;
    }

    periodicMessageText_ = text;
    sendTimer_->start(timeoutMs);

    appendLog(QStringLiteral("Запущен режим 2: периодическая отправка, timeout=%1 мс")
              .arg(timeoutMs));

    updateConnectionControls();
}

//--------------------------------------------------------------------------

void MainWindow::startShortMode(int timeoutMs, const QString &text)
{
    periodicMessageText_ = text;
    shortModeWaitingForResponse_ = false;
    sendTimer_->start(timeoutMs);

    appendLog(QStringLiteral("Запущен режим 3: периодические подключения каждые %1 мс")
              .arg(timeoutMs));

    startShortModeCycle();
    updateConnectionControls();
}

//--------------------------------------------------------------------------

void MainWindow::stopPeriodicMode(const QString &logMessage)
{
    if (!sendTimer_->isActive()) {
        return;
    }

    sendTimer_->stop();
    shortModeWaitingForResponse_ = false;

    if (!logMessage.isEmpty()) {
        appendLog(logMessage);
    }

    updateConnectionControls();
}

//--------------------------------------------------------------------------

void MainWindow::startShortModeCycle()
{
    if (currentSessionMode() != SessionMode::Short || !sendTimer_->isActive()) {
        return;
    }

    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        qDebug() << "CLIENT: mode3 cycle skipped, socket state =" << socketStateToString(socket_->state());
        return;
    }

    QHostAddress address;
    quint16 port = 0;

    if (!tryGetConnectionParameters(address, port)) {
        stopPeriodicMode(QStringLiteral("Режим 3 остановлен: некорректные параметры подключения"));
        return;
    }

    appendLog(QStringLiteral("Режим 3: попытка подключения к %1:%2")
              .arg(address.toString())
              .arg(port));

    socket_->connectToHost(address, port);
    updateConnectionControls();
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

    appendLog(QStringLiteral("Попытка подключения к %1:%2")
              .arg(address.toString())
              .arg(port));

    socket_->connectToHost(address, port);
    updateConnectionControls();
}

//--------------------------------------------------------------------------

void MainWindow::onDisconnectClicked()
{
    if (sendTimer_->isActive()) {
        switch (currentSessionMode()) {
        case SessionMode::Long:
            stopPeriodicMode(QStringLiteral("Режим 2 остановлен перед отключением"));
            break;
        case SessionMode::Short:
            stopPeriodicMode(QStringLiteral("Режим 3 остановлен перед отключением"));
            break;
        case SessionMode::Single:
            stopPeriodicMode();
            break;
        }
    }

    if (socket_->state() == QAbstractSocket::UnconnectedState) {
        updateConnectionControls();
        return;
    }

    appendLog(QStringLiteral("Запрошено отключение от сервера"));
    socket_->disconnectFromHost();
    updateConnectionControls();
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
        if (socket_->state() != QAbstractSocket::ConnectedState) {
            return;
        }

        if (sendPacket(text)) {
            ui->message_lineEdit->clear();
        }
        break;

    case SessionMode::Long:
    {
        if (socket_->state() != QAbstractSocket::ConnectedState) {
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
        startLongMode(timeoutMs, text);
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
        startShortMode(timeoutMs, text);
        break;
    }
    }
}

//--------------------------------------------------------------------------

void MainWindow::onStopClicked()
{
    if (!sendTimer_->isActive()) {
        return;
    }

    const SessionMode mode = currentSessionMode();

    if (mode == SessionMode::Long) {
        stopPeriodicMode(QStringLiteral("Режим 2 остановлен"));
        return;
    }

    if (mode == SessionMode::Short) {
        stopPeriodicMode(QStringLiteral("Режим 3 остановлен"));

        if (socket_->state() != QAbstractSocket::UnconnectedState) {
            appendLog(QStringLiteral("Режим 3: текущий цикл прерывается отключением"));
            socket_->disconnectFromHost();
        }

        return;
    }
}

//--------------------------------------------------------------------------

void MainWindow::onSocketConnected()
{
    resetIncomingFrameState();

    appendLog(QStringLiteral("Подключение к серверу установлено: %1:%2")
              .arg(socket_->peerAddress().toString())
              .arg(socket_->peerPort()));

    qDebug() << "CLIENT: connected to server"
             << socket_->peerAddress().toString()
             << socket_->peerPort();

    if (sendTimer_->isActive() && currentSessionMode() == SessionMode::Short) {
        if (sendPacket(periodicMessageText_)) {
            shortModeWaitingForResponse_ = true;
        } else {
            appendLog(QStringLiteral("Режим 3 остановлен из-за ошибки отправки"));
            stopPeriodicMode();
            socket_->disconnectFromHost();
        }
    }

    updateConnectionControls();
}

//--------------------------------------------------------------------------

void MainWindow::onSocketDisconnected()
{
    const bool keepShortModeRunning =
        sendTimer_->isActive() &&
        currentSessionMode() == SessionMode::Short;

    resetIncomingFrameState();

    if (!keepShortModeRunning) {
        periodicMessageText_.clear();
    }

    appendLog(QStringLiteral("Соединение с сервером закрыто"));
    updateConnectionControls();

    qDebug() << "CLIENT: disconnected";
}

//--------------------------------------------------------------------------

void MainWindow::onSocketReadyRead()
{
    socketReadBuffer_.append(socket_->readAll());
    processSocketBuffer();
}

//--------------------------------------------------------------------------

void MainWindow::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << "CLIENT: socket state =" << socketStateToString(socketState);
    updateConnectionControls();
}

//--------------------------------------------------------------------------

void MainWindow::onSocketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    qDebug() << "CLIENT: socket error =" << socketErrorToString(socketError)
             << "|" << socket_->errorString();

    if (socketError != QAbstractSocket::RemoteHostClosedError) {
        appendLog(QStringLiteral("Ошибка сокета: %1").arg(socket_->errorString()));
    }

    updateConnectionControls();
}

//--------------------------------------------------------------------------

void MainWindow::onSendTimerTimeout()
{
    switch (currentSessionMode()) {
    case SessionMode::Long:
        if (!sendPacket(periodicMessageText_)) {
            stopPeriodicMode(QStringLiteral("Режим 2 остановлен из-за ошибки отправки"));
        }
        break;

    case SessionMode::Short:
        startShortModeCycle();
        break;

    case SessionMode::Single:
        break;
    }
}
