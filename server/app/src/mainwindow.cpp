#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStatusBar>

namespace
{
const QString kDefaultAddress = QStringLiteral("127.0.0.1");
const QString kDefaultPort = QStringLiteral("8888");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serverController_(new ServerController(this))
{
    ui->setupUi(this);

    initializeUi();
    connectSignals();
}

//--------------------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}

//--------------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    serverController_->shutdown();

    QMainWindow::closeEvent(event);
}

//--------------------------------------------------------------------------

void MainWindow::initializeUi()
{
    const QString ipv4BytePattern =
        QStringLiteral("(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)");

    const QString ipv4Pattern =
        QStringLiteral("^%1\\.%1\\.%1\\.%1$").arg(ipv4BytePattern);

    const QString portPattern =
        QStringLiteral("^(6553[0-5]|655[0-2]\\d|65[0-4]\\d{2}|6[0-4]\\d{3}|[1-5]\\d{4}|[1-9]\\d{0,3})$");

    auto *addressValidator = new QRegularExpressionValidator(
        QRegularExpression(ipv4Pattern),
        this
    );

    auto *portValidator = new QRegularExpressionValidator(
        QRegularExpression(portPattern),
        this
    );

    ui->address_lineEdit->setValidator(addressValidator);
    ui->port_lineEdit->setValidator(portValidator);

    ui->serverLog_plainTextEdit->setReadOnly(true);

    ui->address_lineEdit->setText(kDefaultAddress);
    ui->port_lineEdit->setText(kDefaultPort);

    loadSettings();
    updateListeningControls();

    statusBar()->showMessage(QStringLiteral("Сервер не запущен"));
}

//--------------------------------------------------------------------------

void MainWindow::connectSignals()
{
    connect(ui->startListening_button, &QPushButton::clicked,
            this, &MainWindow::onStartListeningClicked);

    connect(ui->stopListening_button, &QPushButton::clicked,
            this, &MainWindow::onStopListeningClicked);

    connect(ui->address_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->port_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(serverController_, &ServerController::logMessage,
            this, &MainWindow::appendLog);

    connect(serverController_, &ServerController::stateChanged,
            this, &MainWindow::updateListeningControls);
}

//--------------------------------------------------------------------------

void MainWindow::loadSettings()
{
    applySettingsData(serverSettings_.load());

    if (!ui->address_lineEdit->hasAcceptableInput()) {
        ui->address_lineEdit->setText(kDefaultAddress);
    }

    if (!ui->port_lineEdit->hasAcceptableInput()) {
        ui->port_lineEdit->setText(kDefaultPort);
    }
}

//--------------------------------------------------------------------------

void MainWindow::saveSettings()
{
    serverSettings_.save(buildSettingsData());
}

//--------------------------------------------------------------------------

ServerSettingsData MainWindow::buildSettingsData() const
{
    ServerSettingsData data;
    data.address = ui->address_lineEdit->text().trimmed();
    data.port = ui->port_lineEdit->text().trimmed();

    return data;
}

//--------------------------------------------------------------------------

void MainWindow::applySettingsData(const ServerSettingsData &data)
{
    ui->address_lineEdit->setText(data.address);
    ui->port_lineEdit->setText(data.port);
}

//--------------------------------------------------------------------------

void MainWindow::appendLog(const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    ui->serverLog_plainTextEdit->appendPlainText(
        QStringLiteral("[%1] %2").arg(timestamp, message)
    );
}

//--------------------------------------------------------------------------

void MainWindow::updateListeningControls()
{
    const bool isListening = serverController_->isListening();

    ui->startListening_button->setEnabled(!isListening);
    ui->stopListening_button->setEnabled(isListening);

    ui->address_lineEdit->setEnabled(!isListening);
    ui->port_lineEdit->setEnabled(!isListening);

    if (isListening) {
        statusBar()->showMessage(QStringLiteral("Сервер прослушивает порт"));
    } else {
        statusBar()->showMessage(QStringLiteral("Сервер не запущен"));
    }
}

//--------------------------------------------------------------------------

bool MainWindow::tryGetListenParameters(QHostAddress &address, quint16 &port) const
{
    if (!ui->address_lineEdit->hasAcceptableInput() ||
        !ui->port_lineEdit->hasAcceptableInput()) {
        return false;
    }

    const QString addressText = ui->address_lineEdit->text().trimmed();
    const QString portText = ui->port_lineEdit->text().trimmed();

    if (!address.setAddress(addressText)) {
        return false;
    }

    bool isOk = false;
    const uint parsedPort = portText.toUInt(&isOk);

    if (!isOk || parsedPort == 0 || parsedPort > 65535) {
        return false;
    }

    port = static_cast<quint16>(parsedPort);
    return true;
}

//--------------------------------------------------------------------------

void MainWindow::onStartListeningClicked()
{
    QHostAddress address;
    quint16 port = 0;

    if (!tryGetListenParameters(address, port)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Некорректные параметры"),
            QStringLiteral("Введите корректные адрес и порт сервера.")
        );
        return;
    }

    saveSettings();

    if (!serverController_->startListening(address, port)) {
        const QString errorText = serverController_->lastErrorString();

        appendLog(QStringLiteral("Ошибка запуска прослушивания: %1").arg(errorText));

        QMessageBox::critical(
            this,
            QStringLiteral("Ошибка запуска сервера"),
            QStringLiteral("Не удалось запустить прослушивание:\n%1").arg(errorText)
        );
        return;
    }

    updateListeningControls();
}

//--------------------------------------------------------------------------

void MainWindow::onStopListeningClicked()
{
    serverController_->stopListening();
    updateListeningControls();
}
