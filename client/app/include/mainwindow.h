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
 * \brief Главное окно TCP-клиента для лабораторной работы №5.
 *
 * \details
 * На текущем этапе класс реализует базовую инициализацию интерфейса:
 * - значения по умолчанию для адреса, порта и timeout;
 * - настройку валидаторов для полей ввода;
 * - загрузку и сохранение пользовательских настроек через QSettings;
 * - стартовое состояние элементов управления.
 *
 * Сетевая логика на основе QTcpSocket будет добавлена следующим шагом.
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
     * Перед закрытием сохраняет текущие пользовательские настройки.
     */
    void closeEvent(QCloseEvent *event) override;

private:
    /*!
     * \brief Выполняет базовую инициализацию интерфейса.
     *
     * \details
     * Метод:
     * - задаёт значения по умолчанию;
     * - настраивает валидаторы;
     * - переводит лог в режим только для чтения;
     * - устанавливает стартовое состояние кнопок;
     * - загружает сохранённые настройки.
     */
    void initializeUi();

    /*!
     * \brief Настраивает сигналы интерфейса.
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

private:
    Ui::MainWindow *ui = nullptr;
};

#endif // MAINWINDOW_H
