#pragma once

#include <LingmoConfig/LingmoConfigExport.h>

#include <QObject>
#include <memory>

namespace Lingmo {

class Config;
class ConfigWatcherPrivate;

// Watches a configuration file for changes and emits notifications.
// Uses inotify on Linux for efficient kernel-level file monitoring.
class LINGMOCONFIG_EXPORT ConfigWatcher : public QObject
{
    Q_OBJECT

public:
    explicit ConfigWatcher(Config *config, QObject *parent = nullptr);
    ~ConfigWatcher() override;

    // Start watching the file at the given path
    void watch(const QString &filePath);

    // Stop watching
    void stop();

    bool isWatching() const;

Q_SIGNALS:
    void fileChanged(const QString &filePath);
    void fileRemoved(const QString &filePath);

private:
    std::unique_ptr<ConfigWatcherPrivate> d;
};

} // namespace Lingmo
