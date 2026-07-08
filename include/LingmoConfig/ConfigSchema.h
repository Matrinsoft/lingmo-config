#pragma once

#include <LingmoConfig/LingmoConfigExport.h>

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <memory>

namespace Lingmo {

class ConfigSchemaPrivate;

// Schema definition for configuration validation.
//
// Defines allowed keys with types, default values, and constraints.
// Example:
//   auto *schema = new ConfigSchema;
//   schema->addKey("General/themeName", QMetaType::QString, "lingmo-light");
//   schema->addKey("General/fontSize", QMetaType::Int, 12, 8, 72);
//   config->setSchema(schema);
class LINGMOCONFIG_EXPORT ConfigSchema : public QObject
{
    Q_OBJECT

public:
    explicit ConfigSchema(QObject *parent = nullptr);
    ~ConfigSchema() override;

    // Add a key with type and default value
    void addKey(const QString &key, QMetaType::Type type,
                const QVariant &defaultValue = {});

    // Add a key with numeric range constraints
    void addKey(const QString &key, QMetaType::Type type,
                const QVariant &defaultValue,
                const QVariant &minValue,
                const QVariant &maxValue);

    // Add a key with enumerated allowed values
    void addKey(const QString &key, QMetaType::Type type,
                const QVariant &defaultValue,
                const QStringList &allowedValues);

    // Remove a key definition
    void removeKey(const QString &key);

    // Check if a key is defined in the schema
    bool hasKey(const QString &key) const;

    // Get the default value for a key
    QVariant defaultValue(const QString &key) const;

    // Get the type for a key
    QMetaType::Type keyType(const QString &key) const;

    // Validate a single value against its schema definition.
    // Returns an empty string on success, or an error message.
    QString validate(const QString &key, const QVariant &value) const;

    // Validate all provided values. Returns map of key -> error.
    // Empty map means all values are valid.
    QMap<QString, QString> validateAll(const QVariantMap &values) const;

    // Apply default values to a value map (fills missing keys)
    void applyDefaults(QVariantMap &values) const;

    // All defined keys
    QStringList keys() const;

Q_SIGNALS:
    void schemaChanged();

private:
    std::unique_ptr<ConfigSchemaPrivate> d;
};

} // namespace Lingmo
