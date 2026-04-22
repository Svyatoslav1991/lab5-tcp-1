#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QDataStream>
#include <QDateTime>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QStatusBar>
#include <QTcpSocket>
#include <QDebug>

namespace
{
const QString kDefaultAddress = QStringLiteral("127.0.0.1");
const QString kDefaultPort = QStringLiteral("8888");
const QString kDefaultTimeout = QStringLiteral("1000");

const QString kSettingsGroup = QStringLiteral("client");
const QString kAddressKey = QStringLiteral("address");
const QString kPortKey = QStringLiteral("port");
const QString kTimeoutKey = QStringLiteral("timeout");
const QString kSessionModeKey = QStringLiteral("session_mode");

const QString kSingleMode = QStringLiteral("single");
const QString kLongMode = QStringLiteral("long");
const QString kShortMode = QStringLiteral("short");

constexpr int kFrameHeaderSize = static_cast<int>(sizeof(quint32));
constexpr quint32 kMaxFramePayloadSize = 1024 * 1024;

const QDataStream::Version kStreamVersion = QDataStream::Qt_5_12;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket_(new QTcpSocket(this))
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

    loadSettings();
    updateConnectionControls();

    ui->single_radioButton->setEnabled(false);
    ui->long_radioButton->setEnabled(false);
    ui->short_radioButton->setEnabled(false);
    ui->timeout_lineEdit->setEnabled(false);
    ui->stop_pushButton->setEnabled(false);

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

    connect(ui->connect_pushButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);

    connect(ui->disconnect_pushButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectClicked);

    connect(ui->write_pushButton, &QPushButton::clicked,
            this, &MainWindow::onWriteClicked);

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
}

//--------------------------------------------------------------------------

void MainWindow::loadSettings()
{
    QSettings settings;

    settings.beginGroup(kSettingsGroup);

    const QString address = settings.value(kAddressKey, kDefaultAddress).toString().trimmed();
    const QString port = settings.value(kPortKey, kDefaultPort).toString().trimmed();
    const QString timeout = settings.value(kTimeoutKey, kDefaultTimeout).toString().trimmed();
    const QString sessionMode = settings.value(kSessionModeKey, kSingleMode).toString();

    settings.endGroup();

    ui->address_lineEdit->setText(address);
    ui->port_lineEdit->setText(port);
    ui->timeout_lineEdit->setText(timeout);

    if (!ui->address_lineEdit->hasAcceptableInput()) {
        ui->address_lineEdit->setText(kDefaultAddress);
    }

    if (!ui->port_lineEdit->hasAcceptableInput()) {
        ui->port_lineEdit->setText(kDefaultPort);
    }

    if (!ui->timeout_lineEdit->hasAcceptableInput()) {
        ui->timeout_lineEdit->setText(kDefaultTimeout);
    }

    if (sessionMode == kLongMode) {
        ui->long_radioButton->setChecked(true);
    } else if (sessionMode == kShortMode) {
        ui->short_radioButton->setChecked(true);
    } else {
        ui->single_radioButton->setChecked(true);
    }

    ui->single_radioButton->setChecked(true);
}

//--------------------------------------------------------------------------

void MainWindow::saveSettings()
{
    QString sessionMode = kSingleMode;

    if (ui->long_radioButton->isChecked()) {
        sessionMode = kLongMode;
    } else if (ui->short_radioButton->isChecked()) {
        sessionMode = kShortMode;
    }

    QSettings settings;

    settings.beginGroup(kSettingsGroup);

    if (ui->address_lineEdit->hasAcceptableInput()) {
        settings.setValue(kAddressKey, ui->address_lineEdit->text().trimmed());
    }

    if (ui->port_lineEdit->hasAcceptableInput()) {
        settings.setValue(kPortKey, ui->port_lineEdit->text().trimmed());
    }

    if (ui->timeout_lineEdit->hasAcceptableInput()) {
        settings.setValue(kTimeoutKey, ui->timeout_lineEdit->text().trimmed());
    }

    settings.setValue(kSessionModeKey, sessionMode);

    settings.endGroup();
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

void MainWindow::updateConnectionControls()
{
    const QAbstractSocket::SocketState state = socket_->state();
    const bool isConnected = state == QAbstractSocket::ConnectedState;
    const bool isUnconnected = state == QAbstractSocket::UnconnectedState;

    ui->connect_pushButton->setEnabled(isUnconnected);
    ui->disconnect_pushButton->setEnabled(!isUnconnected);
    ui->write_pushButton->setEnabled(isConnected);
    ui->message_lineEdit->setEnabled(isConnected);

    ui->address_lineEdit->setEnabled(isUnconnected);
    ui->port_lineEdit->setEnabled(isUnconnected);

    if (isConnected) {
        statusBar()->showMessage(QStringLiteral("Клиент подключен к серверу"));
    } else if (state == QAbstractSocket::ConnectingState) {
        statusBar()->showMessage(QStringLiteral("Подключение к серверу..."));
    } else if (state == QAbstractSocket::ClosingState) {
        statusBar()->showMessage(QStringLiteral("Отключение от сервера..."));
    } else {
        statusBar()->showMessage(QStringLiteral("Клиент не подключен"));
    }
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

QByteArray MainWindow::buildRequestFrame(quint32 number, const QString &text) const
{
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setVersion(kStreamVersion);
    payloadStream << number << text;

    QByteArray frame;
    QDataStream frameStream(&frame, QIODevice::WriteOnly);
    frameStream.setVersion(kStreamVersion);
    frameStream << static_cast<quint32>(payload.size());

    frame.append(payload);

    return frame;
}

//--------------------------------------------------------------------------

bool MainWindow::tryExtractFrame(QByteArray &buffer,
                                 quint32 &pendingBlockSize,
                                 QByteArray &payload)
{
    payload.clear();

    if (pendingBlockSize == 0) {
        if (buffer.size() < kFrameHeaderSize) {
            return false;
        }

        QByteArray headerData = buffer.left(kFrameHeaderSize);
        QDataStream headerStream(&headerData, QIODevice::ReadOnly);
        headerStream.setVersion(kStreamVersion);
        headerStream >> pendingBlockSize;

        if (pendingBlockSize == 0 || pendingBlockSize > kMaxFramePayloadSize) {
            appendLog(QStringLiteral("Получен некорректный размер пакета, входной буфер очищен"));
            buffer.clear();
            pendingBlockSize = 0;
            return false;
        }
    }

    const int fullFrameSize = kFrameHeaderSize + static_cast<int>(pendingBlockSize);

    if (buffer.size() < fullFrameSize) {
        return false;
    }

    payload = buffer.mid(kFrameHeaderSize, static_cast<int>(pendingBlockSize));
    buffer.remove(0, fullFrameSize);
    pendingBlockSize = 0;

    return true;
}

//--------------------------------------------------------------------------

bool MainWindow::parseResponsePayload(const QByteArray &payload,
                                      quint32 &number,
                                      QString &text,
                                      QTime &serverTime) const
{
    QByteArray payloadCopy = payload;
    QDataStream payloadStream(&payloadCopy, QIODevice::ReadOnly);
    payloadStream.setVersion(kStreamVersion);

    payloadStream >> number >> text >> serverTime;

    return payloadStream.status() == QDataStream::Ok;
}

//--------------------------------------------------------------------------

void MainWindow::processSocketBuffer()
{
    while (true) {
        QByteArray payload;

        if (!tryExtractFrame(socketReadBuffer_, pendingServerBlockSize_, payload)) {
            break;
        }

        quint32 number = 0;
        QString text;
        QTime serverTime;

        if (!parseResponsePayload(payload, number, text, serverTime)) {
            appendLog(QStringLiteral("Не удалось разобрать пакет ответа сервера"));
            continue;
        }

        appendLog(QStringLiteral("Получен пакет-ответ: number=%1, text=\"%2\", time=%3")
                  .arg(number)
                  .arg(text)
                  .arg(serverTime.toString(QStringLiteral("HH:mm:ss"))));
    }
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
    if (socket_->state() == QAbstractSocket::UnconnectedState) {
        return;
    }

    appendLog(QStringLiteral("Запрошено отключение от сервера"));
    socket_->disconnectFromHost();
    updateConnectionControls();
}

//--------------------------------------------------------------------------

void MainWindow::onWriteClicked()
{
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    const QString text = ui->message_lineEdit->text().trimmed();

    if (text.isEmpty()) {
        QMessageBox::information(
            this,
            QStringLiteral("Пустое сообщение"),
            QStringLiteral("Введите строку для отправки.")
        );
        return;
    }

    const quint32 requestNumber = nextRequestNumber_++;
    const QByteArray frame = buildRequestFrame(requestNumber, text);

    const qint64 written = socket_->write(frame);

    if (written == -1) {
        appendLog(QStringLiteral("Не удалось отправить пакет: %1").arg(socket_->errorString()));
        return;
    }

    appendLog(QStringLiteral("Отправлен пакет: number=%1, text=\"%2\"")
              .arg(requestNumber)
              .arg(text));

    ui->message_lineEdit->clear();
}

//--------------------------------------------------------------------------

void MainWindow::onSocketConnected()
{
    socketReadBuffer_.clear();
    pendingServerBlockSize_ = 0;

    appendLog(QStringLiteral("Подключение к серверу установлено: %1:%2")
              .arg(socket_->peerAddress().toString())
              .arg(socket_->peerPort()));

    updateConnectionControls();

    qDebug() << "CLIENT: connected to server"
             << socket_->peerAddress().toString()
             << socket_->peerPort();
}

//--------------------------------------------------------------------------

void MainWindow::onSocketDisconnected()
{
    socketReadBuffer_.clear();
    pendingServerBlockSize_ = 0;

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
