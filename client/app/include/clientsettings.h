#ifndef CLIENTSETTINGS_H
#define CLIENTSETTINGS_H

#include <QString>

#include "sessionmode.h"

/*!
 * \struct ClientSettingsData
 * \brief DTO со всеми пользовательскими настройками клиента.
 */
struct ClientSettingsData
{
    QString address;                    /*!< Адрес сервера. */
    QString port;                       /*!< Порт сервера. */
    QString timeout;                    /*!< Таймаут в миллисекундах. */
    SessionMode sessionMode = SessionMode::Single; /*!< Текущий режим клиента. */
};

/*!
 * \class ClientSettings
 * \brief Сервис загрузки и сохранения настроек клиента через QSettings.
 *
 * \details
 * Класс инкапсулирует:
 * - ключи и группу настроек;
 * - значения по умолчанию;
 * - загрузку настроек в DTO;
 * - сохранение DTO в QSettings.
 *
 * Сам класс ничего не знает про UI и валидацию полей интерфейса.
 */
class ClientSettings
{
public:
    /*!
     * \brief Загружает настройки клиента.
     * \return Загруженные настройки с подставленными значениями по умолчанию.
     */
    ClientSettingsData load() const;

    /*!
     * \brief Сохраняет настройки клиента.
     * \param data DTO с данными для сохранения.
     */
    void save(const ClientSettingsData &data) const;
};

#endif // CLIENTSETTINGS_H
