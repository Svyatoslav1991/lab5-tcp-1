#ifndef SESSIONMODE_H
#define SESSIONMODE_H

#include <QString>

/*!
 * \enum SessionMode
 * \brief Режимы работы TCP-клиента.
 */
enum class SessionMode
{
    Single, /*!< Режим 1: ручная отправка пакетов в открытом соединении. */
    Long,   /*!< Режим 2: периодическая отправка пакетов по таймеру в открытом соединении. */
    Short   /*!< Режим 3: периодическое подключение, отправка, получение ответа и отключение. */
};

/*!
 * \brief Преобразует режим клиента в строковое значение для QSettings.
 * \param mode Режим клиента.
 * \return Строковое представление режима.
 */
inline QString sessionModeToSettingsValue(SessionMode mode)
{
    switch (mode) {
    case SessionMode::Single:
        return QStringLiteral("single");
    case SessionMode::Long:
        return QStringLiteral("long");
    case SessionMode::Short:
        return QStringLiteral("short");
    }

    return QStringLiteral("single");
}

/*!
 * \brief Восстанавливает режим клиента из строкового значения QSettings.
 * \param value Строковое представление режима.
 * \return Восстановленный режим клиента.
 */
inline SessionMode sessionModeFromSettingsValue(const QString &value)
{
    if (value == QStringLiteral("long")) {
        return SessionMode::Long;
    }

    if (value == QStringLiteral("short")) {
        return SessionMode::Short;
    }

    return SessionMode::Single;
}

#endif // SESSIONMODE_H
