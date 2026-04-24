#ifndef CLIENTCONTROLLER_H
#define CLIENTCONTROLLER_H

#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QObject>

#include "sessionmode.h"

class QTcpSocket;
class QTimer;

/*!
 * \class ClientController
 * \brief Контроллер сетевого поведения TCP-клиента.
 *
 * \details
 * Класс инкапсулирует:
 * - клиентский QTcpSocket;
 * - таймер периодических режимов;
 * - режим 1 (ручная отправка);
 * - режим 2 (периодическая отправка в открытом соединении);
 * - режим 3 (периодическое подключение, отправка, получение ответа и отключение);
 * - обработку входящего TCP-буфера и пакетного протокола.
 *
 * Класс ничего не знает про конкретные виджеты интерфейса.
 */
class ClientController : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief Конструирует контроллер клиента.
     * \param parent Родительский QObject.
     */
    explicit ClientController(QObject *parent = nullptr);

    /*!
     * \brief Возвращает текущее состояние сокета.
     * \return Текущее состояние QAbstractSocket.
     */
    QAbstractSocket::SocketState socketState() const;

    /*!
     * \brief Возвращает признак активности периодического режима.
     * \return true, если периодический режим сейчас активен.
     */
    bool isPeriodicModeActive() const;

    /*!
     * \brief Возвращает активный периодический режим.
     * \return Режим, в котором сейчас работает таймер, либо SessionMode::Single.
     */
    SessionMode activePeriodicMode() const;

    /*!
     * \brief Запускает подключение к серверу.
     * \param address Адрес сервера.
     * \param port Порт сервера.
     */
    void connectToHost(const QHostAddress &address, quint16 port);

    /*!
     * \brief Запрашивает отключение от сервера.
     */
    void disconnectFromHost();

    /*!
     * \brief Отправляет один пакет серверу.
     * \param text Текст пакета.
     * \return true, если пакет поставлен в очередь на отправку, иначе false.
     */
    bool sendSinglePacket(const QString &text);

    /*!
     * \brief Запускает режим 2: периодическую отправку пакетов в открытом соединении.
     * \param text Текст пакета.
     * \param timeoutMs Интервал отправки в миллисекундах.
     * \return true, если режим успешно запущен, иначе false.
     */
    bool startLongMode(const QString &text, int timeoutMs);

    /*!
     * \brief Запускает режим 3: периодические подключения, отправку, приём ответа и отключение.
     * \param text Текст пакета.
     * \param timeoutMs Интервал цикла в миллисекундах.
     * \param address Адрес сервера.
     * \param port Порт сервера.
     * \return true, если режим успешно запущен, иначе false.
     */
    bool startShortMode(const QString &text, int timeoutMs, const QHostAddress &address, quint16 port);

    /*!
     * \brief Останавливает активный периодический режим.
     * \param logMessage Сообщение в лог. Если пустое, лог не пополняется.
     */
    void stopPeriodicMode(const QString &message = QString());

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
     *
     * \details
     * Используется UI-слоем для обновления доступности кнопок и полей.
     */
    void stateChanged();

private slots:
    /*!
     * \brief Обрабатывает успешное подключение к серверу.
     */
    void onSocketConnected();

    /*!
     * \brief Обрабатывает отключение от сервера.
     */
    void onSocketDisconnected();

    /*!
     * \brief Обрабатывает готовность данных для чтения.
     */
    void onSocketReadyRead();

    /*!
     * \brief Обрабатывает изменение состояния сокета.
     * \param socketState Новое состояние сокета.
     */
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

    /*!
     * \brief Обрабатывает ошибку сокета.
     * \param socketError Код ошибки сокета.
     */
    void onSocketErrorOccurred(QAbstractSocket::SocketError socketError);

    /*!
     * \brief Обрабатывает срабатывание таймера периодического режима.
     */
    void onSendTimerTimeout();

private:
    /*!
     * \brief Настраивает сигналы и слоты сокета и таймера.
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
     * \brief Обрабатывает накопленный входной буфер сокета.
     */
    void processSocketBuffer();

    /*!
     * \brief Отправляет один пакет серверу.
     * \param text Текст пакета.
     * \return true, если пакет поставлен в очередь на отправку, иначе false.
     */
    bool sendPacket(const QString &text);

    /*!
     * \brief Запускает один цикл режима 3.
     */
    void startShortModeCycle();

private:
    QTcpSocket *socket_ = nullptr;              /*!< Клиентский TCP-сокет. */
    QTimer *sendTimer_ = nullptr;               /*!< Таймер периодических режимов. */
    QByteArray socketReadBuffer_;               /*!< Накопленный входной буфер TCP-потока. */
    quint32 pendingServerBlockSize_ = 0;        /*!< Ожидаемый размер текущего входного кадра. */
    quint32 nextRequestNumber_ = 1;             /*!< Номер следующего исходящего пакета. */
    QString periodicMessageText_;               /*!< Текст для периодической отправки в режимах 2 и 3. */
    QHostAddress shortModeAddress_;             /*!< Адрес сервера для режима 3. */
    quint16 shortModePort_ = 0;                 /*!< Порт сервера для режима 3. */
    SessionMode activePeriodicMode_ = SessionMode::Single; /*!< Активный периодический режим. */
};

#endif // CLIENTCONTROLLER_H
