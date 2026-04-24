#include <QtTest>

#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>

#include "clientsettings.h"

/*!
 * \class TestClientSettings
 * \brief Набор unit-тестов для ClientSettings.
 *
 * \details
 * Тесты проверяют:
 * - загрузку значений по умолчанию;
 * - сохранение и повторную загрузку настроек;
 * - trim строковых полей при чтении;
 * - fallback режима на SessionMode::Single для неизвестного значения.
 */
class TestClientSettings : public QObject
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
     * \brief Выполняет общую настройку окружения тестов.
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
     * \brief Проверяет trim для address, port и timeout при чтении из QSettings.
     */
    void load_trimsStringFields();

    /*!
     * \brief Проверяет fallback на SessionMode::Single для неизвестного строкового значения режима.
     */
    void load_returnsSingleForUnknownSessionMode();
};

//--------------------------------------------------------------------------

void TestClientSettings::clearSettings() const
{
    QSettings settings;
    settings.clear();
    settings.sync();
}

//--------------------------------------------------------------------------

void TestClientSettings::initTestCase()
{
    QVERIFY2(settingsDir_.isValid(), "Не удалось создать временную директорию для настроек");

    QCoreApplication::setOrganizationName(QStringLiteral("TestOrg"));
    QCoreApplication::setApplicationName(QStringLiteral("TestClientSettings"));

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir_.path());
}

//--------------------------------------------------------------------------

void TestClientSettings::init()
{
    clearSettings();
}

//--------------------------------------------------------------------------

void TestClientSettings::load_returnsDefaults_whenSettingsAreEmpty()
{
    ClientSettings clientSettings;

    const ClientSettingsData data = clientSettings.load();

    QCOMPARE(data.address, QStringLiteral("127.0.0.1"));
    QCOMPARE(data.port, QStringLiteral("8888"));
    QCOMPARE(data.timeout, QStringLiteral("1000"));
    QCOMPARE(data.sessionMode, SessionMode::Single);
}

//--------------------------------------------------------------------------

void TestClientSettings::save_thenLoad_preservesAllFields()
{
    ClientSettings clientSettings;

    ClientSettingsData savedData;
    savedData.address = QStringLiteral("192.168.10.15");
    savedData.port = QStringLiteral("50001");
    savedData.timeout = QStringLiteral("2500");
    savedData.sessionMode = SessionMode::Short;

    clientSettings.save(savedData);

    const ClientSettingsData loadedData = clientSettings.load();

    QCOMPARE(loadedData.address, savedData.address);
    QCOMPARE(loadedData.port, savedData.port);
    QCOMPARE(loadedData.timeout, savedData.timeout);
    QCOMPARE(loadedData.sessionMode, savedData.sessionMode);
}

//--------------------------------------------------------------------------

void TestClientSettings::load_trimsStringFields()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("client"));
    settings.setValue(QStringLiteral("address"), QStringLiteral("  10.0.0.42  "));
    settings.setValue(QStringLiteral("port"), QStringLiteral("  60000  "));
    settings.setValue(QStringLiteral("timeout"), QStringLiteral("  1500  "));
    settings.setValue(QStringLiteral("session_mode"), QStringLiteral("long"));
    settings.endGroup();
    settings.sync();

    ClientSettings clientSettings;
    const ClientSettingsData data = clientSettings.load();

    QCOMPARE(data.address, QStringLiteral("10.0.0.42"));
    QCOMPARE(data.port, QStringLiteral("60000"));
    QCOMPARE(data.timeout, QStringLiteral("1500"));
    QCOMPARE(data.sessionMode, SessionMode::Long);
}

//--------------------------------------------------------------------------

void TestClientSettings::load_returnsSingleForUnknownSessionMode()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("client"));
    settings.setValue(QStringLiteral("address"), QStringLiteral("127.0.0.1"));
    settings.setValue(QStringLiteral("port"), QStringLiteral("8888"));
    settings.setValue(QStringLiteral("timeout"), QStringLiteral("1000"));
    settings.setValue(QStringLiteral("session_mode"), QStringLiteral("unsupported_mode"));
    settings.endGroup();
    settings.sync();

    ClientSettings clientSettings;
    const ClientSettingsData data = clientSettings.load();

    QCOMPARE(data.sessionMode, SessionMode::Single);
}

QTEST_APPLESS_MAIN(TestClientSettings)

#include "test_clientsettings.moc"
