#include "private/Config_p.h"
#include "private/ConfigWatcher_p.h"
#include <LingmoConfig/ConfigWatcher.h>

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QFileInfo>

namespace Lingmo {

// ── ConfigPrivate ────────────────────────────────────────

ConfigPrivate::ConfigPrivate(Config *qq, const QString &configName)
    : q(qq)
    , name(configName)
    , userConfigDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                    + QStringLiteral("/lingmo"))
{
}

ConfigPrivate::~ConfigPrivate() = default;

QVariantMap ConfigPrivate::parseIniContent(const QString &content)
{
    QVariantMap result;
    QString currentGroup;

    const QStringList lines = content.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();

        // Skip comments and empty lines
        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))
            || trimmed.startsWith(QLatin1Char(';'))) {
            continue;
        }

        // Group header: [GroupName]
        if (trimmed.startsWith(QLatin1Char('[')) && trimmed.endsWith(QLatin1Char(']'))) {
            currentGroup = trimmed.mid(1, trimmed.size() - 2).trimmed();
            continue;
        }

        // Key = Value
        const int eqPos = trimmed.indexOf(QLatin1Char('='));
        if (eqPos > 0) {
            const QString key = trimmed.left(eqPos).trimmed();
            const QString value = trimmed.mid(eqPos + 1).trimmed();

            // Build qualified key: Group/key or just key
            const QString qualifiedKey = currentGroup.isEmpty()
                ? key
                : currentGroup + QLatin1Char('/') + key;

            result.insert(qualifiedKey, QVariant(value));
        }
    }

    return result;
}

bool ConfigPrivate::loadIniFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    const QString content = stream.readAll();
    file.close();

    const QVariantMap values = parseIniContent(content);
    return !values.isEmpty();
}

void ConfigPrivate::mergeMaps(QVariantMap &target, const QVariantMap &source)
{
    auto it = source.constBegin();
    while (it != source.constEnd()) {
        target.insert(it.key(), it.value());
        ++it;
    }
}

void ConfigPrivate::rebuildCache()
{
    resolvedCache.clear();

    // Merge in priority order: system < user < session
    mergeMaps(resolvedCache, systemValues);
    mergeMaps(resolvedCache, userValues);
    mergeMaps(resolvedCache, sessionValues);

    cacheDirty = false;
}

// ── Config ───────────────────────────────────────────────

Config::Config(const QString &name, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ConfigPrivate>(this, name))
{
}

Config::~Config() = default;

QString Config::name() const { return d->name; }

bool Config::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    const QString content = stream.readAll();
    file.close();

    const QVariantMap values = ConfigPrivate::parseIniContent(content);
    if (values.isEmpty())
        return false;

    d->activeFilePath = filePath;
    d->userValues = values;
    d->cacheDirty = true;

    emit loaded();
    return true;
}

bool Config::loadFromDir(const QString &dirPath)
{
    const QDir dir(dirPath);
    const QString confFile = d->name + QStringLiteral(".conf");

    if (dir.exists(confFile)) {
        return loadFromFile(dir.absoluteFilePath(confFile));
    }

    // Also try .ini extension
    const QString iniFile = d->name + QStringLiteral(".ini");
    if (dir.exists(iniFile)) {
        return loadFromFile(dir.absoluteFilePath(iniFile));
    }

    return false;
}

bool Config::load(const QString &systemDir, const QString &userDir)
{
    d->systemValues.clear();
    d->userValues.clear();

    // Determine directories
    const QString sysDir = systemDir.isEmpty()
        ? QStringLiteral("/etc/lingmo")
        : systemDir;

    const QString usrDir = userDir.isEmpty()
        ? d->userConfigDir
        : userDir;

    // Load system config (lowest priority)
    const QDir systemDirObj(sysDir);
    const QString sysConf = d->name + QStringLiteral(".conf");
    if (systemDirObj.exists(sysConf)) {
        QFile file(systemDirObj.absoluteFilePath(sysConf));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            d->systemValues = ConfigPrivate::parseIniContent(stream.readAll());
            file.close();
        }
    }

    // Load user config (higher priority)
    const QDir userDirObj(usrDir);
    const QString usrConf = d->name + QStringLiteral(".conf");
    if (userDirObj.exists(usrConf)) {
        const QString path = userDirObj.absoluteFilePath(usrConf);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            d->userValues = ConfigPrivate::parseIniContent(stream.readAll());
            d->activeFilePath = path;
            file.close();
        }
    }

    d->cacheDirty = true;

    // Start watching if watcher exists
    if (d->watcher && !d->activeFilePath.isEmpty()) {
        d->watcher->watch(d->activeFilePath);
    }

    emit loaded();
    return true;
}

bool Config::save()
{
    if (d->activeFilePath.isEmpty()) {
        // Create user config path
        QDir().mkpath(d->userConfigDir);
        d->activeFilePath = d->userConfigDir + QLatin1Char('/') + d->name
                           + QStringLiteral(".conf");
    }
    return saveToFile(d->activeFilePath);
}

bool Config::saveToFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    stream << QStringLiteral("# Lingmo Config - %1\n\n").arg(d->name);

    // Group values by prefix
    QMap<QString, QMap<QString, QVariant>> grouped;
    for (auto it = d->userValues.constBegin(); it != d->userValues.constEnd(); ++it) {
        const QString key = it.key();
        const int slashPos = key.indexOf(QLatin1Char('/'));
        if (slashPos > 0) {
            const QString group = key.left(slashPos);
            const QString subKey = key.mid(slashPos + 1);
            grouped[group].insert(subKey, it.value());
        } else {
            grouped[QString()].insert(key, it.value());
        }
    }

    // Write ungrouped keys first
    if (grouped.contains(QString())) {
        const auto &ungrouped = grouped.value(QString());
        for (auto it = ungrouped.constBegin(); it != ungrouped.constEnd(); ++it) {
            stream << it.key() << QStringLiteral(" = ") << it.value().toString() << QStringLiteral("\n");
        }
        stream << QStringLiteral("\n");
    }

    // Write grouped keys
    for (auto git = grouped.constBegin(); git != grouped.constEnd(); ++git) {
        if (git.key().isEmpty()) continue;
        stream << QStringLiteral("[") << git.key() << QStringLiteral("]\n");
        const auto &vals = git.value();
        for (auto it = vals.constBegin(); it != vals.constEnd(); ++it) {
            stream << it.key() << QStringLiteral(" = ") << it.value().toString() << QStringLiteral("\n");
        }
        stream << QStringLiteral("\n");
    }

    file.close();
    emit const_cast<Config *>(this)->saved();
    return true;
}

QVariant Config::value(const QString &key, const QVariant &defaultValue) const
{
    if (d->cacheDirty)
        const_cast<ConfigPrivate *>(d.get())->rebuildCache();

    return d->resolvedCache.value(key, defaultValue);
}

void Config::setValue(const QString &key, const QVariant &value)
{
    if (d->userValues.value(key) == value)
        return;

    d->userValues.insert(key, value);
    d->cacheDirty = true;

    emit valueChanged(key, value);
}

bool Config::contains(const QString &key) const
{
    if (d->cacheDirty)
        const_cast<ConfigPrivate *>(d.get())->rebuildCache();

    return d->resolvedCache.contains(key);
}

void Config::remove(const QString &key)
{
    if (!d->userValues.contains(key))
        return;

    d->userValues.remove(key);
    d->cacheDirty = true;

    emit valueChanged(key, QVariant());
}

QStringList Config::groups() const
{
    QSet<QString> groups;
    const QVariantMap &map = d->userValues.isEmpty() ? d->systemValues : d->userValues;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const int slashPos = it.key().indexOf(QLatin1Char('/'));
        if (slashPos > 0) {
            groups.insert(it.key().left(slashPos));
        }
    }
    return QStringList(groups.begin(), groups.end());
}

QVariantMap Config::groupValues(const QString &group) const
{
    QVariantMap result;
    const QString prefix = group + QLatin1Char('/');
    const QVariantMap &map = d->userValues.isEmpty() ? d->systemValues : d->userValues;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (it.key().startsWith(prefix)) {
            result.insert(it.key().mid(prefix.size()), it.value());
        }
    }
    return result;
}

QVariant Config::sessionValue(const QString &key, const QVariant &defaultValue) const
{
    return d->sessionValues.value(key, defaultValue);
}

void Config::setSessionValue(const QString &key, const QVariant &value)
{
    d->sessionValues.insert(key, value);
    d->cacheDirty = true;

    emit valueChanged(key, value);
}

void Config::clearSession()
{
    d->sessionValues.clear();
    d->cacheDirty = true;
}

ConfigWatcher *Config::watcher() const
{
    if (!d->watcher) {
        const_cast<Config *>(this)->d->watcher = new ConfigWatcher(
            const_cast<Config *>(this), const_cast<Config *>(this));
    }
    return d->watcher;
}

} // namespace Lingmo
