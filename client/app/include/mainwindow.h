#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QMainWindow>

#include "clientsettings.h"
#include "sessionmode.h"

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
 * Поддерживаются:
 * - подключение и отключение по запросу пользователя;
 * - пакетный обмен данными через PacketProtocol;
 * - три режима работы клиента;
 * - сохранение пользовательских настроек через ClientSettings.
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
     * \brief Обрабатывает нажатие кнопки отправки или запуска выбранного режима.
     */
    void onWriteClicked();

    /*!
     * \brief Обрабатывает нажатие кнопки остановки периодического режима.
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
     * \brief Обрабатывает срабатывание таймера периодического режима.
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
     * \brief Загружает настройки клиента из ClientSettings.
     */
    void loadSettings();

    /*!
     * \brief Сохраняет настройки клиента через ClientSettings.
     */
    void saveSettings();

    /*!
     * \brief Собирает DTO настроек из текущего состояния UI.
     * \return DTO с настройками клиента.
     */
    ClientSettingsData buildSettingsData() const;

    /*!
     * \brief Применяет DTO настроек к UI.
     * \param data DTO с настройками клиента.
     */
    void applySettingsData(const ClientSettingsData &data);

    /*!
     * \brief Добавляет строку в лог клиента.
     * \param message Текст сообщения.
     */
    void appendLog(const QString &message);

    /*!
     * \brief Возвращает текущий режим работы клиента.
     * \return Текущий режим работы.
     */
    SessionMode currentSessionMode() const;

    /*!
     * \brief Устанавливает текущий режим работы клиента в интерфейсе.
     * \param mode Новый режим.
     */
    void setSessionMode(SessionMode mode);

    /*!
     * \brief Обновляет доступность элементов управления.
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
     * \brief Сбрасывает состояние накопленного входного буфера.
     */
    void resetIncomingFrameState();

    /*!
     * \brief Обрабатывает накопленный входной буфер сокета.
     */
    void processSocketBuffer();

    /*!
     * \brief Отправляет один пакет серверу.
     * \param text Строковое поле пакета.
     * \return true, если пакет поставлен в очередь на отправку, иначе false.
     */
    bool sendPacket(const QString &text);

    /*!
     * \brief Запускает периодический режим 2.
     * \param timeoutMs Интервал таймера в миллисекундах.
     * \param text Текст пакета.
     */
    void startLongMode(int timeoutMs, const QString &text);

    /*!
     * \brief Запускает периодический режим 3.
     * \param timeoutMs Интервал таймера в миллисекундах.
     * \param text Текст пакета.
     */
    void startShortMode(int timeoutMs, const QString &text);

    /*!
     * \brief Останавливает периодический режим.
     * \param logMessage Сообщение в лог. Если пустое, лог не пополняется.
     */
    void stopPeriodicMode(const QString &logMessage = QString());

    /*!
     * \brief Запускает один цикл режима 3.
     */
    void startShortModeCycle();

private:
    Ui::MainWindow *ui = nullptr;          /*!< Сгенерированный UI-объект. */
    QTcpSocket *socket_ = nullptr;         /*!< Клиентский TCP-сокет. */
    QTimer *sendTimer_ = nullptr;          /*!< Таймер периодических режимов 2 и 3. */
    ClientSettings clientSettings_;        /*!< Сервис загрузки и сохранения настроек клиента. */
    QByteArray socketReadBuffer_;          /*!< Накопленный входной буфер TCP-потока. */
    quint32 pendingServerBlockSize_ = 0;   /*!< Ожидаемый размер текущего входного кадра. */
    quint32 nextRequestNumber_ = 1;        /*!< Номер следующего исходящего пакета. */
    QString periodicMessageText_;          /*!< Текст для периодической отправки в режимах 2 и 3. */
    bool shortModeWaitingForResponse_ = false; /*!< Флаг ожидания ответа в текущем цикле режима 3. */
};

#endif // MAINWINDOW_H
