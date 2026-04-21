#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>

namespace
{
const QString kDefaultAddress = QStringLiteral("127.0.0.1");
const QString kDefaultPort = QStringLiteral("8888");

const QString kSettingsGroup = QStringLiteral("server");
const QString kAddressKey = QStringLiteral("address");
const QString kPortKey = QStringLiteral("port");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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
    QMainWindow::closeEvent(event);
}

//--------------------------------------------------------------------------

void MainWindow::initializeUi()
{
    ui->address_lineEdit->setText(kDefaultAddress);
    ui->port_lineEdit->setText(kDefaultPort);

    const QString ipv4BytePattern =
        QStringLiteral("(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)");

    const QString ipv4AddressPattern =
        QStringLiteral("^%1\\.%1\\.%1\\.%1$").arg(ipv4BytePattern);

    auto *addressValidator = new QRegularExpressionValidator(
        QRegularExpression(ipv4AddressPattern),
        this
    );

    ui->address_lineEdit->setValidator(addressValidator);

    const QString portPattern =
        QStringLiteral("^(6553[0-5]|655[0-2]\\d|65[0-4]\\d{2}|6[0-4]\\d{3}|[1-5]\\d{4}|[1-9]\\d{0,3})$");

    auto *portValidator = new QRegularExpressionValidator(
        QRegularExpression(portPattern),
        this
    );

    ui->port_lineEdit->setValidator(portValidator);

    ui->serverLog_plainTextEdit->setReadOnly(true);

    loadSettings();
}

//--------------------------------------------------------------------------

void MainWindow::connectSignals()
{
    connect(ui->address_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->port_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);
}

//--------------------------------------------------------------------------

void MainWindow::loadSettings()
{
    QSettings settings;

    settings.beginGroup(kSettingsGroup);

    const QString savedAddress =
        settings.value(kAddressKey, kDefaultAddress).toString().trimmed();

    const QString savedPort =
        settings.value(kPortKey, kDefaultPort).toString().trimmed();

    settings.endGroup();

    ui->address_lineEdit->setText(savedAddress);
    ui->port_lineEdit->setText(savedPort);

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
    const QString address = ui->address_lineEdit->text().trimmed();
    const QString port = ui->port_lineEdit->text().trimmed();

    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    if (!address.isEmpty() && ui->address_lineEdit->hasAcceptableInput()) {
        settings.setValue(kAddressKey, address);
    }

    if (!port.isEmpty() && ui->port_lineEdit->hasAcceptableInput()) {
        settings.setValue(kPortKey, port);
    }

    settings.endGroup();
}
