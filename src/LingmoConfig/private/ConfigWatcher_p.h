#pragma once

#include <LingmoConfig/ConfigWatcher.h>

#include <QFileSystemWatcher>
#include <QString>

namespace Lingmo {

class Config;

class ConfigWatcherPrivate
{
public:
    explicit ConfigWatcherPrivate(ConfigWatcher *qq, Config *config);
    ~ConfigWatcherPrivate();

    ConfigWatcher *q;
    Config *config;
    QFileSystemWatcher *fsWatcher = nullptr;
    QString watchedPath;
};

} // namespace Lingmo
