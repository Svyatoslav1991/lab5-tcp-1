#include "servercontroller.h"

#include "packetprotocol.h"

#include <QDateTime>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

ServerController::ServerController(QObject *parent)
    : QObject(parent)
    , server_(new QTcpServer(this))
{
    connectSignals();
}

//--------------------------------------------------------------------------

bool ServerController::isListening() const
{
    return server_->isListening();
}

//--------------------------------------------------------------------------

bool ServerController::hasActiveClient() const
{
    return !clientSocket_.isNull();
}

//--------------------------------------------------------------------------

QString ServerController::lastErrorString() const
{
    return lastErrorString_;
}

//--------------------------------------------------------------------------

bool ServerController::startListening(const QHostAddress &address, quint16 port)
{
    lastErrorString_.clear();

    if (server_->isListening()) {
        return true;
    }

    if (!server_->listen(address, port)) {
        lastErrorString_ = server_->errorString();
        return false;
    }

    emit logMessage(QStringLiteral("Сервер начал прослушивание на %1:%2")
                    .arg(server_->serverAddress().toString())
                    .arg(server_->serverPort()));

    emit stateChanged();

    return true;
}

//--------------------------------------------------------------------------

void ServerController::stopListening()
{
    if (clientSocket_) {
        emit logMessage(QStringLiteral("Активный клиент будет отключён"));
        resetClientSocket();
    }

    if (server_->isListening()) {
        server_->close();
        emit logMessage(QStringLiteral("Прослушивание остановлено"));
    }

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ServerController::shutdown()
{
    if (server_->isListening()) {
        server_->close();
    }

    resetClientSocket();
    lastErrorString_.clear();

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ServerController::onNewConnection()
{
    while (server_->hasPendingConnections()) {
        QTcpSocket *pendingSocket = server_->nextPendingConnection();

        if (pendingSocket == nullptr) {
            continue;
        }

        if (clientSocket_) {
            emit logMessage(QStringLiteral("Дополнительное подключение %1:%2 отклонено")
                            .arg(pendingSocket->peerAddress().toString())
                            .arg(pendingSocket->peerPort()));

            pendingSocket->disconnectFromHost();
            pendingSocket->deleteLater();
            continue;
        }

        clientSocket_ = pendingSocket;
        resetIncomingFrameState();

        connect(clientSocket_, &QTcpSocket::readyRead,
                this, &ServerController::onClientReadyRead);

        connect(clientSocket_, &QTcpSocket::disconnected,
                this, &ServerController::onClientDisconnected);

        connect(clientSocket_, &QTcpSocket::stateChanged,
                this, &ServerController::onClientStateChanged);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(clientSocket_, &QTcpSocket::errorOccurred,
                this, &ServerController::onClientErrorOccurred);
#else
        connect(clientSocket_,
                QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                this,
                &ServerController::onClientErrorOccurred);
#endif

        emit logMessage(QStringLiteral("Клиент подключился: %1:%2")
                        .arg(clientSocket_->peerAddress().toString())
                        .arg(clientSocket_->peerPort()));

        qDebug() << "SERVER: client connected:"
                 << clientSocket_->peerAddress().toString()
                 << clientSocket_->peerPort();

        emit stateChanged();
    }
}

//--------------------------------------------------------------------------

void ServerController::onClientReadyRead()
{
    if (!clientSocket_) {
        return;
    }

    clientReadBuffer_.append(clientSocket_->readAll());
    processClientBuffer();
}

//--------------------------------------------------------------------------

void ServerController::onClientDisconnected()
{
    QString peerAddress = QStringLiteral("<unknown>");
    quint16 peerPort = 0;

    if (clientSocket_) {
        peerAddress = clientSocket_->peerAddress().toString();
        peerPort = clientSocket_->peerPort();
    }

    emit logMessage(QStringLiteral("Клиент отключился: %1:%2")
                    .arg(peerAddress)
                    .arg(peerPort));

    qDebug() << "SERVER: client disconnected";

    resetClientSocket();
    emit stateChanged();
}

//--------------------------------------------------------------------------

void ServerController::onClientStateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << "SERVER: client socket state =" << socketStateToString(socketState);
    emit stateChanged();
}

//--------------------------------------------------------------------------

void ServerController::onClientErrorOccurred(QAbstractSocket::SocketError socketError)
{
    qDebug() << "SERVER: client socket error =" << socketErrorToString(socketError);

    if (socketError != QAbstractSocket::RemoteHostClosedError && clientSocket_) {
        emit logMessage(QStringLiteral("Ошибка клиентского сокета: %1")
                        .arg(clientSocket_->errorString()));
    }

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ServerController::onServerAcceptError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "SERVER: accept error =" << socketErrorToString(socketError);

    emit logMessage(QStringLiteral("Ошибка принятия соединения: %1")
                    .arg(socketErrorToString(socketError)));

    emit stateChanged();
}

//--------------------------------------------------------------------------

void ServerController::connectSignals()
{
    connect(server_, &QTcpServer::newConnection,
            this, &ServerController::onNewConnection);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(server_, &QTcpServer::acceptError,
            this, &ServerController::onServerAcceptError);
#endif
}

//--------------------------------------------------------------------------

QString ServerController::socketStateToString(QAbstractSocket::SocketState socketState) const
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

QString ServerController::socketErrorToString(QAbstractSocket::SocketError socketError) const
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

void ServerController::resetIncomingFrameState()
{
    clientReadBuffer_.clear();
    pendingClientBlockSize_ = 0;
}

//--------------------------------------------------------------------------

void ServerController::resetClientSocket()
{
    resetIncomingFrameState();

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

void ServerController::processClientBuffer()
{
    while (true) {
        QByteArray payload;

        if (!PacketProtocol::tryExtractFrame(clientReadBuffer_, pendingClientBlockSize_, payload)) {
            break;
        }

        RequestPacket requestPacket;

        if (!PacketProtocol::parseRequestPayload(payload, requestPacket)) {
            emit logMessage(QStringLiteral("Не удалось разобрать пакет клиента"));
            continue;
        }

        emit logMessage(QStringLiteral("Получен пакет: number=%1, text=\"%2\"")
                        .arg(requestPacket.number)
                        .arg(requestPacket.text));

        if (!clientSocket_) {
            emit logMessage(QStringLiteral("Активный клиент отсутствует, ответ не отправлен"));
            continue;
        }

        ResponsePacket responsePacket;
        responsePacket.number = requestPacket.number;
        responsePacket.text = requestPacket.text;
        responsePacket.serverTime = QTime::currentTime();

        const QByteArray responseFrame = PacketProtocol::buildResponseFrame(responsePacket);
        const qint64 written = clientSocket_->write(responseFrame);

        if (written == -1) {
            emit logMessage(QStringLiteral("Не удалось отправить пакет-ответ: %1")
                            .arg(clientSocket_->errorString()));
            continue;
        }

        emit logMessage(QStringLiteral("Отправлен пакет-ответ: number=%1, text=\"%2\", time=%3")
                        .arg(responsePacket.number)
                        .arg(responsePacket.text)
                        .arg(responsePacket.serverTime.toString(QStringLiteral("HH:mm:ss"))));
    }
}
