#pragma once

#include <LingmoConfig/Config.h>

#include <QVariantMap>
#include <QString>
#include <QDir>

namespace Lingmo {

class ConfigSchema;

class ConfigPrivate
{
public:
    explicit ConfigPrivate(Config *qq, const QString &configName);
    ~ConfigPrivate();

    // Merge values from source into target (source wins on conflict)
    static void mergeMaps(QVariantMap &target, const QVariantMap &source);

    // Detect format from file extension
    enum class Format { INI, JSON, Unknown };
    static Format detectFormat(const QString &filePath);

    // Parse content by format
    static QVariantMap parseIniContent(const QString &content);
    static QVariantMap parseJsonContent(const QString &content);
    static QString serializeToJson(const QVariantMap &values);

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
    QString userConfigDir;
    QString activeFilePath;

    ConfigWatcher *watcher = nullptr;
    ConfigSchema *schema = nullptr;

    void rebuildCache();
};

} // namespace Lingmo
