#include <QtTest>

#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>

#include "serversettings.h"

/*!
 * \class TestServerSettings
 * \brief Набор unit-тестов для ServerSettings.
 *
 * \details
 * Тесты проверяют:
 * - загрузку значений по умолчанию;
 * - сохранение и повторную загрузку настроек;
 * - trim строковых полей при чтении.
 */
class TestServerSettings : public QObject
{
    Q_OBJECT

private:
    /*!
     * \brief Временная директория для изолированных ini-настроек тестов.
     */
    QTemporaryDir settingsDir_;

private:
    /*!
     * \brief Очищает сохранённые настройки тестового приложения.
     */
    void clearSettings() const;

private slots:
    /*!
     * \brief Подготавливает окружение QSettings для тестов.
     */
    void initTestCase();

    /*!
     * \brief Очищает настройки перед каждым тестом.
     */
    void init();

    /*!
     * \brief Проверяет загрузку значений по умолчанию при отсутствии сохранённых настроек.
     */
    void load_returnsDefaults_whenSettingsAreEmpty();

    /*!
     * \brief Проверяет сохранение и повторную загрузку всех полей DTO.
     */
    void save_thenLoad_preservesAllFields();

    /*!
     * \brief Проверяет trim для address и port при чтении из QSettings.
     */
    void load_trimsStringFields();
};

//--------------------------------------------------------------------------

void TestServerSettings::clearSettings() const
{
    QSettings settings;
    settings.clear();
    settings.sync();
}

//--------------------------------------------------------------------------

void TestServerSettings::initTestCase()
{
    QVERIFY2(settingsDir_.isValid(), "Не удалось создать временную директорию для настроек");

    QCoreApplication::setOrganizationName(QStringLiteral("TestOrg"));
    QCoreApplication::setApplicationName(QStringLiteral("TestServerSettings"));

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir_.path());
}

//--------------------------------------------------------------------------

void TestServerSettings::init()
{
    clearSettings();
}

//--------------------------------------------------------------------------

void TestServerSettings::load_returnsDefaults_whenSettingsAreEmpty()
{
    ServerSettings serverSettings;

    const ServerSettingsData data = serverSettings.load();

    QCOMPARE(data.address, QStringLiteral("127.0.0.1"));
    QCOMPARE(data.port, QStringLiteral("8888"));
}

//--------------------------------------------------------------------------

void TestServerSettings::save_thenLoad_preservesAllFields()
{
    ServerSettings serverSettings;

    ServerSettingsData savedData;
    savedData.address = QStringLiteral("192.168.0.55");
    savedData.port = QStringLiteral("50000");

    serverSettings.save(savedData);

    const ServerSettingsData loadedData = serverSettings.load();

    QCOMPARE(loadedData.address, savedData.address);
    QCOMPARE(loadedData.port, savedData.port);
}

//--------------------------------------------------------------------------

void TestServerSettings::load_trimsStringFields()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("server"));
    settings.setValue(QStringLiteral("address"), QStringLiteral("  10.0.0.42  "));
    settings.setValue(QStringLiteral("port"), QStringLiteral("  60000  "));
    settings.endGroup();
    settings.sync();

    ServerSettings serverSettings;
    const ServerSettingsData data = serverSettings.load();

    QCOMPARE(data.address, QStringLiteral("10.0.0.42"));
    QCOMPARE(data.port, QStringLiteral("60000"));
}

QTEST_APPLESS_MAIN(TestServerSettings)

#include "test_serversettings.moc"
