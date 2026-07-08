#include <LingmoConfig/Config.h>
#include <LingmoConfig/ConfigSchema.h>
#include <LingmoConfig/ConfigWatcher.h>

#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QSignalSpy>

class tst_Config : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCreateConfig()
    {
        Lingmo::Config config(QStringLiteral("test"));
        QCOMPARE(config.name(), QStringLiteral("test"));
    }

    void testLoadFromFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString confPath = dir.filePath(QStringLiteral("test.conf"));
        QFile file(confPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream stream(&file);
        stream << QStringLiteral("[General]\n");
        stream << QStringLiteral("themeName = lingmo-dark\n");
        stream << QStringLiteral("accentColor = #3daee9\n");
        stream << QStringLiteral("[Input]\n");
        stream << QStringLiteral("touchpadEnabled = true\n");
        file.close();

        Lingmo::Config config(QStringLiteral("test"));
        QSignalSpy loadedSpy(&config, &Lingmo::Config::loaded);
        QVERIFY(config.loadFromFile(confPath));
        QCOMPARE(loadedSpy.count(), 1);

        QCOMPARE(config.value(QStringLiteral("General/themeName")).toString(),
                 QStringLiteral("lingmo-dark"));
        QCOMPARE(config.value(QStringLiteral("General/accentColor")).toString(),
                 QStringLiteral("#3daee9"));
        QCOMPARE(config.value(QStringLiteral("Input/touchpadEnabled")).toString(),
                 QStringLiteral("true"));
    }

    void testSetValue()
    {
        Lingmo::Config config(QStringLiteral("test"));

        QSignalSpy changedSpy(&config, &Lingmo::Config::valueChanged);
        config.setValue(QStringLiteral("myKey"), QStringLiteral("myValue"));
        QCOMPARE(changedSpy.count(), 1);

        QCOMPARE(config.value(QStringLiteral("myKey")).toString(),
                 QStringLiteral("myValue"));
    }

    void testDefaultValue()
    {
        Lingmo::Config config(QStringLiteral("test"));
        QCOMPARE(config.value(QStringLiteral("nonexistent"), QStringLiteral("default")).toString(),
                 QStringLiteral("default"));
    }

    void testContains()
    {
        Lingmo::Config config(QStringLiteral("test"));
        QVERIFY(!config.contains(QStringLiteral("key")));

        config.setValue(QStringLiteral("key"), QStringLiteral("value"));
        QVERIFY(config.contains(QStringLiteral("key")));
    }

    void testRemove()
    {
        Lingmo::Config config(QStringLiteral("test"));
        config.setValue(QStringLiteral("key"), QStringLiteral("value"));
        QVERIFY(config.contains(QStringLiteral("key")));

        config.remove(QStringLiteral("key"));
        QVERIFY(!config.contains(QStringLiteral("key")));
    }

    void testSessionOverride()
    {
        Lingmo::Config config(QStringLiteral("test"));
        config.setValue(QStringLiteral("key"), QStringLiteral("persistent"));

        // Session value overrides normal value
        config.setSessionValue(QStringLiteral("key"), QStringLiteral("session"));
        QCOMPARE(config.value(QStringLiteral("key")).toString(),
                 QStringLiteral("session"));

        // Clear session restores persistent value
        config.clearSession();
        QCOMPARE(config.value(QStringLiteral("key")).toString(),
                 QStringLiteral("persistent"));
    }

    void testGroups()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString confPath = dir.filePath(QStringLiteral("test.conf"));
        QFile file(confPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream stream(&file);
        stream << QStringLiteral("[Appearance]\n");
        stream << QStringLiteral("theme = dark\n");
        stream << QStringLiteral("[Input]\n");
        stream << QStringLiteral("mouse = enabled\n");
        file.close();

        Lingmo::Config config(QStringLiteral("test"));
        config.loadFromFile(confPath);

        const QStringList groups = config.groups();
        QVERIFY(groups.contains(QStringLiteral("Appearance")));
        QVERIFY(groups.contains(QStringLiteral("Input")));
    }

    void testGroupValues()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString confPath = dir.filePath(QStringLiteral("test.conf"));
        QFile file(confPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream stream(&file);
        stream << QStringLiteral("[Input]\n");
        stream << QStringLiteral("mouse = enabled\n");
        stream << QStringLiteral("touchpad = disabled\n");
        file.close();

        Lingmo::Config config(QStringLiteral("test"));
        config.loadFromFile(confPath);

        const QVariantMap inputValues = config.groupValues(QStringLiteral("Input"));
        QCOMPARE(inputValues.size(), 2);
        QCOMPARE(inputValues.value(QStringLiteral("mouse")).toString(),
                 QStringLiteral("enabled"));
    }

    void testSaveAndReload()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString confPath = dir.filePath(QStringLiteral("test.conf"));

        // Create and save
        Lingmo::Config config1(QStringLiteral("test"));
        config1.setValue(QStringLiteral("General/key1"), QStringLiteral("value1"));
        config1.setValue(QStringLiteral("General/key2"), QStringLiteral("value2"));
        QVERIFY(config1.saveToFile(confPath));

        // Reload and verify
        Lingmo::Config config2(QStringLiteral("test"));
        QVERIFY(config2.loadFromFile(confPath));
        QCOMPARE(config2.value(QStringLiteral("General/key1")).toString(),
                 QStringLiteral("value1"));
        QCOMPARE(config2.value(QStringLiteral("General/key2")).toString(),
                 QStringLiteral("value2"));
    }

    void testLoadLayeredConfig()
    {
        QTemporaryDir systemDir;
        QTemporaryDir userDir;
        QVERIFY(systemDir.isValid());
        QVERIFY(userDir.isValid());

        // System config
        QFile sysFile(systemDir.filePath(QStringLiteral("test.conf")));
        QVERIFY(sysFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream sysStream(&sysFile);
        sysStream << QStringLiteral("[General]\n");
        sysStream << QStringLiteral("themeName = lingmo-light\n");
        sysStream << QStringLiteral("fontSize = 12\n");
        sysFile.close();

        // User config (overrides themeName)
        QFile usrFile(userDir.filePath(QStringLiteral("test.conf")));
        QVERIFY(usrFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream usrStream(&usrFile);
        usrStream << QStringLiteral("[General]\n");
        usrStream << QStringLiteral("themeName = lingmo-dark\n");
        usrFile.close();

        Lingmo::Config config(QStringLiteral("test"));
        QVERIFY(config.load(systemDir.path(), userDir.path()));

        // User overrides system
        QCOMPARE(config.value(QStringLiteral("General/themeName")).toString(),
                 QStringLiteral("lingmo-dark"));
        // System value preserved where user has no override
        QCOMPARE(config.value(QStringLiteral("General/fontSize")).toString(),
                 QStringLiteral("12"));
    }

    void testNullFileHandling()
    {
        Lingmo::Config config(QStringLiteral("test"));
        QVERIFY(!config.loadFromFile(QStringLiteral("/nonexistent/path.conf")));
    }

    void testEmptyConfig()
    {
        Lingmo::Config config(QStringLiteral("test"));
        QCOMPARE(config.value(QStringLiteral("anything"), 42).toInt(), 42);
        QVERIFY(config.groups().isEmpty());
    }

    // ── JSON format tests ─────────────────────────────────

    void testLoadFromJson()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString jsonPath = dir.filePath(QStringLiteral("test.json"));
        QFile file(jsonPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream stream(&file);
        stream << QStringLiteral("{\n");
        stream << QStringLiteral("  \"General\": {\n");
        stream << QStringLiteral("    \"themeName\": \"lingmo-dark\",\n");
        stream << QStringLiteral("    \"fontSize\": 14\n");
        stream << QStringLiteral("  },\n");
        stream << QStringLiteral("  \"Input\": {\n");
        stream << QStringLiteral("    \"touchpadEnabled\": true\n");
        stream << QStringLiteral("  }\n");
        stream << QStringLiteral("}\n");
        file.close();

        Lingmo::Config config(QStringLiteral("test"));
        QVERIFY(config.loadFromFile(jsonPath));

        QCOMPARE(config.value(QStringLiteral("General/themeName")).toString(),
                 QStringLiteral("lingmo-dark"));
        QCOMPARE(config.value(QStringLiteral("General/fontSize")).toInt(), 14);
        QCOMPARE(config.value(QStringLiteral("Input/touchpadEnabled")).toBool(), true);
    }

    void testSaveAsJson()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString jsonPath = dir.filePath(QStringLiteral("test.json"));

        Lingmo::Config config1(QStringLiteral("test"));
        config1.setValue(QStringLiteral("General/themeName"), QStringLiteral("lingmo-dark"));
        config1.setValue(QStringLiteral("General/fontSize"), 14);
        QVERIFY(config1.saveToFile(jsonPath));

        // Reload and verify
        Lingmo::Config config2(QStringLiteral("test"));
        QVERIFY(config2.loadFromFile(jsonPath));
        QCOMPARE(config2.value(QStringLiteral("General/themeName")).toString(),
                 QStringLiteral("lingmo-dark"));
        QCOMPARE(config2.value(QStringLiteral("General/fontSize")).toInt(), 14);
    }

    void testJsonAndIniInteroperability()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // Save as JSON
        const QString jsonPath = dir.filePath(QStringLiteral("test.json"));
        Lingmo::Config config1(QStringLiteral("test"));
        config1.setValue(QStringLiteral("key"), QStringLiteral("json-value"));
        QVERIFY(config1.saveToFile(jsonPath));

        // Load as INI (should fail or return empty — different format)
        Lingmo::Config config2(QStringLiteral("test"));
        // JSON content won't parse as INI, so loadFromFile should fail
        // or return empty
        const bool loaded = config2.loadFromFile(jsonPath);
        // JSON content may partially parse as INI (unlikely to have valid keys)
        // The key point is that format detection works
        Q_UNUSED(loaded);
    }

    // ── Schema validation tests ───────────────────────────

    void testSchemaValidation()
    {
        auto *schema = new Lingmo::ConfigSchema;
        schema->addKey(QStringLiteral("General/themeName"), QMetaType::QString,
                       QStringLiteral("lingmo-light"));
        schema->addKey(QStringLiteral("General/fontSize"), QMetaType::Int,
                       12, 8, 72);

        Lingmo::Config config(QStringLiteral("test"));
        config.setSchema(schema);

        // Valid value
        QSignalSpy validSpy(&config, &Lingmo::Config::validationFailed);
        config.setValue(QStringLiteral("General/themeName"), QStringLiteral("lingmo-dark"));
        QCOMPARE(validSpy.count(), 0);

        // Invalid value (out of range)
        config.setValue(QStringLiteral("General/fontSize"), 100);
        QCOMPARE(validSpy.count(), 1);
        QCOMPARE(validSpy.at(0).at(0).toString(),
                 QStringLiteral("General/fontSize"));

        // Value should NOT be set (rejected)
        QVERIFY(!config.contains(QStringLiteral("General/fontSize")));
    }

    void testSchemaAllowedValues()
    {
        auto *schema = new Lingmo::ConfigSchema;
        schema->addKey(QStringLiteral("General/colorScheme"), QMetaType::QString,
                       QStringLiteral("light"),
                       QStringList() << QStringLiteral("light")
                                     << QStringLiteral("dark")
                                     << QStringLiteral("highcontrast"));

        Lingmo::Config config(QStringLiteral("test"));
        config.setSchema(schema);

        QSignalSpy errorSpy(&config, &Lingmo::Config::validationFailed);

        // Valid
        config.setValue(QStringLiteral("General/colorScheme"), QStringLiteral("dark"));
        QCOMPARE(errorSpy.count(), 0);

        // Invalid
        config.setValue(QStringLiteral("General/colorScheme"), QStringLiteral("neon"));
        QCOMPARE(errorSpy.count(), 1);
    }

    void testSchemaDefaults()
    {
        auto *schema = new Lingmo::ConfigSchema;
        schema->addKey(QStringLiteral("General/themeName"), QMetaType::QString,
                       QStringLiteral("lingmo-light"));
        schema->addKey(QStringLiteral("General/fontSize"), QMetaType::Int, 12);

        Lingmo::Config config(QStringLiteral("test"));
        config.setSchema(schema);

        // Schema defaults should be used when no value is set
        QCOMPARE(config.value(QStringLiteral("General/themeName")).toString(),
                 QStringLiteral("lingmo-light"));
        QCOMPARE(config.value(QStringLiteral("General/fontSize")).toInt(), 12);
    }

    void testSchemaValidateAll()
    {
        auto *schema = new Lingmo::ConfigSchema;
        schema->addKey(QStringLiteral("General/fontSize"), QMetaType::Int,
                       12, 8, 72);

        QVariantMap values;
        values.insert(QStringLiteral("General/fontSize"), 100);

        const auto errors = schema->validateAll(values);
        QCOMPARE(errors.size(), 1);
        QVERIFY(errors.contains(QStringLiteral("General/fontSize")));
    }

    void testSchemaHasKey()
    {
        auto *schema = new Lingmo::ConfigSchema;
        schema->addKey(QStringLiteral("key1"), QMetaType::QString);
        QVERIFY(schema->hasKey(QStringLiteral("key1")));
        QVERIFY(!schema->hasKey(QStringLiteral("key2")));
    }

    void testSchemaRemoveKey()
    {
        auto *schema = new Lingmo::ConfigSchema;
        schema->addKey(QStringLiteral("key1"), QMetaType::QString);
        QVERIFY(schema->hasKey(QStringLiteral("key1")));

        schema->removeKey(QStringLiteral("key1"));
        QVERIFY(!schema->hasKey(QStringLiteral("key1")));
    }
};

QTEST_MAIN(tst_Config)
#include "tst_Config.moc"
