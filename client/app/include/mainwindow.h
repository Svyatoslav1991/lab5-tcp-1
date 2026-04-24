#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHostAddress>
#include <QMainWindow>

#include "clientcontroller.h"
#include "clientsettings.h"
#include "sessionmode.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;

/*!
 * \class MainWindow
 * \brief Главное окно TCP-клиента для лабораторной работы №5.
 *
 * \details
 * Класс реализует UI-слой клиента:
 * - инициализацию интерфейса;
 * - валидацию введённых пользователем параметров;
 * - загрузку и сохранение настроек;
 * - передачу пользовательских команд в ClientController;
 * - отображение логов и обновление состояния элементов управления.
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

private:
    /*!
     * \brief Выполняет базовую инициализацию интерфейса.
     */
    void initializeUi();

    /*!
     * \brief Настраивает сигналы и слоты интерфейса и контроллера.
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

private:
    Ui::MainWindow *ui = nullptr;          /*!< Сгенерированный UI-объект. */
    ClientController *clientController_ = nullptr; /*!< Контроллер сетевого поведения клиента. */
    ClientSettings clientSettings_;        /*!< Сервис загрузки и сохранения настроек клиента. */
};

#endif // MAINWINDOW_H
