#include "private/ConfigWatcher_p.h"
#include <LingmoConfig/Config.h>

#include <QFileInfo>

namespace Lingmo {

ConfigWatcherPrivate::ConfigWatcherPrivate(ConfigWatcher *qq, Config *config)
    : q(qq)
    , config(config)
    , fsWatcher(new QFileSystemWatcher(qq))
{
}

ConfigWatcherPrivate::~ConfigWatcherPrivate() = default;

// ── ConfigWatcher ────────────────────────────────────────

ConfigWatcher::ConfigWatcher(Config *config, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ConfigWatcherPrivate>(this, config))
{
    connect(d->fsWatcher, &QFileSystemWatcher::fileChanged,
            this, [this](const QString &path) {
                QFileInfo info(path);
                if (!info.exists()) {
                    emit fileRemoved(path);
                } else {
                    emit fileChanged(path);
                }
            });
}

ConfigWatcher::~ConfigWatcher() = default;

void ConfigWatcher::watch(const QString &filePath)
{
    // Stop watching previous file
    if (!d->watchedPath.isEmpty()) {
        d->fsWatcher->removePath(d->watchedPath);
        d->watchedPath.clear();
    }

    if (filePath.isEmpty())
        return;

    d->watchedPath = filePath;
    d->fsWatcher->addPath(filePath);
}

void ConfigWatcher::stop()
{
    if (!d->watchedPath.isEmpty()) {
        d->fsWatcher->removePath(d->watchedPath);
        d->watchedPath.clear();
    }
}

bool ConfigWatcher::isWatching() const
{
    return !d->watchedPath.isEmpty();
}

} // namespace Lingmo
