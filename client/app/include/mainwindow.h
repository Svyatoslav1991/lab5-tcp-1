#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractSocket>
#include <QHostAddress>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;
class QTcpSocket;

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
 * - вывод изменений состояния сокета в qDebug().
 *
 * Кнопки write и stop пока зарезервированы под задание 3.
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
     * \brief Обрабатывает успешное подключение к серверу.
     */
    void onSocketConnected();

    /*!
     * \brief Обрабатывает отключение от сервера.
     */
    void onSocketDisconnected();

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

private:
    /*!
     * \brief Выполняет базовую инициализацию интерфейса.
     */
    void initializeUi();

    /*!
     * \brief Настраивает сигналы и слоты интерфейса и сокета.
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
     * \brief Обновляет доступность элементов управления в зависимости от состояния сокета.
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

private:
    Ui::MainWindow *ui = nullptr;
    QTcpSocket *socket_ = nullptr;
};

#endif // MAINWINDOW_H
