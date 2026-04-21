#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QAbstractSocket>
#include <QHostAddress>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;
class QTcpServer;
class QTcpSocket;

/*!
 * \class MainWindow
 * \brief Главное окно TCP-сервера для лабораторной работы №5.
 *
 * \details
 * Класс реализует однопоточный асинхронный TCP-сервер с графическим интерфейсом.
 * Пользователь задаёт адрес и порт, после чего сервер начинает прослушивание
 * входящих соединений через QTcpServer.
 *
 * На текущем этапе реализованы:
 * - запуск и остановка прослушивания;
 * - приём входящего подключения;
 * - обработка отключения клиента;
 * - отслеживание изменения состояния сокета клиента;
 * - сохранение адреса и порта через QSettings.
 *
 * Для упрощения первого этапа сервер поддерживает одного активного клиента.
 * Дополнительные входящие подключения отклоняются.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /*!
     * \brief Конструирует главное окно сервера.
     * \param parent Родительский виджет.
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /*!
     * \brief Разрушает главное окно сервера.
     */
    ~MainWindow() override;

protected:
    /*!
     * \brief Обрабатывает закрытие окна.
     * \param event Событие закрытия окна.
     *
     * \details
     * Перед закрытием сохраняет пользовательские настройки и корректно
     * останавливает сервер при необходимости.
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /*!
     * \brief Обрабатывает нажатие кнопки запуска прослушивания.
     */
    void onStartListeningClicked();

    /*!
     * \brief Обрабатывает нажатие кнопки остановки прослушивания.
     */
    void onStopListeningClicked();

    /*!
     * \brief Обрабатывает появление нового входящего соединения.
     */
    void onNewConnection();

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
     * \brief Выполняет базовую инициализацию интерфейса.
     *
     * \details
     * Метод:
     * - задаёт значения по умолчанию;
     * - настраивает валидаторы;
     * - переводит лог в режим только для чтения;
     * - загружает сохранённые настройки;
     * - обновляет состояние элементов управления.
     */
    void initializeUi();

    /*!
     * \brief Настраивает сигналы и слоты интерфейса и серверных объектов.
     */
    void connectSignals();

    /*!
     * \brief Загружает адрес и порт из QSettings.
     */
    void loadSettings();

    /*!
     * \brief Сохраняет адрес и порт в QSettings.
     */
    void saveSettings();

    /*!
     * \brief Добавляет сообщение в лог сервера.
     * \param message Текст сообщения.
     */
    void appendLog(const QString &message);

    /*!
     * \brief Обновляет доступность элементов интерфейса в зависимости от состояния прослушивания.
     * \param isListening true, если сервер прослушивает порт.
     */
    void updateListeningControls(bool isListening);

    /*!
     * \brief Проверяет и разбирает параметры прослушивания из интерфейса.
     * \param address Выходной IP-адрес.
     * \param port Выходной порт.
     * \return true, если параметры корректны, иначе false.
     */
    bool tryGetListenParameters(QHostAddress &address, quint16 &port) const;

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
     * \brief Корректно отключает и удаляет активный клиентский сокет.
     */
    void resetClientSocket();

private:
    Ui::MainWindow *ui = nullptr;
    QTcpServer *server_ = nullptr;
    QPointer<QTcpSocket> clientSocket_;
};

#endif // MAINWINDOW_H
