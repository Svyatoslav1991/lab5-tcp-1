#include <QApplication>
#include <QCoreApplication>

#include "mainwindow.h"

/*!
 * \brief Точка входа в приложение TCP-клиента.
 * \param argc Количество аргументов командной строки.
 * \param argv Аргументы командной строки.
 * \return Код завершения приложения.
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("Myshkovskiy Svyatoslav"));
    QCoreApplication::setApplicationName(QStringLiteral("lab5_tcp_client"));

    MainWindow window;
    window.show();

    return app.exec();
}
