#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <QPointer>

class QTcpServer;
class QTcpSocket;

/*!
 * \class ServerController
 * \brief Контроллер сетевого поведения TCP-сервера.
 *
 * \details
 * Класс инкапсулирует:
 * - QTcpServer;
 * - активный клиентский QTcpSocket;
 * - приём входящих соединений;
 * - пакетный обмен через PacketProtocol;
 * - обработку отключения клиента и ошибок;
 * - накопленный входной TCP-буфер.
 *
 * Для упрощения текущей реализации сервер обслуживает одного активного клиента.
 */
class ServerController : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief Конструирует контроллер сервера.
     * \param parent Родительский QObject.
     */
    explicit ServerController(QObject *parent = nullptr);

    /*!
     * \brief Возвращает признак активного прослушивания порта.
     * \return true, если сервер прослушивает порт.
     */
    bool isListening() const;

    /*!
     * \brief Возвращает признак наличия активного клиента.
     * \return true, если активный клиент подключён.
     */
    bool hasActiveClient() const;

    /*!
     * \brief Возвращает текст последней ошибки запуска прослушивания.
     * \return Текст последней ошибки.
     */
    QString lastErrorString() const;

    /*!
     * \brief Запускает прослушивание на заданном адресе и порту.
     * \param address Адрес сервера.
     * \param port Порт сервера.
     * \return true, если прослушивание успешно запущено, иначе false.
     */
    bool startListening(const QHostAddress &address, quint16 port);

    /*!
     * \brief Останавливает прослушивание и при необходимости отключает активного клиента.
     */
    void stopListening();

    /*!
     * \brief Тихо завершает работу контроллера без пользовательских сообщений.
     */
    void shutdown();

signals:
    /*!
     * \brief Сообщение для пользовательского лога.
     * \param message Текст сообщения.
     */
    void logMessage(const QString &message);

    /*!
     * \brief Сигнал об изменении внутреннего состояния контроллера.
     */
    void stateChanged();

private slots:
    /*!
     * \brief Обрабатывает появление нового входящего соединения.
     */
    void onNewConnection();

    /*!
     * \brief Обрабатывает готовность данных от активного клиента.
     */
    void onClientReadyRead();

    /*!
     * \brief Обрабатывает отключение активного клиента.
     */
    void onClientDisconnected();

    /*!
     * \brief Обрабатывает изменение состояния активного клиентского сокета.
     * \param socketState Новое состояние сокета.
     */
    void onClientStateChanged(QAbstractSocket::SocketState socketState);

    /*!
     * \brief Обрабатывает ошибку активного клиентского сокета.
     * \param socketError Код ошибки сокета.
     */
    void onClientErrorOccurred(QAbstractSocket::SocketError socketError);

    /*!
     * \brief Обрабатывает ошибку при приёме нового соединения сервером.
     * \param socketError Код ошибки сокета.
     */
    void onServerAcceptError(QAbstractSocket::SocketError socketError);

private:
    /*!
     * \brief Настраивает сигналы и слоты серверных объектов.
     */
    void connectSignals();

    /*!
     * \brief Возвращает строковое представление состояния сокета.
     * \param socketState Состояние сокета.
     * \return Строковое представление состояния сокета.
     */
    QString socketStateToString(QAbstractSocket::SocketState socketState) const;

    /*!
     * \brief Возвращает строковое представление ошибки сокета.
     * \param socketError Код ошибки сокета.
     * \return Строковое представление ошибки сокета.
     */
    QString socketErrorToString(QAbstractSocket::SocketError socketError) const;

    /*!
     * \brief Сбрасывает состояние накопленного входного буфера.
     */
    void resetIncomingFrameState();

    /*!
     * \brief Корректно отключает и удаляет активный клиентский сокет.
     */
    void resetClientSocket();

    /*!
     * \brief Обрабатывает накопленный входной буфер клиента.
     */
    void processClientBuffer();

private:
    QTcpServer *server_ = nullptr;             /*!< Сервер TCP. */
    QPointer<QTcpSocket> clientSocket_;        /*!< Активный клиентский сокет. */
    QByteArray clientReadBuffer_;              /*!< Накопленный входной TCP-буфер клиента. */
    quint32 pendingClientBlockSize_ = 0;       /*!< Ожидаемый размер текущего входного кадра. */
    QString lastErrorString_;                  /*!< Текст последней ошибки запуска сервера. */
};

#endif // SERVERCONTROLLER_H
