#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QMainWindow>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;
class QTcpSocket;
class QTimer;

/*!
 * \class MainWindow
 * \brief Главное окно TCP-клиента для лабораторной работы №5.
 *
 * \details
 * Класс реализует асинхронный TCP-клиент с графическим интерфейсом.
 * Пользователь задаёт адрес и порт сервера, после чего по кнопке запускает
 * подключение через QTcpSocket::connectToHost(). Отключение выполняется
 * по запросу пользователя через QTcpSocket::disconnectFromHost().
 *
 * На текущем этапе реализованы:
 * - ввод и валидация адреса, порта и timeout;
 * - сохранение пользовательских настроек через QSettings;
 * - асинхронное подключение и отключение;
 * - обработка connected/disconnected/stateChanged/errorOccurred;
 * - пакетный обмен через QDataStream;
 * - режим 1: ручная отправка пакетов;
 * - режим 2: периодическая отправка пакетов по QTimer;
 * - вывод изменений состояния сокета в qDebug().
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /*!
     * \brief Конструирует главное окно клиента.
     * \param parent Родительский виджет.
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /*!
     * \brief Разрушает главное окно клиента.
     */
    ~MainWindow() override;

protected:
    /*!
     * \brief Обрабатывает закрытие окна.
     * \param event Событие закрытия окна.
     *
     * \details
     * Перед закрытием сохраняет настройки и корректно завершает соединение,
     * если сокет ещё активен.
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /*!
     * \brief Обрабатывает нажатие кнопки подключения.
     */
    void onConnectClicked();

    /*!
     * \brief Обрабатывает нажатие кнопки отключения.
     */
    void onDisconnectClicked();

    /*!
     * \brief Обрабатывает нажатие кнопки отправки/запуска режима отправки.
     */
    void onWriteClicked();

    /*!
     * \brief Обрабатывает нажатие кнопки остановки периодической отправки.
     */
    void onStopClicked();

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
     * \brief Обрабатывает периодическое срабатывание таймера отправки.
     */
    void onSendTimerTimeout();

private:
    /*!
     * \brief Выполняет базовую инициализацию интерфейса.
     */
    void initializeUi();

    /*!
     * \brief Настраивает сигналы и слоты интерфейса, сокета и таймера.
     */
    void connectSignals();

    /*!
     * \brief Загружает настройки клиента из QSettings.
     */
    void loadSettings();

    /*!
     * \brief Сохраняет настройки клиента в QSettings.
     */
    void saveSettings();

    /*!
     * \brief Добавляет строку в лог клиента.
     * \param message Текст сообщения.
     */
    void appendLog(const QString &message);

    /*!
     * \brief Обновляет доступность элементов управления в зависимости от состояния сокета и режима.
     */
    void updateConnectionControls();

    /*!
     * \brief Проверяет и разбирает параметры подключения из интерфейса.
     * \param address Выходной адрес сервера.
     * \param port Выходной порт сервера.
     * \return true, если параметры корректны, иначе false.
     */
    bool tryGetConnectionParameters(QHostAddress &address, quint16 &port) const;

    /*!
     * \brief Проверяет и разбирает timeout из интерфейса.
     * \param timeoutMs Выходной timeout в миллисекундах.
     * \return true, если timeout корректен, иначе false.
     */
    bool tryGetTimeout(int &timeoutMs) const;

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
     * \brief Обрабатывает накопленный входной буфер сокета.
     */
    void processSocketBuffer();

    /*!
     * \brief Формирует кадр клиентского запроса.
     * \param number Числовое поле пакета.
     * \param text Строковое поле пакета.
     * \return Готовый бинарный кадр для отправки серверу.
     */
    QByteArray buildRequestFrame(quint32 number, const QString &text) const;

    /*!
     * \brief Пытается извлечь один полный кадр из накопленного буфера.
     * \param buffer Буфер входных байтов.
     * \param pendingBlockSize Ожидаемый размер полезной нагрузки.
     * \param payload Выходная полезная нагрузка кадра.
     * \return true, если удалось извлечь полный кадр, иначе false.
     */
    bool tryExtractFrame(QByteArray &buffer,
                         quint32 &pendingBlockSize,
                         QByteArray &payload);

    /*!
     * \brief Разбирает полезную нагрузку серверного ответа.
     * \param payload Бинарная полезная нагрузка.
     * \param number Выходное числовое поле.
     * \param text Выходное строковое поле.
     * \param serverTime Выходное поле времени сервера.
     * \return true, если пакет успешно разобран, иначе false.
     */
    bool parseResponsePayload(const QByteArray &payload,
                              quint32 &number,
                              QString &text,
                              QTime &serverTime) const;

    /*!
     * \brief Отправляет один пакет серверу.
     * \param text Строковое поле пакета.
     * \return true, если пакет поставлен в очередь на отправку, иначе false.
     */
    bool sendPacket(const QString &text);

private:
    Ui::MainWindow *ui = nullptr;
    QTcpSocket *socket_ = nullptr;
    QTimer *sendTimer_ = nullptr;
    QByteArray socketReadBuffer_;
    quint32 pendingServerBlockSize_ = 0;
    quint32 nextRequestNumber_ = 1;
    QString periodicMessageText_;
};

#endif // MAINWINDOW_H
