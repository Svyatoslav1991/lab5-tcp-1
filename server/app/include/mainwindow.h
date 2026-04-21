#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
 * На текущем этапе класс представляет собой базовый каркас оконного приложения.
 * Логика прослушивания порта, обработка подключений и работа с QTcpServer/QTcpSocket
 * будут добавлены на следующих шагах.
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
     * \param event Событие закрытия.
     *
     * \details
     * Перед закрытием окна сохраняет текущие настройки интерфейса.
     */
    void closeEvent(QCloseEvent *event) override;

private:
    /*!
     * \brief Выполняет базовую инициализацию интерфейса.
     *
     * \details
     * На данном этапе метод:
     * - задаёт значения по умолчанию для адреса и порта;
     * - настраивает валидаторы для полей ввода;
     * - переводит поле лога в режим только для чтения;
     * - загружает ранее сохранённые настройки.
     */
    void initializeUi();

    /*!
     * \brief Настраивает сигналы интерфейса.
     */
    void connectSignals();

    /*!
     * \brief Загружает сохранённые адрес и порт из QSettings.
     */
    void loadSettings();

    /*!
     * \brief Сохраняет текущие адрес и порт в QSettings.
     *
     * \details
     * Сохраняются только корректные значения, прошедшие валидацию.
     */
    void saveSettings();

private:
    Ui::MainWindow *ui = nullptr;
};

#endif // MAINWINDOW_H
