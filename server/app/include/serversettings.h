#ifndef SERVERSETTINGS_H
#define SERVERSETTINGS_H

#include <QString>

/*!
 * \struct ServerSettingsData
 * \brief DTO с пользовательскими настройками TCP-сервера.
 */
struct ServerSettingsData
{
    QString address; /*!< Адрес сервера. */
    QString port;    /*!< Порт сервера. */
};

/*!
 * \class ServerSettings
 * \brief Сервис загрузки и сохранения настроек сервера через QSettings.
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
class ServerSettings
{
public:
    /*!
     * \brief Загружает настройки сервера.
     * \return Загруженные настройки с подставленными значениями по умолчанию.
     */
    ServerSettingsData load() const;

    /*!
     * \brief Сохраняет настройки сервера.
     * \param data DTO с данными для сохранения.
     */
    void save(const ServerSettingsData &data) const;
};

#endif // SERVERSETTINGS_H
