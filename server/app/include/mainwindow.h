#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

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

private:
    /*!
     * \brief Выполняет базовую инициализацию интерфейса.
     */
    void initializeUi();

private:
    Ui::MainWindow *ui = nullptr;
};

#endif // MAINWINDOW_H