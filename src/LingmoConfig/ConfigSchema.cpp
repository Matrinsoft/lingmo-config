#include "private/ConfigSchema_p.h"

namespace Lingmo {

ConfigSchema::ConfigSchema(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ConfigSchemaPrivate>())
{
}

ConfigSchema::~ConfigSchema() = default;

void ConfigSchema::addKey(const QString &key, QMetaType::Type type,
                           const QVariant &defaultValue)
{
    SchemaKey sk;
    sk.type = type;
    sk.defaultValue = defaultValue;
    d->keys.insert(key, sk);
    emit schemaChanged();
}

void ConfigSchema::addKey(const QString &key, QMetaType::Type type,
                           const QVariant &defaultValue,
                           const QVariant &minValue,
                           const QVariant &maxValue)
{
    SchemaKey sk;
    sk.type = type;
    sk.defaultValue = defaultValue;
    sk.minValue = minValue;
    sk.maxValue = maxValue;
    sk.hasRange = true;
    d->keys.insert(key, sk);
    emit schemaChanged();
}

void ConfigSchema::addKey(const QString &key, QMetaType::Type type,
                           const QVariant &defaultValue,
                           const QStringList &allowedValues)
{
    SchemaKey sk;
    sk.type = type;
    sk.defaultValue = defaultValue;
    sk.allowedValues = allowedValues;
    sk.hasAllowedValues = true;
    d->keys.insert(key, sk);
    emit schemaChanged();
}

void ConfigSchema::removeKey(const QString &key)
{
    d->keys.remove(key);
    emit schemaChanged();
}

bool ConfigSchema::hasKey(const QString &key) const
{
    return d->keys.contains(key);
}

QVariant ConfigSchema::defaultValue(const QString &key) const
{
    return d->keys.value(key).defaultValue;
}

QMetaType::Type ConfigSchema::keyType(const QString &key) const
{
    return d->keys.value(key).type;
}

QString ConfigSchema::validate(const QString &key, const QVariant &value) const
{
    const auto it = d->keys.constFind(key);
    if (it == d->keys.constEnd())
        return QString(); // Unknown keys pass validation

    const SchemaKey &sk = it.value();

    // Check type
    if (value.typeId() != sk.type && !value.isNull()) {
        return QStringLiteral("Type mismatch for '%1': expected %2, got %3")
            .arg(key)
            .arg(QMetaType(sk.type).name())
            .arg(value.typeName());
    }

    // Check range (for numeric types)
    if (sk.hasRange && !value.isNull()) {
        const double numVal = value.toDouble();
        const bool less = (numVal < sk.minValue.toDouble());
        const bool greater = (numVal > sk.maxValue.toDouble());
        if (less || greater) {
            return QStringLiteral("Value for '%1' out of range [%2, %3]: %4")
                .arg(key)
                .arg(sk.minValue.toString())
                .arg(sk.maxValue.toString())
                .arg(value.toString());
        }
    }

    // Check allowed values
    if (sk.hasAllowedValues && !value.isNull()) {
        if (!sk.allowedValues.contains(value.toString())) {
            return QStringLiteral("Value for '%1' not in allowed list: %2")
                .arg(key, value.toString());
        }
    }

    return QString(); // Valid
}

QMap<QString, QString> ConfigSchema::validateAll(const QVariantMap &values) const
{
    QMap<QString, QString> errors;

    for (auto it = d->keys.constBegin(); it != d->keys.constEnd(); ++it) {
        const QString key = it.key();
        const QVariant value = values.value(key, it.value().defaultValue);

        const QString error = validate(key, value);
        if (!error.isEmpty()) {
            errors.insert(key, error);
        }
    }

    return errors;
}

void ConfigSchema::applyDefaults(QVariantMap &values) const
{
    for (auto it = d->keys.constBegin(); it != d->keys.constEnd(); ++it) {
        if (!values.contains(it.key()) && !it.value().defaultValue.isNull()) {
            values.insert(it.key(), it.value().defaultValue);
        }
    }
}

QStringList ConfigSchema::keys() const
{
    return d->keys.keys();
}

} // namespace Lingmo
