#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QDataStream>
#include <QDateTime>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QStatusBar>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

namespace
{
const QString kDefaultAddress = QStringLiteral("127.0.0.1");
const QString kDefaultPort = QStringLiteral("8888");

const QString kSettingsGroup = QStringLiteral("server");
const QString kAddressKey = QStringLiteral("address");
const QString kPortKey = QStringLiteral("port");

constexpr int kFrameHeaderSize = static_cast<int>(sizeof(quint32));
constexpr quint32 kMaxFramePayloadSize = 1024 * 1024;

const QDataStream::Version kStreamVersion = QDataStream::Qt_5_12;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , server_(new QTcpServer(this))
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

    if (server_->isListening()) {
        server_->close();
    }

    resetClientSocket();

    QMainWindow::closeEvent(event);
}

//--------------------------------------------------------------------------

void MainWindow::initializeUi()
{
    const QString ipv4BytePattern =
        QStringLiteral("(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)");

    const QString ipv4Pattern =
        QStringLiteral("^%1\\.%1\\.%1\\.%1$").arg(ipv4BytePattern);

    const QString portPattern =
        QStringLiteral("^(6553[0-5]|655[0-2]\\d|65[0-4]\\d{2}|6[0-4]\\d{3}|[1-5]\\d{4}|[1-9]\\d{0,3})$");

    auto *addressValidator = new QRegularExpressionValidator(
        QRegularExpression(ipv4Pattern),
        this
    );

    auto *portValidator = new QRegularExpressionValidator(
        QRegularExpression(portPattern),
        this
    );

    ui->address_lineEdit->setValidator(addressValidator);
    ui->port_lineEdit->setValidator(portValidator);

    ui->serverLog_plainTextEdit->setReadOnly(true);

    ui->address_lineEdit->setText(kDefaultAddress);
    ui->port_lineEdit->setText(kDefaultPort);

    loadSettings();
    updateListeningControls(false);

    statusBar()->showMessage(QStringLiteral("Сервер не запущен"));
}

//--------------------------------------------------------------------------

void MainWindow::connectSignals()
{
    connect(ui->startListening_button, &QPushButton::clicked,
            this, &MainWindow::onStartListeningClicked);

    connect(ui->stopListening_button, &QPushButton::clicked,
            this, &MainWindow::onStopListeningClicked);

    connect(ui->address_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->port_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(server_, &QTcpServer::newConnection,
            this, &MainWindow::onNewConnection);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(server_, &QTcpServer::acceptError,
            this, &MainWindow::onServerAcceptError);
#endif
}

//--------------------------------------------------------------------------

void MainWindow::loadSettings()
{
    QSettings settings;

    settings.beginGroup(kSettingsGroup);

    const QString address = settings.value(kAddressKey, kDefaultAddress).toString().trimmed();
    const QString port = settings.value(kPortKey, kDefaultPort).toString().trimmed();

    settings.endGroup();

    ui->address_lineEdit->setText(address);
    ui->port_lineEdit->setText(port);

    if (!ui->address_lineEdit->hasAcceptableInput()) {
        ui->address_lineEdit->setText(kDefaultAddress);
    }

    if (!ui->port_lineEdit->hasAcceptableInput()) {
        ui->port_lineEdit->setText(kDefaultPort);
    }
}

//--------------------------------------------------------------------------

void MainWindow::saveSettings()
{
    QSettings settings;

    settings.beginGroup(kSettingsGroup);

    if (ui->address_lineEdit->hasAcceptableInput()) {
        settings.setValue(kAddressKey, ui->address_lineEdit->text().trimmed());
    }

    if (ui->port_lineEdit->hasAcceptableInput()) {
        settings.setValue(kPortKey, ui->port_lineEdit->text().trimmed());
    }

    settings.endGroup();
}

//--------------------------------------------------------------------------

void MainWindow::appendLog(const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    ui->serverLog_plainTextEdit->appendPlainText(
        QStringLiteral("[%1] %2").arg(timestamp, message)
    );
}

//--------------------------------------------------------------------------

void MainWindow::updateListeningControls(bool isListening)
{
    ui->startListening_button->setEnabled(!isListening);
    ui->stopListening_button->setEnabled(isListening);

    ui->address_lineEdit->setEnabled(!isListening);
    ui->port_lineEdit->setEnabled(!isListening);
}

//--------------------------------------------------------------------------

bool MainWindow::tryGetListenParameters(QHostAddress &address, quint16 &port) const
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

void MainWindow::resetClientSocket()
{
    clientReadBuffer_.clear();
    pendingClientBlockSize_ = 0;

    if (!clientSocket_) {
        return;
    }

    clientSocket_->disconnect(this);

    if (clientSocket_->state() != QAbstractSocket::UnconnectedState) {
        clientSocket_->disconnectFromHost();
    }

    clientSocket_->deleteLater();
    clientSocket_.clear();
}

//--------------------------------------------------------------------------

QByteArray MainWindow::buildResponseFrame(quint32 number,
                                          const QString &text,
                                          const QTime &serverTime) const
{
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setVersion(kStreamVersion);
    payloadStream << number << text << serverTime;

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

bool MainWindow::parseRequestPayload(const QByteArray &payload,
                                     quint32 &number,
                                     QString &text) const
{
    QByteArray payloadCopy = payload;
    QDataStream payloadStream(&payloadCopy, QIODevice::ReadOnly);
    payloadStream.setVersion(kStreamVersion);

    payloadStream >> number >> text;

    return payloadStream.status() == QDataStream::Ok;
}

//--------------------------------------------------------------------------

void MainWindow::processClientBuffer()
{
    while (true) {
        QByteArray payload;

        if (!tryExtractFrame(clientReadBuffer_, pendingClientBlockSize_, payload)) {
            break;
        }

        quint32 number = 0;
        QString text;

        if (!parseRequestPayload(payload, number, text)) {
            appendLog(QStringLiteral("Не удалось разобрать пакет клиента"));
            continue;
        }

        appendLog(QStringLiteral("Получен пакет: number=%1, text=\"%2\"")
                  .arg(number)
                  .arg(text));

        const QTime currentTime = QTime::currentTime();
        const QByteArray responseFrame = buildResponseFrame(number, text, currentTime);

        if (!clientSocket_) {
            appendLog(QStringLiteral("Активный клиент отсутствует, ответ не отправлен"));
            continue;
        }

        const qint64 written = clientSocket_->write(responseFrame);

        if (written == -1) {
            appendLog(QStringLiteral("Не удалось отправить пакет-ответ: %1")
                      .arg(clientSocket_->errorString()));
            continue;
        }

        appendLog(QStringLiteral("Отправлен пакет-ответ: number=%1, text=\"%2\", time=%3")
                  .arg(number)
                  .arg(text)
                  .arg(currentTime.toString(QStringLiteral("HH:mm:ss"))));
    }
}

//--------------------------------------------------------------------------

void MainWindow::onStartListeningClicked()
{
    QHostAddress address;
    quint16 port = 0;

    if (!tryGetListenParameters(address, port)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Некорректные параметры"),
            QStringLiteral("Введите корректные адрес и порт сервера.")
        );
        return;
    }

    saveSettings();

    if (!server_->listen(address, port)) {
        const QString errorText = server_->errorString();

        appendLog(QStringLiteral("Ошибка запуска прослушивания: %1").arg(errorText));

        QMessageBox::critical(
            this,
            QStringLiteral("Ошибка запуска сервера"),
            QStringLiteral("Не удалось запустить прослушивание:\n%1").arg(errorText)
        );
        return;
    }

    appendLog(QStringLiteral("Сервер начал прослушивание на %1:%2")
              .arg(server_->serverAddress().toString())
              .arg(server_->serverPort()));

    updateListeningControls(true);
    statusBar()->showMessage(QStringLiteral("Сервер прослушивает порт"));
}

//--------------------------------------------------------------------------

void MainWindow::onStopListeningClicked()
{
    if (clientSocket_) {
        appendLog(QStringLiteral("Активный клиент будет отключён"));
        resetClientSocket();
    }

    if (server_->isListening()) {
        server_->close();
        appendLog(QStringLiteral("Прослушивание остановлено"));
    }

    updateListeningControls(false);
    statusBar()->showMessage(QStringLiteral("Сервер остановлен"));
}

//--------------------------------------------------------------------------

void MainWindow::onNewConnection()
{
    while (server_->hasPendingConnections()) {
        QTcpSocket *pendingSocket = server_->nextPendingConnection();

        if (pendingSocket == nullptr) {
            continue;
        }

        if (clientSocket_) {
            appendLog(QStringLiteral("Дополнительное подключение %1:%2 отклонено")
                      .arg(pendingSocket->peerAddress().toString())
                      .arg(pendingSocket->peerPort()));

            pendingSocket->disconnectFromHost();
            pendingSocket->deleteLater();
            continue;
        }

        clientSocket_ = pendingSocket;
        clientReadBuffer_.clear();
        pendingClientBlockSize_ = 0;

        connect(clientSocket_, &QTcpSocket::readyRead,
                this, &MainWindow::onClientReadyRead);

        connect(clientSocket_, &QTcpSocket::disconnected,
                this, &MainWindow::onClientDisconnected);

        connect(clientSocket_, &QTcpSocket::stateChanged,
                this, &MainWindow::onClientStateChanged);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(clientSocket_, &QTcpSocket::errorOccurred,
                this, &MainWindow::onClientErrorOccurred);
#else
        connect(clientSocket_,
                QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                this,
                &MainWindow::onClientErrorOccurred);
#endif

        appendLog(QStringLiteral("Клиент подключился: %1:%2")
                  .arg(clientSocket_->peerAddress().toString())
                  .arg(clientSocket_->peerPort()));

        statusBar()->showMessage(QStringLiteral("Клиент подключён"));

        qDebug() << "SERVER: client connected:"
                 << clientSocket_->peerAddress().toString()
                 << clientSocket_->peerPort();
    }
}

//--------------------------------------------------------------------------

void MainWindow::onClientReadyRead()
{
    if (!clientSocket_) {
        return;
    }

    clientReadBuffer_.append(clientSocket_->readAll());
    processClientBuffer();
}

//--------------------------------------------------------------------------

void MainWindow::onClientDisconnected()
{
    QString peerAddress = QStringLiteral("<unknown>");
    quint16 peerPort = 0;

    if (clientSocket_) {
        peerAddress = clientSocket_->peerAddress().toString();
        peerPort = clientSocket_->peerPort();
    }

    appendLog(QStringLiteral("Клиент отключился: %1:%2")
              .arg(peerAddress)
              .arg(peerPort));

    qDebug() << "SERVER: client disconnected";

    resetClientSocket();

    if (server_->isListening()) {
        statusBar()->showMessage(QStringLiteral("Сервер прослушивает порт"));
    } else {
        statusBar()->showMessage(QStringLiteral("Сервер не запущен"));
    }
}

//--------------------------------------------------------------------------

void MainWindow::onClientStateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << "SERVER: client socket state =" << socketStateToString(socketState);
}

//--------------------------------------------------------------------------

void MainWindow::onClientErrorOccurred(QAbstractSocket::SocketError socketError)
{
    qDebug() << "SERVER: client socket error =" << socketErrorToString(socketError);

    if (socketError != QAbstractSocket::RemoteHostClosedError && clientSocket_) {
        appendLog(QStringLiteral("Ошибка клиентского сокета: %1")
                  .arg(clientSocket_->errorString()));
    }
}

//--------------------------------------------------------------------------

void MainWindow::onServerAcceptError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "SERVER: accept error =" << socketErrorToString(socketError);

    appendLog(QStringLiteral("Ошибка принятия соединения: %1")
              .arg(socketErrorToString(socketError)));
}
