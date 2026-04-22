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
const QString kDefaultTimeout = QStringLiteral("1000");

const QString kSettingsGroup = QStringLiteral("client");
const QString kAddressKey = QStringLiteral("address");
const QString kPortKey = QStringLiteral("port");
const QString kTimeoutKey = QStringLiteral("timeout");
const QString kSessionModeKey = QStringLiteral("session_mode");

const QString kSingleMode = QStringLiteral("single");
const QString kLongMode = QStringLiteral("long");
const QString kShortMode = QStringLiteral("short");
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
    const QString ipv4BytePattern =
        QStringLiteral("(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)");

    const QString ipv4AddressPattern =
        QStringLiteral("^%1\\.%1\\.%1\\.%1$").arg(ipv4BytePattern);

    const QString portPattern =
        QStringLiteral("^(6553[0-5]|655[0-2]\\d|65[0-4]\\d{2}|6[0-4]\\d{3}|[1-5]\\d{4}|[1-9]\\d{0,3})$");

    const QString timeoutPattern =
        QStringLiteral("^[1-9]\\d{0,5}$");

    auto *addressValidator = new QRegularExpressionValidator(
        QRegularExpression(ipv4AddressPattern),
        this
    );

    auto *portValidator = new QRegularExpressionValidator(
        QRegularExpression(portPattern),
        this
    );

    auto *timeoutValidator = new QRegularExpressionValidator(
        QRegularExpression(timeoutPattern),
        this
    );

    ui->address_lineEdit->setValidator(addressValidator);
    ui->port_lineEdit->setValidator(portValidator);
    ui->timeout_lineEdit->setValidator(timeoutValidator);

    ui->address_lineEdit->setText(kDefaultAddress);
    ui->port_lineEdit->setText(kDefaultPort);
    ui->timeout_lineEdit->setText(kDefaultTimeout);

    ui->clientLog_plainTextEdit->setReadOnly(true);

    ui->single_radioButton->setChecked(true);

    ui->disconnect_pushButton->setEnabled(false);
    ui->write_pushButton->setEnabled(false);
    ui->stop_pushButton->setEnabled(false);

    loadSettings();
}

//--------------------------------------------------------------------------

void MainWindow::connectSignals()
{
    connect(ui->address_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->port_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->timeout_lineEdit, &QLineEdit::editingFinished,
            this, &MainWindow::saveSettings);

    connect(ui->single_radioButton, &QRadioButton::toggled,
            this, &MainWindow::saveSettings);

    connect(ui->long_radioButton, &QRadioButton::toggled,
            this, &MainWindow::saveSettings);

    connect(ui->short_radioButton, &QRadioButton::toggled,
            this, &MainWindow::saveSettings);
}

//--------------------------------------------------------------------------

void MainWindow::loadSettings()
{
    QSettings settings;

    settings.beginGroup(kSettingsGroup);

    const QString address = settings.value(kAddressKey, kDefaultAddress).toString().trimmed();
    const QString port = settings.value(kPortKey, kDefaultPort).toString().trimmed();
    const QString timeout = settings.value(kTimeoutKey, kDefaultTimeout).toString().trimmed();
    const QString sessionMode = settings.value(kSessionModeKey, kSingleMode).toString();

    settings.endGroup();

    ui->address_lineEdit->setText(address);
    ui->port_lineEdit->setText(port);
    ui->timeout_lineEdit->setText(timeout);

    if (!ui->address_lineEdit->hasAcceptableInput()) {
        ui->address_lineEdit->setText(kDefaultAddress);
    }

    if (!ui->port_lineEdit->hasAcceptableInput()) {
        ui->port_lineEdit->setText(kDefaultPort);
    }

    if (!ui->timeout_lineEdit->hasAcceptableInput()) {
        ui->timeout_lineEdit->setText(kDefaultTimeout);
    }

    if (sessionMode == kLongMode) {
        ui->long_radioButton->setChecked(true);
    } else if (sessionMode == kShortMode) {
        ui->short_radioButton->setChecked(true);
    } else {
        ui->single_radioButton->setChecked(true);
    }
}

//--------------------------------------------------------------------------

void MainWindow::saveSettings()
{
    QString sessionMode = kSingleMode;

    if (ui->long_radioButton->isChecked()) {
        sessionMode = kLongMode;
    } else if (ui->short_radioButton->isChecked()) {
        sessionMode = kShortMode;
    }

    QSettings settings;

    settings.beginGroup(kSettingsGroup);

    if (ui->address_lineEdit->hasAcceptableInput()) {
        settings.setValue(kAddressKey, ui->address_lineEdit->text().trimmed());
    }

    if (ui->port_lineEdit->hasAcceptableInput()) {
        settings.setValue(kPortKey, ui->port_lineEdit->text().trimmed());
    }

    if (ui->timeout_lineEdit->hasAcceptableInput()) {
        settings.setValue(kTimeoutKey, ui->timeout_lineEdit->text().trimmed());
    }

    settings.setValue(kSessionModeKey, sessionMode);

    settings.endGroup();
}
