#include "serversettings.h"

#include <QSettings>

namespace
{
const QString kDefaultAddress = QStringLiteral("127.0.0.1");
const QString kDefaultPort = QStringLiteral("8888");

const QString kSettingsGroup = QStringLiteral("server");
const QString kAddressKey = QStringLiteral("address");
const QString kPortKey = QStringLiteral("port");
}

ServerSettingsData ServerSettings::load() const
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    ServerSettingsData data;
    data.address = settings.value(kAddressKey, kDefaultAddress).toString().trimmed();
    data.port = settings.value(kPortKey, kDefaultPort).toString().trimmed();

    settings.endGroup();

    return data;
}

//--------------------------------------------------------------------------

void ServerSettings::save(const ServerSettingsData &data) const
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    settings.setValue(kAddressKey, data.address);
    settings.setValue(kPortKey, data.port);

    settings.endGroup();
}
