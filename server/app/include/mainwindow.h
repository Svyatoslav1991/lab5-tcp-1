#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHostAddress>
#include <QMainWindow>

#include "servercontroller.h"
#include "serversettings.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;

/*!
 * \class MainWindow
 * \brief Главное окно TCP-сервера для лабораторной работы №5.
 *
 * \details
 * Класс реализует UI-слой сервера:
 * - инициализацию интерфейса;
 * - валидацию параметров прослушивания;
 * - загрузку и сохранение настроек;
 * - передачу пользовательских команд в ServerController;
 * - отображение логов и обновление состояния элементов управления.
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
     * \brief Загружает настройки сервера из ServerSettings.
     */
    void loadSettings();

    /*!
     * \brief Сохраняет настройки сервера через ServerSettings.
     */
    void saveSettings();

    /*!
     * \brief Собирает DTO настроек из текущего состояния UI.
     * \return DTO с настройками сервера.
     */
    ServerSettingsData buildSettingsData() const;

    /*!
     * \brief Применяет DTO настроек к UI.
     * \param data DTO с настройками сервера.
     */
    void applySettingsData(const ServerSettingsData &data);

    /*!
     * \brief Добавляет строку в лог сервера.
     * \param message Текст сообщения.
     */
    void appendLog(const QString &message);

    /*!
     * \brief Обновляет доступность элементов управления в зависимости от состояния прослушивания.
     */
    void updateListeningControls();

    /*!
     * \brief Проверяет и разбирает параметры прослушивания из интерфейса.
     * \param address Выходной IP-адрес.
     * \param port Выходной порт.
     * \return true, если параметры корректны, иначе false.
     */
    bool tryGetListenParameters(QHostAddress &address, quint16 &port) const;

private:
    Ui::MainWindow *ui = nullptr;               /*!< Сгенерированный UI-объект. */
    ServerController *serverController_ = nullptr; /*!< Контроллер сетевого поведения сервера. */
    ServerSettings serverSettings_;             /*!< Сервис загрузки и сохранения настроек сервера. */
};

#endif // MAINWINDOW_H
