#include "clientcontroller.h"

#include "packetprotocol.h"

#include <QTcpSocket>
#include <QTimer>
#include <QDebug>

ClientController::ClientController(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , sendTimer_(new QTimer(this))
{
    sendTimer_->setSingleShot(false);
    connectSignals();
}

//--------------------------------------------------------------------------

QAbstractSocket::SocketState ClientController::socketState() const
{
    return socket_->state();
}

//--------------------------------------------------------------------------

bool ClientController::isPeriodicModeActive() const
{
    return sendTimer_->isActive();
}

//--------------------------------------------------------------------------

SessionMode ClientController::activePeriodicMode() const
{
    return activePeriodicMode_;
}

//--------------------------------------------------------------------------

void ClientController::connectToHost(const QHostAddress &address, quint16 port)
{
    emit logMessage(QStringLiteral("Попытка подключения к %1:%2")
                    .arg(address.toString())
                    .arg(port));

    socket_->connectToHost(address, port);
    emit stateChanged();
}

//--------------------------------------------------------------------------

void ClientController::disconnectFromHost()
{
    if (sendTimer_->isActive()) {
        switch (activePeriodicMode_) {
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
        emit stateChanged();
        return;
    }

    emit logMessage(QStringLiteral("Запрошено отключение от сервера"));
    socket_->disconnectFromHost();
    emit stateChanged();
}

//--------------------------------------------------------------------------

bool ClientController::sendSinglePacket(const QString &text)
{
    return sendPacket(text);
}

//--------------------------------------------------------------------------

bool ClientController::startLongMode(const QString &text, int timeoutMs)
{
    if (!sendPacket(text)) {
        return false;
    }

    periodicMessageText_ = text;
    activePeriodicMode_ = SessionMode::Long;
    sendTimer_->start(timeoutMs);

    emit logMessage(QStringLiteral("Запущен режим 2: периодическая отправка, timeout=%1 мс")
                    .arg(timeoutMs));

    emit stateChanged();
    return true;
}

//--------------------------------------------------------------------------

bool ClientController::startShortMode(const QString &text,
                                      int timeoutMs,
                                      const QHostAddress &address,
                                      quint16 port)
{
    periodicMessageText_ = text;
    shortModeAddress_ = address;
    shortModePort_ = port;
    activePeriodicMode_ = SessionMode::Short;

    sendTimer_->start(timeoutMs);

    emit logMessage(QStringLiteral("Запущен режим 3: периодические подключения каждые %1 мс")
                    .arg(timeoutMs));

    startShortModeCycle();
    emit stateChanged();

    return true;
}

//--------------------------------------------------------------------------

void ClientController::stopPeriodicMode(const QString &message)
{
    if (!sendTimer_->isActive()) {
        return;
    }

    sendTimer_->stop();
    activePeriodicMode_ = SessionMode::Single;

    if (!message.isEmpty()) {
        emit logMessage(message);
    }

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ClientController::shutdown()
{
    if (sendTimer_->isActive()) {
        sendTimer_->stop();
    }

    activePeriodicMode_ = SessionMode::Single;
    periodicMessageText_.clear();
    resetIncomingFrameState();

    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
        socket_->close();
    }

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ClientController::onSocketConnected()
{
    resetIncomingFrameState();

    emit logMessage(QStringLiteral("Подключение к серверу установлено: %1:%2")
                    .arg(socket_->peerAddress().toString())
                    .arg(socket_->peerPort()));

    qDebug() << "CLIENT: connected to server"
             << socket_->peerAddress().toString()
             << socket_->peerPort();

    if (sendTimer_->isActive() && activePeriodicMode_ == SessionMode::Short) {
        if (!sendPacket(periodicMessageText_)) {
            emit logMessage(QStringLiteral("Режим 3 остановлен из-за ошибки отправки"));
            stopPeriodicMode();
            socket_->disconnectFromHost();
        }
    }

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ClientController::onSocketDisconnected()
{
    const bool keepShortModeRunning =
        sendTimer_->isActive() &&
        activePeriodicMode_ == SessionMode::Short;

    resetIncomingFrameState();

    if (!keepShortModeRunning) {
        periodicMessageText_.clear();
    }

    emit logMessage(QStringLiteral("Соединение с сервером закрыто"));

    qDebug() << "CLIENT: disconnected";

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ClientController::onSocketReadyRead()
{
    socketReadBuffer_.append(socket_->readAll());
    processSocketBuffer();
}

//--------------------------------------------------------------------------

void ClientController::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << "CLIENT: socket state =" << socketStateToString(socketState);
    emit stateChanged();
}

//--------------------------------------------------------------------------

void ClientController::onSocketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    qDebug() << "CLIENT: socket error =" << socketErrorToString(socketError)
             << "|" << socket_->errorString();

    if (socketError != QAbstractSocket::RemoteHostClosedError) {
        emit logMessage(QStringLiteral("Ошибка сокета: %1").arg(socket_->errorString()));
    }

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ClientController::onSendTimerTimeout()
{
    switch (activePeriodicMode_) {
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

//--------------------------------------------------------------------------

void ClientController::connectSignals()
{
    connect(socket_, &QTcpSocket::connected,
            this, &ClientController::onSocketConnected);

    connect(socket_, &QTcpSocket::disconnected,
            this, &ClientController::onSocketDisconnected);

    connect(socket_, &QTcpSocket::readyRead,
            this, &ClientController::onSocketReadyRead);

    connect(socket_, &QTcpSocket::stateChanged,
            this, &ClientController::onSocketStateChanged);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket_, &QTcpSocket::errorOccurred,
            this, &ClientController::onSocketErrorOccurred);
#else
    connect(socket_,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this,
            &ClientController::onSocketErrorOccurred);
#endif

    connect(sendTimer_, &QTimer::timeout,
            this, &ClientController::onSendTimerTimeout);
}

//--------------------------------------------------------------------------

QString ClientController::socketStateToString(QAbstractSocket::SocketState socketState) const
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

QString ClientController::socketErrorToString(QAbstractSocket::SocketError socketError) const
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

void ClientController::resetIncomingFrameState()
{
    socketReadBuffer_.clear();
    pendingServerBlockSize_ = 0;
}

//--------------------------------------------------------------------------

void ClientController::processSocketBuffer()
{
    bool responseReceived = false;

    while (true) {
        QByteArray payload;

        if (!PacketProtocol::tryExtractFrame(socketReadBuffer_, pendingServerBlockSize_, payload)) {
            break;
        }

        ResponsePacket packet;

        if (!PacketProtocol::parseResponsePayload(payload, packet)) {
            emit logMessage(QStringLiteral("Не удалось разобрать пакет ответа сервера"));
            continue;
        }

        emit logMessage(QStringLiteral("Получен пакет-ответ: number=%1, text=\"%2\", time=%3")
                        .arg(packet.number)
                        .arg(packet.text)
                        .arg(packet.serverTime.toString(QStringLiteral("HH:mm:ss"))));

        responseReceived = true;
    }

    if (responseReceived &&
        sendTimer_->isActive() &&
        activePeriodicMode_ == SessionMode::Short &&
        socket_->state() == QAbstractSocket::ConnectedState) {
        emit logMessage(QStringLiteral("Режим 3: ответ получен, выполняется отключение"));
        socket_->disconnectFromHost();
    }
}

//--------------------------------------------------------------------------

bool ClientController::sendPacket(const QString &text)
{
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        return false;
    }

    const RequestPacket packet { nextRequestNumber_++, text };
    const QByteArray frame = PacketProtocol::buildRequestFrame(packet);

    const qint64 written = socket_->write(frame);

    if (written == -1) {
        emit logMessage(QStringLiteral("Не удалось отправить пакет: %1").arg(socket_->errorString()));
        return false;
    }

    emit logMessage(QStringLiteral("Отправлен пакет: number=%1, text=\"%2\"")
                    .arg(packet.number)
                    .arg(packet.text));

    return true;
}

//--------------------------------------------------------------------------

void ClientController::startShortModeCycle()
{
    if (activePeriodicMode_ != SessionMode::Short || !sendTimer_->isActive()) {
        return;
    }

    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        qDebug() << "CLIENT: mode3 cycle skipped, socket state =" << socketStateToString(socket_->state());
        return;
    }

    emit logMessage(QStringLiteral("Режим 3: попытка подключения к %1:%2")
                    .arg(shortModeAddress_.toString())
                    .arg(shortModePort_));

    socket_->connectToHost(shortModeAddress_, shortModePort_);
    emit stateChanged();
}
