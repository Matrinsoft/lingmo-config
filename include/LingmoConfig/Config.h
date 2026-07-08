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
class ConfigPrivate;

// Configuration system providing layered key-value storage.
//
// Config resolution priority (highest first):
//   1. Session overrides (set via setSessionValue)
//   2. User config (~/.config/lingmo/<name>.conf)
//   3. System config (/etc/lingmo/<name>.conf)
//
// Config files use INI format with groups:
//   [General]
//   themeName = lingmo-dark
//   [Input]
//   touchpadEnabled = true
class LINGMOCONFIG_EXPORT Config : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QStringList groups READ groups NOTIFY groupsChanged)

public:
    explicit Config(const QString &name, QObject *parent = nullptr);
    ~Config() override;

    QString name() const;

    // Load configuration from a specific file path
    bool loadFromFile(const QString &filePath);

    // Load configuration from a directory (scans for <name>.conf)
    bool loadFromDir(const QString &dirPath);

    // Load layered config (system + user + session)
    // systemDir defaults to /etc/lingmo/
    // userDir defaults to ~/.config/lingmo/
    bool load(const QString &systemDir = {},
              const QString &userDir = {});

    // Save user configuration
    bool save();
    bool saveToFile(const QString &filePath) const;

    // Key-value access
    QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    void setValue(const QString &key, const QVariant &value);
    bool contains(const QString &key) const;
    void remove(const QString &key);

    // Group-based access
    QStringList groups() const;
    QVariantMap groupValues(const QString &group) const;

    // Session overrides (highest priority, not persisted)
    QVariant sessionValue(const QString &key, const QVariant &defaultValue = {}) const;
    void setSessionValue(const QString &key, const QVariant &value);
    void clearSession();

    // File watching
    ConfigWatcher *watcher() const;

Q_SIGNALS:
    void valueChanged(const QString &key, const QVariant &value);
    void groupsChanged();
    void fileChanged();
    void loaded();
    void saved();

private:
    std::unique_ptr<ConfigPrivate> d;
};

} // namespace Lingmo
