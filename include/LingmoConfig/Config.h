#pragma once

#include <LingmoConfig/LingmoConfigExport.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <memory>

namespace Lingmo {

class ConfigWatcher;
class ConfigSchema;
class ConfigPrivate;

// Configuration system providing layered key-value storage.
//
// Config resolution priority (highest first):
//   1. Session overrides (set via setSessionValue)
//   2. User config (~/.config/lingmo/<name>.conf or .json)
//   3. System config (/etc/lingmo/<name>.conf or .json)
//
// Supported formats: INI and JSON.
class LINGMOCONFIG_EXPORT Config : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QStringList groups READ groups NOTIFY groupsChanged)

public:
    explicit Config(const QString &name, QObject *parent = nullptr);
    ~Config() override;

    QString name() const;

    // ── Loading ───────────────────────────────────────────

    // Load from a specific file (auto-detects INI vs JSON by extension)
    bool loadFromFile(const QString &filePath);

    // Load from a directory (scans for <name>.conf, <name>.json)
    bool loadFromDir(const QString &dirPath);

    // Load layered config (system + user + session)
    bool load(const QString &systemDir = {},
              const QString &userDir = {});

    // ── Saving ────────────────────────────────────────────

    bool save();
    bool saveToFile(const QString &filePath) const;

    // ── Key-value access ──────────────────────────────────

    QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    void setValue(const QString &key, const QVariant &value);
    bool contains(const QString &key) const;
    void remove(const QString &key);

    // ── Group access ──────────────────────────────────────

    QStringList groups() const;
    QVariantMap groupValues(const QString &group) const;

    // ── Session overrides ─────────────────────────────────

    QVariant sessionValue(const QString &key, const QVariant &defaultValue = {}) const;
    void setSessionValue(const QString &key, const QVariant &value);
    void clearSession();

    // ── Schema validation ─────────────────────────────────

    void setSchema(ConfigSchema *schema);
    ConfigSchema *schema() const;

    // Validate all current values against the schema.
    // Returns true if all values are valid.
    bool validate() const;

    // ── File watching ─────────────────────────────────────

    ConfigWatcher *watcher() const;

Q_SIGNALS:
    void valueChanged(const QString &key, const QVariant &value);
    void groupsChanged();
    void fileChanged();
    void loaded();
    void saved();
    void validationFailed(const QString &key, const QString &error);

private:
    std::unique_ptr<ConfigPrivate> d;
};

} // namespace Lingmo
