#include "private/Config_p.h"
#include "private/ConfigWatcher_p.h"
#include <LingmoConfig/ConfigWatcher.h>
#include <LingmoConfig/ConfigSchema.h>

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

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

ConfigPrivate::Format ConfigPrivate::detectFormat(const QString &filePath)
{
    if (filePath.endsWith(QLatin1String(".json"), Qt::CaseInsensitive))
        return Format::JSON;
    if (filePath.endsWith(QLatin1String(".conf"), Qt::CaseInsensitive)
        || filePath.endsWith(QLatin1String(".ini"), Qt::CaseInsensitive))
        return Format::INI;
    return Format::Unknown;
}

QVariantMap ConfigPrivate::parseIniContent(const QString &content)
{
    QVariantMap result;
    QString currentGroup;

    const QStringList lines = content.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();

        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))
            || trimmed.startsWith(QLatin1Char(';'))) {
            continue;
        }

        if (trimmed.startsWith(QLatin1Char('[')) && trimmed.endsWith(QLatin1Char(']'))) {
            currentGroup = trimmed.mid(1, trimmed.size() - 2).trimmed();
            continue;
        }

        const int eqPos = trimmed.indexOf(QLatin1Char('='));
        if (eqPos > 0) {
            const QString key = trimmed.left(eqPos).trimmed();
            const QString value = trimmed.mid(eqPos + 1).trimmed();
            const QString qualifiedKey = currentGroup.isEmpty()
                ? key
                : currentGroup + QLatin1Char('/') + key;
            result.insert(qualifiedKey, QVariant(value));
        }
    }

    return result;
}

QVariantMap ConfigPrivate::parseJsonContent(const QString &content)
{
    const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (doc.isNull() || !doc.isObject())
        return {};

    QVariantMap result;
    const QJsonObject obj = doc.object();

    // Flatten nested JSON to Group/key format
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        if (it.value().isObject()) {
            const QJsonObject group = it.value().toObject();
            for (auto git = group.constBegin(); git != group.constEnd(); ++git) {
                const QString qualifiedKey = it.key() + QLatin1Char('/') + git.key();
                result.insert(qualifiedKey, git.value().toVariant());
            }
        } else {
            result.insert(it.key(), it.value().toVariant());
        }
    }

    return result;
}

QString ConfigPrivate::serializeToJson(const QVariantMap &values)
{
    QJsonObject root;

    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        const QString key = it.key();
        const int slashPos = key.indexOf(QLatin1Char('/'));

        if (slashPos > 0) {
            const QString group = key.left(slashPos);
            const QString subKey = key.mid(slashPos + 1);
            if (!root.contains(group)) {
                root.insert(group, QJsonObject());
            }
            QJsonObject groupObj = root.value(group).toObject();
            groupObj.insert(subKey, QJsonValue::fromVariant(it.value()));
            root.insert(group, groupObj);
        } else {
            root.insert(key, QJsonValue::fromVariant(it.value()));
        }
    }

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void ConfigPrivate::mergeMaps(QVariantMap &target, const QVariantMap &source)
{
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) {
        target.insert(it.key(), it.value());
    }
}

void ConfigPrivate::rebuildCache()
{
    resolvedCache.clear();
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

    const auto format = ConfigPrivate::detectFormat(filePath);
    QVariantMap values;

    switch (format) {
    case ConfigPrivate::Format::JSON:
        values = ConfigPrivate::parseJsonContent(content);
        break;
    case ConfigPrivate::Format::INI:
    case ConfigPrivate::Format::Unknown:
        values = ConfigPrivate::parseIniContent(content);
        break;
    }

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

    // Try JSON first, then INI
    const QString jsonFile = d->name + QStringLiteral(".json");
    if (dir.exists(jsonFile))
        return loadFromFile(dir.absoluteFilePath(jsonFile));

    const QString confFile = d->name + QStringLiteral(".conf");
    if (dir.exists(confFile))
        return loadFromFile(dir.absoluteFilePath(confFile));

    const QString iniFile = d->name + QStringLiteral(".ini");
    if (dir.exists(iniFile))
        return loadFromFile(dir.absoluteFilePath(iniFile));

    return false;
}

bool Config::load(const QString &systemDir, const QString &userDir)
{
    d->systemValues.clear();
    d->userValues.clear();

    const QString sysDir = systemDir.isEmpty()
        ? QStringLiteral("/etc/lingmo") : systemDir;
    const QString usrDir = userDir.isEmpty()
        ? d->userConfigDir : userDir;

    // Load system config (lowest priority)
    loadFromDir(sysDir);
    d->systemValues = d->userValues;
    d->userValues.clear();
    d->activeFilePath.clear();

    // Load user config (higher priority)
    loadFromDir(usrDir);

    d->cacheDirty = true;

    if (d->watcher && !d->activeFilePath.isEmpty())
        d->watcher->watch(d->activeFilePath);

    emit loaded();
    return true;
}

bool Config::save()
{
    if (d->activeFilePath.isEmpty()) {
        QDir().mkpath(d->userConfigDir);
        d->activeFilePath = d->userConfigDir + QLatin1Char('/') + d->name
                           + QStringLiteral(".conf");
    }
    return saveToFile(d->activeFilePath);
}

bool Config::saveToFile(const QString &filePath) const
{
    const auto format = ConfigPrivate::detectFormat(filePath);
    const QVariantMap &data = d->userValues;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);

    if (format == ConfigPrivate::Format::JSON) {
        stream << ConfigPrivate::serializeToJson(data);
    } else {
        // INI format
        stream << QStringLiteral("# Lingmo Config - %1\n\n").arg(d->name);

        QMap<QString, QMap<QString, QVariant>> grouped;
        for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
            const int slashPos = it.key().indexOf(QLatin1Char('/'));
            if (slashPos > 0) {
                grouped[it.key().left(slashPos)].insert(
                    it.key().mid(slashPos + 1), it.value());
            } else {
                grouped[QString()].insert(it.key(), it.value());
            }
        }

        if (grouped.contains(QString())) {
            for (auto it = grouped.value(QString()).constBegin();
                 it != grouped.value(QString()).constEnd(); ++it) {
                stream << it.key() << " = " << it.value().toString() << "\n";
            }
            stream << "\n";
        }

        for (auto git = grouped.constBegin(); git != grouped.constEnd(); ++git) {
            if (git.key().isEmpty()) continue;
            stream << "[" << git.key() << "]\n";
            for (auto it = git.value().constBegin(); it != git.value().constEnd(); ++it) {
                stream << it.key() << " = " << it.value().toString() << "\n";
            }
            stream << "\n";
        }
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
    // Validate against schema if set
    if (d->schema) {
        const QString error = d->schema->validate(key, value);
        if (!error.isEmpty()) {
            emit validationFailed(key, error);
            return;
        }
    }

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
    QSet<QString> grp;
    const QVariantMap &map = d->userValues.isEmpty() ? d->systemValues : d->userValues;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const int pos = it.key().indexOf(QLatin1Char('/'));
        if (pos > 0) grp.insert(it.key().left(pos));
    }
    return QStringList(grp.begin(), grp.end());
}

QVariantMap Config::groupValues(const QString &group) const
{
    QVariantMap result;
    const QString prefix = group + QLatin1Char('/');
    const QVariantMap &map = d->userValues.isEmpty() ? d->systemValues : d->userValues;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (it.key().startsWith(prefix))
            result.insert(it.key().mid(prefix.size()), it.value());
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

void Config::setSchema(ConfigSchema *schema)
{
    d->schema = schema;
}

ConfigSchema *Config::schema() const
{
    return d->schema;
}

bool Config::validate() const
{
    if (!d->schema) return true;

    if (d->cacheDirty)
        const_cast<ConfigPrivate *>(d.get())->rebuildCache();

    const auto errors = d->schema->validateAll(d->resolvedCache);
    for (auto it = errors.constBegin(); it != errors.constEnd(); ++it) {
        emit const_cast<Config *>(this)->validationFailed(it.key(), it.value());
    }
    return errors.isEmpty();
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
