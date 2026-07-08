#pragma once

#include <LingmoConfig/ConfigSchema.h>

#include <QMap>
#include <QString>
#include <QVariant>

namespace Lingmo {

struct SchemaKey {
    QMetaType::Type type = QMetaType::QString;
    QVariant defaultValue;
    QVariant minValue;
    QVariant maxValue;
    QStringList allowedValues;
    bool hasRange = false;
    bool hasAllowedValues = false;
};

class ConfigSchemaPrivate
{
public:
    QMap<QString, SchemaKey> keys;
};

} // namespace Lingmo
