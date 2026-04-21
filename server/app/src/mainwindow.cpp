#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initializeUi();
}


MainWindow::~MainWindow()
{
    delete ui;
}


//--------------------------------------------------------------------------

void MainWindow::initializeUi()
{
    setWindowTitle(QStringLiteral("Lab5 Tcp Server"));
}