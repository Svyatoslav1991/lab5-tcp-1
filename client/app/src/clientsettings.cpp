#include "clientsettings.h"

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
}

ClientSettingsData ClientSettings::load() const
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    ClientSettingsData data;
    data.address = settings.value(kAddressKey, kDefaultAddress).toString().trimmed();
    data.port = settings.value(kPortKey, kDefaultPort).toString().trimmed();
    data.timeout = settings.value(kTimeoutKey, kDefaultTimeout).toString().trimmed();
    data.sessionMode = sessionModeFromSettingsValue(
        settings.value(kSessionModeKey, sessionModeToSettingsValue(SessionMode::Single)).toString()
    );

    settings.endGroup();

    return data;
}

//--------------------------------------------------------------------------

void ClientSettings::save(const ClientSettingsData &data) const
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    settings.setValue(kAddressKey, data.address);
    settings.setValue(kPortKey, data.port);
    settings.setValue(kTimeoutKey, data.timeout);
    settings.setValue(kSessionModeKey, sessionModeToSettingsValue(data.sessionMode));

    settings.endGroup();
}
