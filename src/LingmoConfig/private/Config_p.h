#pragma once

#include <LingmoConfig/Config.h>

#include <QSettings>
#include <QVariantMap>
#include <QString>
#include <QDir>

namespace Lingmo {

class ConfigPrivate
{
public:
    explicit ConfigPrivate(Config *qq, const QString &configName);
    ~ConfigPrivate();

    // Parse INI content from a file
    bool loadIniFile(const QString &filePath);

    // Merge values from source into target (source wins on conflict)
    static void mergeMaps(QVariantMap &target, const QVariantMap &source);

    Config *q;
    QString name;

    // Layered storage: system < user < session
    QVariantMap systemValues;
    QVariantMap userValues;
    QVariantMap sessionValues;

    // Resolved cache (merged from all layers)
    QVariantMap resolvedCache;
    bool cacheDirty = true;

    // File paths
    QString systemConfigPath;
    QString userConfigPath;
    QString userConfigDir;
    QString activeFilePath;  // file currently loaded from

    ConfigWatcher *watcher = nullptr;

    void rebuildCache();
    static QVariantMap parseIniContent(const QString &content);
};

} // namespace Lingmo
