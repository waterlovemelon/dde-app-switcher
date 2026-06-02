#include <QtTest/QtTest>
#include "core/Config.h"
#include "core/ConfigManager.h"

#include <QDir>
#include <QTemporaryDir>

using namespace oopsjump;

class ConfigTest : public QObject {
    Q_OBJECT

private slots:
    void defaultsAreSane()
    {
        const Config config = Config::defaults();
        QCOMPARE(config.version, 1);
        QVERIFY(config.general.enabled);
        QCOMPARE(config.hotkey.mode, HotkeyMode::Direct);
        QCOMPARE(config.window.defaultStrategy, MultiWindowStrategy::Cycle);
        QVERIFY(config.bindings.isEmpty());
    }

    void strategyRoundTrips()
    {
        QCOMPARE(multiWindowStrategyFromString("cycle"), MultiWindowStrategy::Cycle);
        QCOMPARE(multiWindowStrategyFromString("recent"), MultiWindowStrategy::Recent);
        QCOMPARE(multiWindowStrategyFromString("picker"), MultiWindowStrategy::Picker);
        QCOMPARE(multiWindowStrategyFromString("default"), MultiWindowStrategy::Default);
        QCOMPARE(multiWindowStrategyFromString("bogus"), MultiWindowStrategy::Default);
    }

    void modeRoundTrips()
    {
        QCOMPARE(hotkeyModeFromString("direct"), HotkeyMode::Direct);
        QCOMPARE(hotkeyModeFromString("leader"), HotkeyMode::Leader);
        QCOMPARE(hotkeyModeFromString("bogus"), HotkeyMode::Direct);
    }

    void loadMissingFileReturnsDefaults()
    {
        const ConfigManager manager("/nonexistent/path/config.json");
        const auto result = manager.load();
        QVERIFY(result.ok);
        QCOMPARE(result.value.version, 1);
        QVERIFY(result.value.bindings.isEmpty());
    }

    void saveAndLoadRoundTrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + "/config.json";

        Config config = Config::defaults();
        config.general.autostart = true;
        config.general.logLevel = "debug";
        config.hotkey.mode = HotkeyMode::Leader;
        config.hotkey.leaderKey = "Ctrl+Space";
        config.window.defaultStrategy = MultiWindowStrategy::Recent;
        config.window.cycleTimeoutMs = 500;

        Binding binding;
        binding.id = "test-binding";
        binding.enabled = true;
        binding.hotkey = "Alt+1";
        binding.desktopId = "org.example.app";
        binding.strategy = MultiWindowStrategy::Cycle;
        binding.launchIfNotRunning = false;
        config.bindings.append(binding);

        const ConfigManager manager(path);
        const auto saveResult = manager.save(config);
        QVERIFY2(saveResult.ok, qPrintable(saveResult.message));

        const auto loadResult = manager.load();
        QVERIFY(loadResult.ok);
        QCOMPARE(loadResult.value.general.autostart, true);
        QCOMPARE(loadResult.value.general.logLevel, QString("debug"));
        QCOMPARE(loadResult.value.hotkey.mode, HotkeyMode::Leader);
        QCOMPARE(loadResult.value.hotkey.leaderKey, QString("Ctrl+Space"));
        QCOMPARE(loadResult.value.window.defaultStrategy, MultiWindowStrategy::Recent);
        QCOMPARE(loadResult.value.window.cycleTimeoutMs, 500);
        QCOMPARE(loadResult.value.bindings.size(), 1);
        QCOMPARE(loadResult.value.bindings.first().id, QString("test-binding"));
        QCOMPARE(loadResult.value.bindings.first().hotkey, QString("Alt+1"));
        QCOMPARE(loadResult.value.bindings.first().desktopId, QString("org.example.app"));
        QCOMPARE(loadResult.value.bindings.first().strategy, MultiWindowStrategy::Cycle);
        QCOMPARE(loadResult.value.bindings.first().launchIfNotRunning, false);
    }

    void validateRejectsEmptyBindingId()
    {
        Config config = Config::defaults();
        Binding binding;
        binding.id = "  ";
        binding.hotkey = "Alt+1";
        config.bindings.append(binding);

        const auto result = ConfigManager::validate(config);
        QVERIFY(!result.ok);
        QCOMPARE(result.errorCode, QString("config_validation_failed"));
    }

    void validateRejectsDuplicateIds()
    {
        Config config = Config::defaults();
        Binding b1;
        b1.id = "same-id";
        b1.hotkey = "Alt+1";
        Binding b2;
        b2.id = "same-id";
        b2.hotkey = "Alt+2";
        config.bindings.append(b1);
        config.bindings.append(b2);

        const auto result = ConfigManager::validate(config);
        QVERIFY(!result.ok);
        QCOMPARE(result.errorCode, QString("config_validation_failed"));
    }

    void loadInvalidJsonReturnsError()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + "/bad.json";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{invalid json");
        file.close();

        const ConfigManager manager(path);
        const auto result = manager.load();
        QVERIFY(!result.ok);
        QCOMPARE(result.errorCode, QString("config_parse_failed"));
    }
};

QTEST_MAIN(ConfigTest)
#include "test_config.moc"
