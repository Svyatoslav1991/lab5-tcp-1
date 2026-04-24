#include <QtTest>

#include "sessionmode.h"

Q_DECLARE_METATYPE(SessionMode)

/*!
 * \class TestSessionMode
 * \brief Набор unit-тестов для sessionmode.h.
 *
 * \details
 * Тесты покрывают:
 * - преобразование SessionMode в строку для QSettings;
 * - восстановление SessionMode из строкового значения;
 * - fallback-поведение для неизвестной строки.
 */
class TestSessionMode : public QObject
{
    Q_OBJECT

private slots:
    /*!
     * \brief Проверяет преобразование SessionMode::Single в строку.
     */
    void sessionModeToSettingsValue_returnsSingleForSingle();

    /*!
     * \brief Проверяет преобразование SessionMode::Long в строку.
     */
    void sessionModeToSettingsValue_returnsLongForLong();

    /*!
     * \brief Проверяет преобразование SessionMode::Short в строку.
     */
    void sessionModeToSettingsValue_returnsShortForShort();

    /*!
     * \brief Проверяет восстановление SessionMode::Single из строки "single".
     */
    void sessionModeFromSettingsValue_returnsSingleForSingleString();

    /*!
     * \brief Проверяет восстановление SessionMode::Long из строки "long".
     */
    void sessionModeFromSettingsValue_returnsLongForLongString();

    /*!
     * \brief Проверяет восстановление SessionMode::Short из строки "short".
     */
    void sessionModeFromSettingsValue_returnsShortForShortString();

    /*!
     * \brief Проверяет fallback на SessionMode::Single для неизвестной строки.
     */
    void sessionModeFromSettingsValue_returnsSingleForUnknownString();

    /*!
     * \brief Подготавливает данные для проверки round-trip преобразования.
     */
    void roundTrip_conversionPreservesMode_data();

    /*!
     * \brief Проверяет корректность round-trip преобразования для всех режимов.
     */
    void roundTrip_conversionPreservesMode();
};

//--------------------------------------------------------------------------

void TestSessionMode::sessionModeToSettingsValue_returnsSingleForSingle()
{
    QCOMPARE(sessionModeToSettingsValue(SessionMode::Single), QStringLiteral("single"));
}

//--------------------------------------------------------------------------

void TestSessionMode::sessionModeToSettingsValue_returnsLongForLong()
{
    QCOMPARE(sessionModeToSettingsValue(SessionMode::Long), QStringLiteral("long"));
}

//--------------------------------------------------------------------------

void TestSessionMode::sessionModeToSettingsValue_returnsShortForShort()
{
    QCOMPARE(sessionModeToSettingsValue(SessionMode::Short), QStringLiteral("short"));
}

//--------------------------------------------------------------------------

void TestSessionMode::sessionModeFromSettingsValue_returnsSingleForSingleString()
{
    QCOMPARE(sessionModeFromSettingsValue(QStringLiteral("single")), SessionMode::Single);
}

//--------------------------------------------------------------------------

void TestSessionMode::sessionModeFromSettingsValue_returnsLongForLongString()
{
    QCOMPARE(sessionModeFromSettingsValue(QStringLiteral("long")), SessionMode::Long);
}

//--------------------------------------------------------------------------

void TestSessionMode::sessionModeFromSettingsValue_returnsShortForShortString()
{
    QCOMPARE(sessionModeFromSettingsValue(QStringLiteral("short")), SessionMode::Short);
}

//--------------------------------------------------------------------------

void TestSessionMode::sessionModeFromSettingsValue_returnsSingleForUnknownString()
{
    QCOMPARE(sessionModeFromSettingsValue(QStringLiteral("unexpected_value")), SessionMode::Single);
}

//--------------------------------------------------------------------------

void TestSessionMode::roundTrip_conversionPreservesMode_data()
{
    QTest::addColumn<SessionMode>("mode");
    QTest::addColumn<QString>("expectedString");

    QTest::newRow("single") << SessionMode::Single << QStringLiteral("single");
    QTest::newRow("long") << SessionMode::Long << QStringLiteral("long");
    QTest::newRow("short") << SessionMode::Short << QStringLiteral("short");
}

//--------------------------------------------------------------------------

void TestSessionMode::roundTrip_conversionPreservesMode()
{
    QFETCH(SessionMode, mode);
    QFETCH(QString, expectedString);

    const QString actualString = sessionModeToSettingsValue(mode);
    QCOMPARE(actualString, expectedString);

    const SessionMode restoredMode = sessionModeFromSettingsValue(actualString);
    QCOMPARE(restoredMode, mode);
}

QTEST_APPLESS_MAIN(TestSessionMode)

#include "test_sessionmode.moc"
