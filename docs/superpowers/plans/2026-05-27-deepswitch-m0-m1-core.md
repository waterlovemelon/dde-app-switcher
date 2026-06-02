# Oops Jump M0-M1 Core Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the M0-M1 command-line Oops Jump core: a testable Qt/C++20 agent that can load config, parse hotkeys, scan `.desktop` files, match X11 windows to apps, register X11 global hotkeys, and launch or focus configured applications.

**Architecture:** Start with a small CMake/Qt project and isolate pure core logic from X11-specific backends. Implement unit-tested config, hotkey, desktop parsing, matching, and action decisions first; then connect those units through X11 backends and a minimal `oops-jump-agent` CLI.

**Tech Stack:** C++20, CMake, Qt6 Core, Qt6 Test, Qt6 Gui, X11/Xlib, optional XCB only after the Xlib prototype is stable.

---

## Scope

This plan covers the v0.2 design's M0 and M1 work:

- M0 X11 technical validation.
- M1 command-line prototype.
- Unit tests for pure logic.
- Manual validation commands for real X11 behavior.

This plan excludes:

- Settings UI.
- D-Bus IPC.
- deb packaging.
- autostart.
- Wayland/Treeland implementation.
- polished Overlay UI.

Those belong in later plans after this core path works on deepin v25 X11.

## File Structure

Create this structure:

```text
CMakeLists.txt
src/
├── agent/
│   └── main.cpp
├── core/
│   ├── ActionEngine.cpp
│   ├── ActionEngine.h
│   ├── AppInfo.h
│   ├── AppMatcher.cpp
│   ├── AppMatcher.h
│   ├── AppRegistry.cpp
│   ├── AppRegistry.h
│   ├── Config.cpp
│   ├── Config.h
│   ├── ConfigManager.cpp
│   ├── ConfigManager.h
│   ├── DesktopEntry.cpp
│   ├── DesktopEntry.h
│   ├── Hotkey.cpp
│   ├── Hotkey.h
│   ├── Launcher.cpp
│   ├── Launcher.h
│   ├── Result.h
│   ├── WindowInfo.h
│   └── X11Types.h
└── backends/
    └── x11/
        ├── X11Connection.cpp
        ├── X11Connection.h
        ├── X11HotkeyBackend.cpp
        ├── X11HotkeyBackend.h
        ├── X11WindowBackend.cpp
        └── X11WindowBackend.h
tests/
├── CMakeLists.txt
├── core/
│   ├── test_app_matcher.cpp
│   ├── test_config.cpp
│   ├── test_desktop_entry.cpp
│   └── test_hotkey.cpp
└── fixtures/
    └── applications/
        ├── code.desktop
        ├── firefox.desktop
        └── hidden.desktop
docs/
├── m0-x11-findings.md
└── superpowers/
    └── plans/
        └── 2026-05-27-deepswitch-m0-m1-core.md
```

Boundary rules:

- `src/core` must not include X11 headers.
- `src/backends/x11` is the only place that includes `<X11/Xlib.h>` and `<X11/Xatom.h>`.
- `src/agent/main.cpp` wires modules together and owns CLI behavior.
- Tests target `oopsjump_core`, not the agent executable.

---

### Task 1: Project Scaffold

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/agent/main.cpp`
- Create: `src/core/Result.h`
- Create: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the top-level CMake file**

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)

project(oops-jump VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test Gui)
find_package(X11 REQUIRED)

enable_testing()

add_library(oopsjump_core
    src/core/ActionEngine.cpp
    src/core/AppMatcher.cpp
    src/core/AppRegistry.cpp
    src/core/Config.cpp
    src/core/ConfigManager.cpp
    src/core/DesktopEntry.cpp
    src/core/Hotkey.cpp
    src/core/Launcher.cpp
)

target_include_directories(oopsjump_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(oopsjump_core PUBLIC
    Qt6::Core
)

add_library(oopsjump_x11
    src/backends/x11/X11Connection.cpp
    src/backends/x11/X11HotkeyBackend.cpp
    src/backends/x11/X11WindowBackend.cpp
)

target_include_directories(oopsjump_x11 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(oopsjump_x11 PUBLIC
    oopsjump_core
    Qt6::Core
    Qt6::Gui
    X11::X11
)

add_executable(oops-jump-agent
    src/agent/main.cpp
)

target_link_libraries(oops-jump-agent PRIVATE
    oopsjump_core
    oopsjump_x11
    Qt6::Core
)

add_subdirectory(tests)
```

- [ ] **Step 2: Create a minimal agent entrypoint**

Create `src/agent/main.cpp`:

```cpp
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("oops-jump-agent");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Oops Jump command-line agent");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({ "validate-config", "Validate the config file and exit." });
    parser.addOption({ "list-bindings", "List configured bindings and exit." });
    parser.addOption({ "list-apps", "List desktop applications and exit." });
    parser.addOption({ "list-windows", "List X11 windows and exit." });
    parser.addOption({ "trigger", "Trigger an action id and exit.", "action-id" });
    parser.addOption({ "config", "Use a specific config file.", "path" });
    parser.process(app);

    QTextStream out(stdout);
    out << "oops-jump-agent scaffold ready\n";
    return 0;
}
```

- [ ] **Step 3: Create the shared Result type**

Create `src/core/Result.h`:

```cpp
#pragma once

#include <QString>
#include <utility>

namespace oopsjump {

template <typename T>
struct Result {
    bool ok = false;
    T value {};
    QString errorCode;
    QString message;

    static Result success(T resultValue)
    {
        Result result;
        result.ok = true;
        result.value = std::move(resultValue);
        return result;
    }

    static Result failure(QString code, QString failureMessage)
    {
        Result result;
        result.ok = false;
        result.errorCode = std::move(code);
        result.message = std::move(failureMessage);
        return result;
    }
};

struct VoidResult {
    bool ok = false;
    QString errorCode;
    QString message;

    static VoidResult success()
    {
        return { true, {}, {} };
    }

    static VoidResult failure(QString code, QString failureMessage)
    {
        return { false, std::move(code), std::move(failureMessage) };
    }
};

}
```

- [ ] **Step 4: Create the test CMake file**

Create `tests/CMakeLists.txt`:

```cmake
function(add_oopsjump_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE oopsjump_core Qt6::Test Qt6::Core)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()
```

- [ ] **Step 5: Add empty implementation files so the scaffold configures**

Create these files with only an include of their matching header after the header exists in later tasks:

```text
src/core/ActionEngine.cpp
src/core/AppMatcher.cpp
src/core/AppRegistry.cpp
src/core/Config.cpp
src/core/ConfigManager.cpp
src/core/DesktopEntry.cpp
src/core/Hotkey.cpp
src/core/Launcher.cpp
src/backends/x11/X11Connection.cpp
src/backends/x11/X11HotkeyBackend.cpp
src/backends/x11/X11WindowBackend.cpp
```

For this step, create each `.cpp` file with this exact temporary content:

```cpp
#include "core/Config.h"
```

When the task for each module is implemented, replace the temporary include with the correct header.

- [ ] **Step 6: Configure the build**

Run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

Expected: CMake configuration succeeds after the source files from Step 5 exist. If it fails because Qt6 or X11 development packages are missing, install the platform packages before continuing.

- [ ] **Step 7: Commit the scaffold**

```bash
git add CMakeLists.txt src tests
git commit -m "chore: scaffold oops-jump core project"
```

---

### Task 2: Core Data Types

**Files:**
- Create: `src/core/Config.h`
- Create: `src/core/Config.cpp`
- Create: `src/core/AppInfo.h`
- Create: `src/core/WindowInfo.h`
- Create: `src/core/X11Types.h`
- Modify: `tests/CMakeLists.txt`
- Test: `tests/core/test_config.cpp`

- [ ] **Step 1: Register the config test target**

Append to `tests/CMakeLists.txt`:

```cmake
add_oopsjump_test(test_config core/test_config.cpp)
```

- [ ] **Step 2: Write the failing config default test**

Create `tests/core/test_config.cpp`:

```cpp
#include <QtTest/QtTest>
#include "core/Config.h"

using namespace oopsjump;

class ConfigTest : public QObject {
    Q_OBJECT

private slots:
    void defaultConfigHasSafeValues()
    {
        const Config config = Config::defaults();
        QCOMPARE(config.version, 1);
        QVERIFY(config.general.enabled);
        QCOMPARE(config.general.sessionBackend, QString("auto"));
        QCOMPARE(config.hotkey.mode, HotkeyMode::Direct);
        QCOMPARE(config.window.cycleTimeoutMs, 1200);
        QVERIFY(config.bindings.isEmpty());
    }

    void strategyFromStringFallsBackToDefault()
    {
        QCOMPARE(multiWindowStrategyFromString("recent"), MultiWindowStrategy::Recent);
        QCOMPARE(multiWindowStrategyFromString("cycle"), MultiWindowStrategy::Cycle);
        QCOMPARE(multiWindowStrategyFromString("picker"), MultiWindowStrategy::Picker);
        QCOMPARE(multiWindowStrategyFromString("unknown"), MultiWindowStrategy::Default);
    }
};

QTEST_MAIN(ConfigTest)
#include "test_config.moc"
```

- [ ] **Step 3: Run the test and verify it fails**

Run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target test_config -j"$(nproc)"
```

Expected: FAIL because `src/core/Config.h` does not exist or `Config::defaults()` is not defined.

- [ ] **Step 4: Implement config data types**

Create `src/core/Config.h`:

```cpp
#pragma once

#include <QList>
#include <QString>

namespace oopsjump {

enum class HotkeyMode {
    Direct,
    Leader
};

enum class MultiWindowStrategy {
    Default,
    Recent,
    Cycle,
    Picker
};

struct GeneralConfig {
    bool enabled = true;
    bool autostart = false;
    QString sessionBackend = "auto";
    bool showOverlay = true;
    QString logLevel = "info";
};

struct HotkeyConfig {
    HotkeyMode mode = HotkeyMode::Direct;
    QString leaderKey = "Alt+Space";
    int leaderTimeoutMs = 1500;
};

struct WindowConfig {
    MultiWindowStrategy defaultStrategy = MultiWindowStrategy::Cycle;
    int cycleTimeoutMs = 1200;
    int launchTimeoutMs = 8000;
    bool includeAllWorkspaces = true;
    bool switchWorkspaceWhenNeeded = true;
};

struct MatchRule {
    QString type;
    QString op;
    QString value;
    int weight = 0;
    QString effect = "include";
};

struct Binding {
    QString id;
    bool enabled = true;
    QString hotkey;
    QString selectionKey;
    QString desktopId;
    QString command;
    MultiWindowStrategy strategy = MultiWindowStrategy::Default;
    bool launchIfNotRunning = true;
    bool focusExistingWindow = true;
    QList<MatchRule> matchRules;
};

struct Config {
    int version = 1;
    GeneralConfig general;
    HotkeyConfig hotkey;
    WindowConfig window;
    QList<Binding> bindings;

    static Config defaults();
};

QString multiWindowStrategyToString(MultiWindowStrategy strategy);
MultiWindowStrategy multiWindowStrategyFromString(const QString& value);
QString hotkeyModeToString(HotkeyMode mode);
HotkeyMode hotkeyModeFromString(const QString& value);

}
```

Create `src/core/Config.cpp`:

```cpp
#include "core/Config.h"

namespace oopsjump {

Config Config::defaults()
{
    return {};
}

QString multiWindowStrategyToString(MultiWindowStrategy strategy)
{
    switch (strategy) {
    case MultiWindowStrategy::Recent:
        return "recent";
    case MultiWindowStrategy::Cycle:
        return "cycle";
    case MultiWindowStrategy::Picker:
        return "picker";
    case MultiWindowStrategy::Default:
        return "default";
    }
    return "default";
}

MultiWindowStrategy multiWindowStrategyFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == "recent") {
        return MultiWindowStrategy::Recent;
    }
    if (normalized == "cycle") {
        return MultiWindowStrategy::Cycle;
    }
    if (normalized == "picker") {
        return MultiWindowStrategy::Picker;
    }
    return MultiWindowStrategy::Default;
}

QString hotkeyModeToString(HotkeyMode mode)
{
    switch (mode) {
    case HotkeyMode::Direct:
        return "direct";
    case HotkeyMode::Leader:
        return "leader";
    }
    return "direct";
}

HotkeyMode hotkeyModeFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == "leader") {
        return HotkeyMode::Leader;
    }
    return HotkeyMode::Direct;
}

}
```

Create `src/core/AppInfo.h`:

```cpp
#pragma once

#include <QString>
#include <QStringList>

namespace oopsjump {

struct AppInfo {
    QString desktopId;
    QString name;
    QString localizedName;
    QString exec;
    QString icon;
    QString startupWmClass;
    QStringList categories;
    QString desktopFilePath;
    bool terminal = false;
    bool noDisplay = false;
    bool hidden = false;
};

}
```

Create `src/core/WindowInfo.h`:

```cpp
#pragma once

#include <QString>

namespace oopsjump {

using WindowId = quint64;

struct WindowInfo {
    WindowId id = 0;
    QString title;
    QString wmClass;
    QString instanceName;
    int pid = 0;
    int desktop = -1;
    bool minimized = false;
    bool active = false;
    int lastActiveOrder = 0;
    QString windowType;
    QString appId;
    bool skipTaskbar = false;
};

}
```

Create `src/core/X11Types.h`:

```cpp
#pragma once

namespace oopsjump {

struct X11KeyRegistration {
    int keycode = 0;
    unsigned int modifiers = 0;
};

}
```

- [ ] **Step 5: Run the test and verify it passes**

Run:

```bash
cmake --build build --target test_config -j"$(nproc)"
ctest --test-dir build -R test_config --output-on-failure
```

Expected: PASS for `test_config`.

- [ ] **Step 6: Commit core data types**

```bash
git add src/core tests/core/test_config.cpp
git commit -m "feat: add core config data types"
```

---

### Task 3: Hotkey Parser

**Files:**
- Create: `src/core/Hotkey.h`
- Create: `src/core/Hotkey.cpp`
- Modify: `tests/CMakeLists.txt`
- Test: `tests/core/test_hotkey.cpp`

- [ ] **Step 1: Register the hotkey test target**

Append to `tests/CMakeLists.txt`:

```cmake
add_oopsjump_test(test_hotkey core/test_hotkey.cpp)
```

- [ ] **Step 2: Write failing hotkey parser tests**

Create `tests/core/test_hotkey.cpp`:

```cpp
#include <QtTest/QtTest>
#include "core/Hotkey.h"

using namespace oopsjump;

class HotkeyTest : public QObject {
    Q_OBJECT

private slots:
    void normalizesAliases()
    {
        const auto parsed = Hotkey::parse("control + win + return");
        QVERIFY(parsed.ok);
        QCOMPARE(parsed.value.sequence, QString("Ctrl+Meta+Enter"));
        QCOMPARE(parsed.value.key, QString("Enter"));
        QVERIFY(parsed.value.modifiers.contains("Ctrl"));
        QVERIFY(parsed.value.modifiers.contains("Meta"));
    }

    void rejectsModifierOnlyHotkey()
    {
        const auto parsed = Hotkey::parse("Alt");
        QVERIFY(!parsed.ok);
        QCOMPARE(parsed.errorCode, QString("hotkey_invalid"));
    }

    void detectsDuplicateModifier()
    {
        const auto parsed = Hotkey::parse("Alt+Alt+1");
        QVERIFY(!parsed.ok);
        QCOMPARE(parsed.errorCode, QString("hotkey_invalid"));
    }

    void normalizesCaseAndSpacing()
    {
        const auto parsed = Hotkey::parse(" alt + shift + q ");
        QVERIFY(parsed.ok);
        QCOMPARE(parsed.value.sequence, QString("Alt+Shift+Q"));
    }
};

QTEST_MAIN(HotkeyTest)
#include "test_hotkey.moc"
```

- [ ] **Step 3: Run the test and verify it fails**

Run:

```bash
cmake --build build --target test_hotkey -j"$(nproc)"
```

Expected: FAIL because `Hotkey::parse` is not defined.

- [ ] **Step 4: Implement the parser**

Create `src/core/Hotkey.h`:

```cpp
#pragma once

#include "core/Result.h"
#include <QString>
#include <QStringList>

namespace oopsjump {

struct Hotkey {
    QString sequence;
    QString key;
    QStringList modifiers;

    static Result<Hotkey> parse(const QString& input);
};

}
```

Create `src/core/Hotkey.cpp`:

```cpp
#include "core/Hotkey.h"

#include <QSet>

namespace oopsjump {

static QString normalizeToken(const QString& token)
{
    const QString t = token.trimmed().toLower();
    if (t == "control") {
        return "Ctrl";
    }
    if (t == "ctrl") {
        return "Ctrl";
    }
    if (t == "alt") {
        return "Alt";
    }
    if (t == "shift") {
        return "Shift";
    }
    if (t == "super" || t == "meta" || t == "win") {
        return "Meta";
    }
    if (t == "return" || t == "enter") {
        return "Enter";
    }
    if (t == "esc" || t == "escape") {
        return "Esc";
    }
    if (t.size() == 1) {
        return t.toUpper();
    }
    return token.trimmed();
}

static bool isModifier(const QString& token)
{
    return token == "Ctrl" || token == "Alt" || token == "Shift" || token == "Meta";
}

Result<Hotkey> Hotkey::parse(const QString& input)
{
    const QStringList rawTokens = input.split('+', Qt::SkipEmptyParts);
    if (rawTokens.isEmpty()) {
        return Result<Hotkey>::failure("hotkey_invalid", "Hotkey is empty.");
    }

    QStringList modifiers;
    QSet<QString> seenModifiers;
    QString key;

    for (const QString& rawToken : rawTokens) {
        const QString token = normalizeToken(rawToken);
        if (token.isEmpty()) {
            return Result<Hotkey>::failure("hotkey_invalid", "Hotkey contains an empty token.");
        }

        if (isModifier(token)) {
            if (seenModifiers.contains(token)) {
                return Result<Hotkey>::failure("hotkey_invalid", "Hotkey contains a duplicate modifier.");
            }
            seenModifiers.insert(token);
            modifiers.append(token);
            continue;
        }

        if (!key.isEmpty()) {
            return Result<Hotkey>::failure("hotkey_invalid", "Hotkey contains more than one main key.");
        }
        key = token;
    }

    if (key.isEmpty()) {
        return Result<Hotkey>::failure("hotkey_invalid", "Hotkey must contain a main key.");
    }

    const QStringList modifierOrder = { "Ctrl", "Alt", "Shift", "Meta" };
    QStringList orderedModifiers;
    for (const QString& candidate : modifierOrder) {
        if (modifiers.contains(candidate)) {
            orderedModifiers.append(candidate);
        }
    }

    Hotkey hotkey;
    QStringList sequenceParts = orderedModifiers;
    sequenceParts.append(key);
    hotkey.key = key;
    hotkey.modifiers = orderedModifiers;
    hotkey.sequence = sequenceParts.join("+");
    return Result<Hotkey>::success(hotkey);
}

}
```

- [ ] **Step 5: Run hotkey tests**

Run:

```bash
cmake --build build --target test_hotkey -j"$(nproc)"
ctest --test-dir build -R test_hotkey --output-on-failure
```

Expected: PASS for `test_hotkey`.

- [ ] **Step 6: Run all current tests**

Run:

```bash
ctest --test-dir build --output-on-failure
```

Expected: PASS for `test_config` and `test_hotkey`.

- [ ] **Step 7: Commit hotkey parser**

```bash
git add src/core/Hotkey.h src/core/Hotkey.cpp tests/core/test_hotkey.cpp
git commit -m "feat: add hotkey parser"
```

---

### Task 4: Desktop Entry Parser and App Registry

**Files:**
- Create: `src/core/DesktopEntry.h`
- Create: `src/core/DesktopEntry.cpp`
- Create: `src/core/AppRegistry.h`
- Create: `src/core/AppRegistry.cpp`
- Create: `tests/fixtures/applications/firefox.desktop`
- Create: `tests/fixtures/applications/code.desktop`
- Create: `tests/fixtures/applications/hidden.desktop`
- Modify: `tests/CMakeLists.txt`
- Test: `tests/core/test_desktop_entry.cpp`

- [ ] **Step 1: Create desktop fixtures**

Create `tests/fixtures/applications/firefox.desktop`:

```ini
[Desktop Entry]
Type=Application
Name=Firefox
Name[zh_CN]=火狐浏览器
Exec=firefox %u
Icon=firefox
StartupWMClass=firefox
Categories=Network;WebBrowser;
Terminal=false
NoDisplay=false
```

Create `tests/fixtures/applications/code.desktop`:

```ini
[Desktop Entry]
Type=Application
Name=Visual Studio Code
Exec=/usr/bin/code --unity-launch %F
Icon=code
StartupWMClass=code
Categories=Development;IDE;
Terminal=false
NoDisplay=false
```

Create `tests/fixtures/applications/hidden.desktop`:

```ini
[Desktop Entry]
Type=Application
Name=Hidden Tool
Exec=hidden-tool
Hidden=true
Terminal=false
```

- [ ] **Step 2: Register the desktop parser test target**

Append to `tests/CMakeLists.txt`:

```cmake
add_oopsjump_test(test_desktop_entry core/test_desktop_entry.cpp)
```

- [ ] **Step 3: Write failing desktop parser tests**

Create `tests/core/test_desktop_entry.cpp`:

```cpp
#include <QtTest/QtTest>
#include "core/DesktopEntry.h"
#include "core/AppRegistry.h"

using namespace oopsjump;

class DesktopEntryTest : public QObject {
    Q_OBJECT

private slots:
    void parsesLocalizedNameAndStartupClass()
    {
        const auto result = DesktopEntry::fromFile("tests/fixtures/applications/firefox.desktop");
        QVERIFY(result.ok);
        QCOMPARE(result.value.name, QString("Firefox"));
        QCOMPARE(result.value.localizedName, QString("火狐浏览器"));
        QCOMPARE(result.value.startupWmClass, QString("firefox"));
        QCOMPARE(result.value.exec, QString("firefox"));
    }

    void removesDesktopFieldCodes()
    {
        QCOMPARE(DesktopEntry::cleanExec("/usr/bin/code --unity-launch %F"), QString("/usr/bin/code --unity-launch"));
        QCOMPARE(DesktopEntry::cleanExec("firefox %u"), QString("firefox"));
    }

    void registryFiltersHiddenApplications()
    {
        AppRegistry registry;
        registry.setApplicationDirs({ "tests/fixtures/applications" });
        const auto scan = registry.scan();
        QVERIFY(scan.ok);
        const QList<AppInfo> apps = registry.listApplications();
        QCOMPARE(apps.size(), 2);
        QVERIFY(registry.findByDesktopId("firefox.desktop").has_value());
        QVERIFY(!registry.findByDesktopId("hidden.desktop").has_value());
    }
};

QTEST_MAIN(DesktopEntryTest)
#include "test_desktop_entry.moc"
```

- [ ] **Step 4: Run the test and verify it fails**

Run:

```bash
cmake --build build --target test_desktop_entry -j"$(nproc)"
```

Expected: FAIL because `DesktopEntry` and `AppRegistry` are not implemented.

- [ ] **Step 5: Implement DesktopEntry**

Create `src/core/DesktopEntry.h`:

```cpp
#pragma once

#include "core/AppInfo.h"
#include "core/Result.h"
#include <QString>

namespace oopsjump {

class DesktopEntry {
public:
    static Result<AppInfo> fromFile(const QString& path);
    static QString cleanExec(const QString& exec);
};

}
```

Create `src/core/DesktopEntry.cpp`:

```cpp
#include "core/DesktopEntry.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace oopsjump {

static bool parseBool(const QString& value)
{
    return value.trimmed().compare("true", Qt::CaseInsensitive) == 0;
}

QString DesktopEntry::cleanExec(const QString& exec)
{
    QString cleaned = exec;
    const QStringList fieldCodes = { "%f", "%F", "%u", "%U", "%i", "%c", "%k" };
    for (const QString& code : fieldCodes) {
        cleaned.replace(code, "");
    }
    return cleaned.simplified();
}

Result<AppInfo> DesktopEntry::fromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return Result<AppInfo>::failure("desktop_file_invalid", "Cannot open desktop file.");
    }

    AppInfo app;
    app.desktopFilePath = QFileInfo(path).absoluteFilePath();
    app.desktopId = QFileInfo(path).fileName();

    bool inDesktopEntry = false;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        if (line == "[Desktop Entry]") {
            inDesktopEntry = true;
            continue;
        }
        if (line.startsWith('[') && line.endsWith(']')) {
            inDesktopEntry = false;
            continue;
        }
        if (!inDesktopEntry) {
            continue;
        }

        const int equals = line.indexOf('=');
        if (equals <= 0) {
            continue;
        }
        const QString key = line.left(equals);
        const QString value = line.mid(equals + 1);

        if (key == "Type" && value != "Application") {
            return Result<AppInfo>::failure("desktop_file_invalid", "Desktop entry is not an application.");
        }
        if (key == "Name") {
            app.name = value;
        } else if (key == "Name[zh_CN]") {
            app.localizedName = value;
        } else if (key == "Exec") {
            app.exec = cleanExec(value);
        } else if (key == "Icon") {
            app.icon = value;
        } else if (key == "StartupWMClass") {
            app.startupWmClass = value;
        } else if (key == "Categories") {
            app.categories = value.split(';', Qt::SkipEmptyParts);
        } else if (key == "Terminal") {
            app.terminal = parseBool(value);
        } else if (key == "NoDisplay") {
            app.noDisplay = parseBool(value);
        } else if (key == "Hidden") {
            app.hidden = parseBool(value);
        }
    }

    if (app.name.isEmpty() || app.exec.isEmpty()) {
        return Result<AppInfo>::failure("desktop_file_invalid", "Desktop entry is missing Name or Exec.");
    }
    if (app.localizedName.isEmpty()) {
        app.localizedName = app.name;
    }

    return Result<AppInfo>::success(app);
}

}
```

- [ ] **Step 6: Implement AppRegistry**

Create `src/core/AppRegistry.h`:

```cpp
#pragma once

#include "core/AppInfo.h"
#include "core/Result.h"
#include <QList>
#include <QStringList>
#include <optional>

namespace oopsjump {

class AppRegistry {
public:
    void setApplicationDirs(QStringList dirs);
    Result<int> scan();
    QList<AppInfo> listApplications() const;
    std::optional<AppInfo> findByDesktopId(const QString& desktopId) const;

private:
    QStringList m_dirs;
    QList<AppInfo> m_apps;
};

}
```

Create `src/core/AppRegistry.cpp`:

```cpp
#include "core/AppRegistry.h"
#include "core/DesktopEntry.h"

#include <QDir>
#include <QSet>

namespace oopsjump {

void AppRegistry::setApplicationDirs(QStringList dirs)
{
    m_dirs = std::move(dirs);
}

Result<int> AppRegistry::scan()
{
    m_apps.clear();
    QSet<QString> seenDesktopIds;

    const QStringList dirs = m_dirs.isEmpty()
        ? QStringList{ QDir::homePath() + "/.local/share/applications", "/usr/local/share/applications", "/usr/share/applications" }
        : m_dirs;

    for (const QString& dirPath : dirs) {
        QDir dir(dirPath);
        const QStringList files = dir.entryList({ "*.desktop" }, QDir::Files, QDir::Name);
        for (const QString& fileName : files) {
            if (seenDesktopIds.contains(fileName)) {
                continue;
            }
            const auto parsed = DesktopEntry::fromFile(dir.absoluteFilePath(fileName));
            if (!parsed.ok) {
                continue;
            }
            const AppInfo app = parsed.value;
            if (app.hidden || app.noDisplay) {
                continue;
            }
            seenDesktopIds.insert(app.desktopId);
            m_apps.append(app);
        }
    }

    std::sort(m_apps.begin(), m_apps.end(), [](const AppInfo& left, const AppInfo& right) {
        return left.localizedName.localeAwareCompare(right.localizedName) < 0;
    });

    return Result<int>::success(m_apps.size());
}

QList<AppInfo> AppRegistry::listApplications() const
{
    return m_apps;
}

std::optional<AppInfo> AppRegistry::findByDesktopId(const QString& desktopId) const
{
    for (const AppInfo& app : m_apps) {
        if (app.desktopId == desktopId) {
            return app;
        }
    }
    return std::nullopt;
}

}
```

- [ ] **Step 7: Run desktop entry tests**

Run:

```bash
cmake --build build --target test_desktop_entry -j"$(nproc)"
ctest --test-dir build -R test_desktop_entry --output-on-failure
```

Expected: PASS for `test_desktop_entry`.

- [ ] **Step 8: Commit desktop parsing and app registry**

```bash
git add src/core tests/core/test_desktop_entry.cpp tests/fixtures
git commit -m "feat: add desktop app registry"
```

---

### Task 5: Config Manager

**Files:**
- Create: `src/core/ConfigManager.h`
- Create: `src/core/ConfigManager.cpp`
- Modify: `tests/core/test_config.cpp`

- [ ] **Step 1: Add failing ConfigManager tests**

Append these test methods inside `ConfigTest` in `tests/core/test_config.cpp`:

```cpp
    void loadsConfigWithBinding()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("config.json");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(R"json({
  "version": 1,
  "general": { "enabled": true, "session_backend": "auto" },
  "hotkey": { "mode": "direct", "leader_key": "Alt+Space", "leader_timeout_ms": 1500 },
  "window": { "default_multi_window_strategy": "cycle", "cycle_timeout_ms": 1200, "launch_timeout_ms": 8000 },
  "bindings": [
    {
      "id": "app.firefox",
      "enabled": true,
      "hotkey": "Alt+1",
      "desktop_id": "firefox.desktop",
      "multi_window_strategy": "cycle"
    }
  ]
})json");
        file.close();

        ConfigManager manager(path);
        const auto loaded = manager.load();
        QVERIFY(loaded.ok);
        QCOMPARE(loaded.value.bindings.size(), 1);
        QCOMPARE(loaded.value.bindings[0].id, QString("app.firefox"));
        QCOMPARE(loaded.value.bindings[0].hotkey, QString("Alt+1"));
        QCOMPARE(loaded.value.bindings[0].strategy, MultiWindowStrategy::Cycle);
    }

    void rejectsDuplicateBindingIds()
    {
        Config config = Config::defaults();
        Binding first;
        first.id = "app.firefox";
        first.hotkey = "Alt+1";
        Binding second;
        second.id = "app.firefox";
        second.hotkey = "Alt+2";
        config.bindings = { first, second };

        const auto validation = ConfigManager::validate(config);
        QVERIFY(!validation.ok);
        QCOMPARE(validation.errorCode, QString("config_validation_failed"));
    }
```

Add includes to the top:

```cpp
#include "core/ConfigManager.h"
#include <QFile>
#include <QTemporaryDir>
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```bash
cmake --build build --target test_config -j"$(nproc)"
```

Expected: FAIL because `ConfigManager` is not defined.

- [ ] **Step 3: Implement ConfigManager**

Create `src/core/ConfigManager.h`:

```cpp
#pragma once

#include "core/Config.h"
#include "core/Result.h"
#include <QString>

namespace oopsjump {

class ConfigManager {
public:
    explicit ConfigManager(QString path);

    Result<Config> load() const;
    VoidResult save(const Config& config) const;
    static VoidResult validate(const Config& config);

private:
    QString m_path;
};

}
```

Create `src/core/ConfigManager.cpp`:

```cpp
#include "core/ConfigManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <utility>

namespace oopsjump {

ConfigManager::ConfigManager(QString path)
    : m_path(std::move(path))
{
}

static Binding bindingFromJson(const QJsonObject& object)
{
    Binding binding;
    binding.id = object.value("id").toString();
    binding.enabled = object.value("enabled").toBool(true);
    binding.hotkey = object.value("hotkey").toString();
    binding.selectionKey = object.value("selection_key").toString();
    binding.desktopId = object.value("desktop_id").toString();
    binding.command = object.value("command").toString();
    binding.strategy = multiWindowStrategyFromString(object.value("multi_window_strategy").toString("default"));
    binding.launchIfNotRunning = object.value("launch_if_not_running").toBool(true);
    binding.focusExistingWindow = object.value("focus_existing_window").toBool(true);
    return binding;
}

static QJsonObject bindingToJson(const Binding& binding)
{
    QJsonObject object;
    object["id"] = binding.id;
    object["enabled"] = binding.enabled;
    object["hotkey"] = binding.hotkey;
    object["selection_key"] = binding.selectionKey;
    object["desktop_id"] = binding.desktopId;
    object["command"] = binding.command;
    object["multi_window_strategy"] = multiWindowStrategyToString(binding.strategy);
    object["launch_if_not_running"] = binding.launchIfNotRunning;
    object["focus_existing_window"] = binding.focusExistingWindow;
    return object;
}

Result<Config> ConfigManager::load() const
{
    QFile file(m_path);
    if (!file.exists()) {
        return Result<Config>::success(Config::defaults());
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<Config>::failure("config_read_failed", "Cannot open config file.");
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return Result<Config>::failure("config_parse_failed", parseError.errorString());
    }

    const QJsonObject root = document.object();
    Config config = Config::defaults();
    config.version = root.value("version").toInt(1);

    const QJsonObject general = root.value("general").toObject();
    config.general.enabled = general.value("enabled").toBool(true);
    config.general.autostart = general.value("autostart").toBool(false);
    config.general.sessionBackend = general.value("session_backend").toString("auto");
    config.general.showOverlay = general.value("show_overlay").toBool(true);
    config.general.logLevel = general.value("log_level").toString("info");

    const QJsonObject hotkey = root.value("hotkey").toObject();
    config.hotkey.mode = hotkeyModeFromString(hotkey.value("mode").toString("direct"));
    config.hotkey.leaderKey = hotkey.value("leader_key").toString("Alt+Space");
    config.hotkey.leaderTimeoutMs = hotkey.value("leader_timeout_ms").toInt(1500);

    const QJsonObject window = root.value("window").toObject();
    config.window.defaultStrategy = multiWindowStrategyFromString(window.value("default_multi_window_strategy").toString("cycle"));
    config.window.cycleTimeoutMs = window.value("cycle_timeout_ms").toInt(1200);
    config.window.launchTimeoutMs = window.value("launch_timeout_ms").toInt(8000);
    config.window.includeAllWorkspaces = window.value("include_all_workspaces").toBool(true);
    config.window.switchWorkspaceWhenNeeded = window.value("switch_workspace_when_needed").toBool(true);

    const QJsonArray bindings = root.value("bindings").toArray();
    for (const QJsonValue& value : bindings) {
        if (value.isObject()) {
            config.bindings.append(bindingFromJson(value.toObject()));
        }
    }

    const auto validation = validate(config);
    if (!validation.ok) {
        return Result<Config>::failure(validation.errorCode, validation.message);
    }

    return Result<Config>::success(config);
}

VoidResult ConfigManager::save(const Config& config) const
{
    const auto validation = validate(config);
    if (!validation.ok) {
        return validation;
    }

    QJsonObject root;
    root["version"] = config.version;

    QJsonObject general;
    general["enabled"] = config.general.enabled;
    general["autostart"] = config.general.autostart;
    general["session_backend"] = config.general.sessionBackend;
    general["show_overlay"] = config.general.showOverlay;
    general["log_level"] = config.general.logLevel;
    root["general"] = general;

    QJsonObject hotkey;
    hotkey["mode"] = hotkeyModeToString(config.hotkey.mode);
    hotkey["leader_key"] = config.hotkey.leaderKey;
    hotkey["leader_timeout_ms"] = config.hotkey.leaderTimeoutMs;
    root["hotkey"] = hotkey;

    QJsonObject window;
    window["default_multi_window_strategy"] = multiWindowStrategyToString(config.window.defaultStrategy);
    window["cycle_timeout_ms"] = config.window.cycleTimeoutMs;
    window["launch_timeout_ms"] = config.window.launchTimeoutMs;
    window["include_all_workspaces"] = config.window.includeAllWorkspaces;
    window["switch_workspace_when_needed"] = config.window.switchWorkspaceWhenNeeded;
    root["window"] = window;

    QJsonArray bindings;
    for (const Binding& binding : config.bindings) {
        bindings.append(bindingToJson(binding));
    }
    root["bindings"] = bindings;

    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        return VoidResult::failure("config_write_failed", "Cannot open config file for writing.");
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        return VoidResult::failure("config_write_failed", "Cannot commit config file.");
    }
    return VoidResult::success();
}

VoidResult ConfigManager::validate(const Config& config)
{
    QSet<QString> ids;
    for (const Binding& binding : config.bindings) {
        if (binding.id.trimmed().isEmpty()) {
            return VoidResult::failure("config_validation_failed", "Binding id is empty.");
        }
        if (ids.contains(binding.id)) {
            return VoidResult::failure("config_validation_failed", "Duplicate binding id: " + binding.id);
        }
        ids.insert(binding.id);
    }
    return VoidResult::success();
}

}
```

- [ ] **Step 4: Run config tests**

Run:

```bash
cmake --build build --target test_config -j"$(nproc)"
ctest --test-dir build -R test_config --output-on-failure
```

Expected: PASS for `test_config`.

- [ ] **Step 5: Commit ConfigManager**

```bash
git add src/core/ConfigManager.h src/core/ConfigManager.cpp tests/core/test_config.cpp
git commit -m "feat: add config manager"
```

---

### Task 6: App Matcher

**Files:**
- Create: `src/core/AppMatcher.h`
- Create: `src/core/AppMatcher.cpp`
- Modify: `tests/CMakeLists.txt`
- Test: `tests/core/test_app_matcher.cpp`

- [ ] **Step 1: Register the matcher test target**

Append to `tests/CMakeLists.txt`:

```cmake
add_oopsjump_test(test_app_matcher core/test_app_matcher.cpp)
```

- [ ] **Step 2: Write failing matcher tests**

Create `tests/core/test_app_matcher.cpp`:

```cpp
#include <QtTest/QtTest>
#include "core/AppMatcher.h"

using namespace oopsjump;

class AppMatcherTest : public QObject {
    Q_OBJECT

private slots:
    void startupWmClassMatchesWindowClass()
    {
        AppInfo app;
        app.desktopId = "firefox.desktop";
        app.exec = "firefox";
        app.startupWmClass = "firefox";

        WindowInfo window;
        window.wmClass = "Firefox";
        window.title = "GitHub - Mozilla Firefox";

        const MatchResult result = AppMatcher::match(app, window, {});
        QVERIFY(result.matched);
        QVERIFY(result.totalScore >= 80);
    }

    void excludeRuleRejectsWindow()
    {
        AppInfo app;
        app.desktopId = "firefox.desktop";
        app.startupWmClass = "firefox";

        WindowInfo window;
        window.wmClass = "firefox";
        window.title = "Private Window";

        MatchRule rule;
        rule.type = "window_title";
        rule.op = "contains_ignore_case";
        rule.value = "Private";
        rule.effect = "exclude";

        const MatchResult result = AppMatcher::match(app, window, { rule });
        QVERIFY(!result.matched);
    }
};

QTEST_MAIN(AppMatcherTest)
#include "test_app_matcher.moc"
```

- [ ] **Step 3: Run the test and verify it fails**

Run:

```bash
cmake --build build --target test_app_matcher -j"$(nproc)"
```

Expected: FAIL because `AppMatcher` is not implemented.

- [ ] **Step 4: Implement AppMatcher**

Create `src/core/AppMatcher.h`:

```cpp
#pragma once

#include "core/AppInfo.h"
#include "core/Config.h"
#include "core/WindowInfo.h"
#include <QList>

namespace oopsjump {

struct MatchEvidence {
    QString source;
    QString expected;
    QString actual;
    int score = 0;
};

struct MatchResult {
    bool matched = false;
    int totalScore = 0;
    QList<MatchEvidence> evidence;
};

class AppMatcher {
public:
    static MatchResult match(const AppInfo& app, const WindowInfo& window, const QList<MatchRule>& rules);

private:
    static bool ruleMatches(const MatchRule& rule, const WindowInfo& window);
};

}
```

Create `src/core/AppMatcher.cpp`:

```cpp
#include "core/AppMatcher.h"

namespace oopsjump {

static bool equalsIgnoreCase(const QString& left, const QString& right)
{
    return left.compare(right, Qt::CaseInsensitive) == 0;
}

static bool containsIgnoreCase(const QString& haystack, const QString& needle)
{
    return haystack.contains(needle, Qt::CaseInsensitive);
}

bool AppMatcher::ruleMatches(const MatchRule& rule, const WindowInfo& window)
{
    QString actual;
    if (rule.type == "wm_class") {
        actual = window.wmClass;
    } else if (rule.type == "window_title") {
        actual = window.title;
    } else if (rule.type == "process_name") {
        actual = window.appId;
    } else {
        return false;
    }

    if (rule.op == "equals_ignore_case") {
        return equalsIgnoreCase(actual, rule.value);
    }
    if (rule.op == "contains_ignore_case") {
        return containsIgnoreCase(actual, rule.value);
    }
    if (rule.op == "equals") {
        return actual == rule.value;
    }
    if (rule.op == "contains") {
        return actual.contains(rule.value);
    }
    return false;
}

MatchResult AppMatcher::match(const AppInfo& app, const WindowInfo& window, const QList<MatchRule>& rules)
{
    MatchResult result;

    if (!app.startupWmClass.isEmpty() && equalsIgnoreCase(app.startupWmClass, window.wmClass)) {
        result.totalScore += 100;
        result.evidence.append({ "startup_wm_class", app.startupWmClass, window.wmClass, 100 });
    }

    const QString desktopBase = app.desktopId;
    if (!desktopBase.isEmpty() && containsIgnoreCase(desktopBase, window.wmClass)) {
        result.totalScore += 80;
        result.evidence.append({ "desktop_id", desktopBase, window.wmClass, 80 });
    }

    const QString execBase = app.exec.section('/', -1).section(' ', 0, 0);
    if (!execBase.isEmpty() && equalsIgnoreCase(execBase, window.appId)) {
        result.totalScore += 70;
        result.evidence.append({ "exec", execBase, window.appId, 70 });
    }

    for (const MatchRule& rule : rules) {
        if (!ruleMatches(rule, window)) {
            continue;
        }
        if (rule.effect == "exclude") {
            result.matched = false;
            result.totalScore = 0;
            result.evidence.append({ "exclude_rule", rule.value, window.title, -1000 });
            return result;
        }
        const int score = rule.weight == 0 ? 120 : rule.weight;
        result.totalScore += score;
        result.evidence.append({ "user_rule", rule.value, window.title, score });
    }

    result.matched = result.totalScore >= 80;
    return result;
}

}
```

- [ ] **Step 5: Run matcher tests**

Run:

```bash
cmake --build build --target test_app_matcher -j"$(nproc)"
ctest --test-dir build -R test_app_matcher --output-on-failure
```

Expected: PASS for `test_app_matcher`.

- [ ] **Step 6: Commit matcher**

```bash
git add src/core/AppMatcher.h src/core/AppMatcher.cpp tests/core/test_app_matcher.cpp
git commit -m "feat: add app window matcher"
```

---

### Task 7: X11 Connection and Window Backend

**Files:**
- Create: `src/backends/x11/X11Connection.h`
- Create: `src/backends/x11/X11Connection.cpp`
- Create: `src/backends/x11/X11WindowBackend.h`
- Create: `src/backends/x11/X11WindowBackend.cpp`
- Modify: `src/agent/main.cpp`
- Create: `docs/m0-x11-findings.md`

- [ ] **Step 1: Implement X11Connection**

Create `src/backends/x11/X11Connection.h`:

```cpp
#pragma once

#include "core/Result.h"
#include <QString>
#include <X11/Xlib.h>

namespace oopsjump {

class X11Connection {
public:
    X11Connection();
    ~X11Connection();

    X11Connection(const X11Connection&) = delete;
    X11Connection& operator=(const X11Connection&) = delete;

    VoidResult open();
    Display* display() const;
    Window rootWindow() const;
    Atom atom(const char* name) const;

private:
    Display* m_display = nullptr;
};

}
```

Create `src/backends/x11/X11Connection.cpp`:

```cpp
#include "backends/x11/X11Connection.h"

namespace oopsjump {

X11Connection::X11Connection() = default;

X11Connection::~X11Connection()
{
    if (m_display) {
        XCloseDisplay(m_display);
    }
}

VoidResult X11Connection::open()
{
    if (m_display) {
        return VoidResult::success();
    }
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        return VoidResult::failure("x11_display_unavailable", "Cannot open X11 display.");
    }
    return VoidResult::success();
}

Display* X11Connection::display() const
{
    return m_display;
}

Window X11Connection::rootWindow() const
{
    return DefaultRootWindow(m_display);
}

Atom X11Connection::atom(const char* name) const
{
    return XInternAtom(m_display, name, False);
}

}
```

- [ ] **Step 2: Implement window listing and activation**

Create `src/backends/x11/X11WindowBackend.h`:

```cpp
#pragma once

#include "core/Result.h"
#include "core/WindowInfo.h"
#include "backends/x11/X11Connection.h"
#include <QList>

namespace oopsjump {

class X11WindowBackend {
public:
    explicit X11WindowBackend(X11Connection& connection);

    Result<QList<WindowInfo>> listWindows() const;
    VoidResult activateWindow(WindowId id) const;

private:
    QString windowStringProperty(Window window, Atom atom) const;
    QStringList windowClass(Window window) const;
    int windowPid(Window window) const;
    bool isSkippable(Window window) const;

    X11Connection& m_connection;
};

}
```

Create `src/backends/x11/X11WindowBackend.cpp`:

```cpp
#include "backends/x11/X11WindowBackend.h"

#include <QByteArray>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>

namespace oopsjump {

X11WindowBackend::X11WindowBackend(X11Connection& connection)
    : m_connection(connection)
{
}

QString X11WindowBackend::windowStringProperty(Window window, Atom atom) const
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;

    const int status = XGetWindowProperty(
        m_connection.display(),
        window,
        atom,
        0,
        4096,
        False,
        AnyPropertyType,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &data);

    if (status != Success || !data) {
        return {};
    }

    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<int>(itemCount));
    XFree(data);
    return QString::fromUtf8(bytes).trimmed();
}

QStringList X11WindowBackend::windowClass(Window window) const
{
    XClassHint hint;
    if (!XGetClassHint(m_connection.display(), window, &hint)) {
        return {};
    }

    QStringList result;
    if (hint.res_name) {
        result << QString::fromLocal8Bit(hint.res_name);
        XFree(hint.res_name);
    }
    if (hint.res_class) {
        result << QString::fromLocal8Bit(hint.res_class);
        XFree(hint.res_class);
    }
    return result;
}

int X11WindowBackend::windowPid(Window window) const
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;

    const int status = XGetWindowProperty(
        m_connection.display(),
        window,
        m_connection.atom("_NET_WM_PID"),
        0,
        1,
        False,
        XA_CARDINAL,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &data);

    if (status != Success || !data || itemCount == 0) {
        if (data) {
            XFree(data);
        }
        return 0;
    }

    const int pid = *reinterpret_cast<long*>(data);
    XFree(data);
    return pid;
}

bool X11WindowBackend::isSkippable(Window window) const
{
    const QString type = windowStringProperty(window, m_connection.atom("_NET_WM_WINDOW_TYPE"));
    return type.contains("_NET_WM_WINDOW_TYPE_DOCK")
        || type.contains("_NET_WM_WINDOW_TYPE_DESKTOP")
        || type.contains("_NET_WM_WINDOW_TYPE_NOTIFICATION");
}

Result<QList<WindowInfo>> X11WindowBackend::listWindows() const
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;

    const Atom clientList = m_connection.atom("_NET_CLIENT_LIST_STACKING");
    const int status = XGetWindowProperty(
        m_connection.display(),
        m_connection.rootWindow(),
        clientList,
        0,
        4096,
        False,
        XA_WINDOW,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &data);

    if (status != Success || !data) {
        return Result<QList<WindowInfo>>::failure("window_backend_unavailable", "Cannot read X11 client list.");
    }

    QList<WindowInfo> windows;
    const Window* rawWindows = reinterpret_cast<const Window*>(data);
    for (unsigned long i = 0; i < itemCount; ++i) {
        const Window window = rawWindows[i];
        if (isSkippable(window)) {
            continue;
        }

        WindowInfo info;
        info.id = static_cast<WindowId>(window);
        info.title = windowStringProperty(window, m_connection.atom("_NET_WM_NAME"));
        const QStringList cls = windowClass(window);
        if (!cls.isEmpty()) {
            info.instanceName = cls.value(0);
            info.wmClass = cls.value(1, cls.value(0));
        }
        info.pid = windowPid(window);
        info.lastActiveOrder = static_cast<int>(i);
        windows.append(info);
    }

    XFree(data);
    return Result<QList<WindowInfo>>::success(windows);
}

VoidResult X11WindowBackend::activateWindow(WindowId id) const
{
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.xclient.type = ClientMessage;
    event.xclient.display = m_connection.display();
    event.xclient.window = static_cast<Window>(id);
    event.xclient.message_type = m_connection.atom("_NET_ACTIVE_WINDOW");
    event.xclient.format = 32;
    event.xclient.data.l[0] = 2;
    event.xclient.data.l[1] = CurrentTime;
    event.xclient.data.l[2] = 0;

    const int status = XSendEvent(
        m_connection.display(),
        m_connection.rootWindow(),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &event);
    XFlush(m_connection.display());

    if (status == 0) {
        return VoidResult::failure("window_activate_failed", "XSendEvent failed.");
    }
    return VoidResult::success();
}

}
```

- [ ] **Step 3: Add `--list-windows` CLI wiring**

Modify `src/agent/main.cpp` so after `parser.process(app);` it handles `--list-windows`:

```cpp
    if (parser.isSet("list-windows")) {
        X11Connection connection;
        const auto opened = connection.open();
        if (!opened.ok) {
            QTextStream(stderr) << opened.errorCode << ": " << opened.message << "\n";
            return 2;
        }

        X11WindowBackend windows(connection);
        const auto listed = windows.listWindows();
        if (!listed.ok) {
            QTextStream(stderr) << listed.errorCode << ": " << listed.message << "\n";
            return 2;
        }

        for (const WindowInfo& window : listed.value) {
            out << QString::number(window.id)
                << "\t" << window.wmClass
                << "\t" << window.title
                << "\n";
        }
        return 0;
    }
```

Add includes to `src/agent/main.cpp`:

```cpp
#include "backends/x11/X11Connection.h"
#include "backends/x11/X11WindowBackend.h"
#include "core/WindowInfo.h"
```

- [ ] **Step 4: Build and manually list windows**

Run:

```bash
cmake --build build --target oops-jump-agent -j"$(nproc)"
./build/oops-jump-agent --list-windows
```

Expected on X11: prints one line per normal window with `window id`, `WM_CLASS`, and title.

Expected outside X11: exits with code `2` and prints `x11_display_unavailable`.

- [ ] **Step 5: Document M0 window findings**

Create `docs/m0-x11-findings.md`:

```markdown
# M0 X11 Findings

## Environment

- `XDG_SESSION_TYPE`: record from the test machine.
- `DISPLAY`: record from the test machine.
- deepin version: record from the test machine.

## Window Listing

- `./build/oops-jump-agent --list-windows` result: record whether normal windows appear.
- Firefox `WM_CLASS`: record actual value.
- Deepin Terminal `WM_CLASS`: record actual value.
- VS Code `WM_CLASS`: record actual value.

## Window Activation

- `_NET_ACTIVE_WINDOW` activation result: record whether focus changes.
- Minimized window behavior: record whether activation restores the window.
- Cross-workspace behavior: record whether activation switches workspace.

## Limits Observed

- Record concrete X11 or deepin behavior that affects implementation.
```

- [ ] **Step 6: Commit X11 window backend**

```bash
git add src/backends/x11 src/agent/main.cpp docs/m0-x11-findings.md
git commit -m "feat: add x11 window backend"
```

---

### Task 8: X11 Hotkey Backend

**Files:**
- Create: `src/backends/x11/X11HotkeyBackend.h`
- Create: `src/backends/x11/X11HotkeyBackend.cpp`
- Modify: `src/agent/main.cpp`

- [ ] **Step 1: Implement hotkey backend**

Create `src/backends/x11/X11HotkeyBackend.h`:

```cpp
#pragma once

#include "core/Hotkey.h"
#include "core/Result.h"
#include "backends/x11/X11Connection.h"
#include <QHash>

namespace oopsjump {

class X11HotkeyBackend {
public:
    explicit X11HotkeyBackend(X11Connection& connection);

    VoidResult registerHotkey(const Hotkey& hotkey, const QString& actionId);
    void unregisterAll();
    QString pollTriggeredAction();

private:
    unsigned int modifierMask(const QStringList& modifiers) const;
    KeySym keySym(const QString& key) const;
    QList<unsigned int> lockVariants(unsigned int base) const;

    X11Connection& m_connection;
    QHash<QString, QString> m_keyToAction;
};

}
```

Create `src/backends/x11/X11HotkeyBackend.cpp`:

```cpp
#include "backends/x11/X11HotkeyBackend.h"

#include <X11/keysym.h>

namespace oopsjump {

X11HotkeyBackend::X11HotkeyBackend(X11Connection& connection)
    : m_connection(connection)
{
}

unsigned int X11HotkeyBackend::modifierMask(const QStringList& modifiers) const
{
    unsigned int mask = 0;
    if (modifiers.contains("Ctrl")) {
        mask |= ControlMask;
    }
    if (modifiers.contains("Alt")) {
        mask |= Mod1Mask;
    }
    if (modifiers.contains("Shift")) {
        mask |= ShiftMask;
    }
    if (modifiers.contains("Meta")) {
        mask |= Mod4Mask;
    }
    return mask;
}

KeySym X11HotkeyBackend::keySym(const QString& key) const
{
    if (key == "Enter") {
        return XK_Return;
    }
    if (key == "Esc") {
        return XK_Escape;
    }
    if (key == "Space") {
        return XK_space;
    }
    if (key.size() == 1) {
        return XStringToKeysym(key.toLower().toLocal8Bit().constData());
    }
    return XStringToKeysym(key.toLocal8Bit().constData());
}

QList<unsigned int> X11HotkeyBackend::lockVariants(unsigned int base) const
{
    constexpr unsigned int numLock = Mod2Mask;
    constexpr unsigned int capsLock = LockMask;
    constexpr unsigned int scrollLock = Mod5Mask;
    return {
        base,
        base | numLock,
        base | capsLock,
        base | scrollLock,
        base | numLock | capsLock,
        base | numLock | scrollLock,
        base | capsLock | scrollLock,
        base | numLock | capsLock | scrollLock
    };
}

VoidResult X11HotkeyBackend::registerHotkey(const Hotkey& hotkey, const QString& actionId)
{
    const KeySym sym = keySym(hotkey.key);
    if (sym == NoSymbol) {
        return VoidResult::failure("hotkey_invalid", "Cannot map hotkey key to X11 keysym.");
    }

    const KeyCode keycode = XKeysymToKeycode(m_connection.display(), sym);
    if (keycode == 0) {
        return VoidResult::failure("hotkey_invalid", "Cannot map X11 keysym to keycode.");
    }

    const unsigned int baseMask = modifierMask(hotkey.modifiers);
    for (const unsigned int mask : lockVariants(baseMask)) {
        XGrabKey(
            m_connection.display(),
            keycode,
            mask,
            m_connection.rootWindow(),
            True,
            GrabModeAsync,
            GrabModeAsync);
    }
    XFlush(m_connection.display());

    m_keyToAction.insert(QString("%1:%2").arg(static_cast<int>(keycode)).arg(baseMask), actionId);
    return VoidResult::success();
}

void X11HotkeyBackend::unregisterAll()
{
    XUngrabKey(m_connection.display(), AnyKey, AnyModifier, m_connection.rootWindow());
    XFlush(m_connection.display());
    m_keyToAction.clear();
}

QString X11HotkeyBackend::pollTriggeredAction()
{
    while (XPending(m_connection.display()) > 0) {
        XEvent event;
        XNextEvent(m_connection.display(), &event);
        if (event.type != KeyPress) {
            continue;
        }

        const unsigned int cleanState = event.xkey.state & ~(Mod2Mask | LockMask | Mod5Mask);
        const QString key = QString("%1:%2").arg(event.xkey.keycode).arg(cleanState);
        if (m_keyToAction.contains(key)) {
            return m_keyToAction.value(key);
        }
    }
    return {};
}

}
```

- [ ] **Step 2: Add `--trigger` and a manual hotkey loop**

Modify `src/agent/main.cpp`:

```cpp
    if (parser.isSet("trigger")) {
        out << "trigger requested: " << parser.value("trigger") << "\n";
        return 0;
    }
```

Add a temporary manual hotkey mode for M0:

```cpp
    if (!parser.isSet("validate-config")
        && !parser.isSet("list-bindings")
        && !parser.isSet("list-apps")
        && !parser.isSet("list-windows")
        && !parser.isSet("trigger")) {
        X11Connection connection;
        const auto opened = connection.open();
        if (!opened.ok) {
            QTextStream(stderr) << opened.errorCode << ": " << opened.message << "\n";
            return 2;
        }

        const auto parsed = Hotkey::parse("Alt+1");
        if (!parsed.ok) {
            QTextStream(stderr) << parsed.errorCode << ": " << parsed.message << "\n";
            return 2;
        }

        X11HotkeyBackend hotkeys(connection);
        const auto registered = hotkeys.registerHotkey(parsed.value, "app.firefox");
        if (!registered.ok) {
            QTextStream(stderr) << registered.errorCode << ": " << registered.message << "\n";
            return 2;
        }

        out << "registered Alt+1 for app.firefox; press Ctrl+C to exit\n";
        while (true) {
            const QString action = hotkeys.pollTriggeredAction();
            if (!action.isEmpty()) {
                out << "triggered " << action << "\n";
                out.flush();
            }
            QThread::msleep(20);
        }
    }
```

Add includes:

```cpp
#include "backends/x11/X11HotkeyBackend.h"
#include "core/Hotkey.h"
#include <QThread>
```

- [ ] **Step 3: Build and manually verify hotkey capture**

Run:

```bash
cmake --build build --target oops-jump-agent -j"$(nproc)"
./build/oops-jump-agent
```

Expected on X11: terminal prints `registered Alt+1 for app.firefox`; pressing `Alt+1` prints `triggered app.firefox`.

Stop with `Ctrl+C`.

- [ ] **Step 4: Record hotkey findings**

Append to `docs/m0-x11-findings.md`:

```markdown
## Hotkey Registration

- `Alt+1` registration: record success or failure.
- NumLock on: record whether `Alt+1` still triggers.
- CapsLock on: record whether `Alt+1` still triggers.
- Conflict behavior: record what happens if the desktop already owns the shortcut.
```

- [ ] **Step 5: Commit X11 hotkey backend**

```bash
git add src/backends/x11/X11HotkeyBackend.h src/backends/x11/X11HotkeyBackend.cpp src/agent/main.cpp docs/m0-x11-findings.md
git commit -m "feat: add x11 hotkey backend"
```

---

### Task 9: Launcher and Action Engine

**Files:**
- Create: `src/core/Launcher.h`
- Create: `src/core/Launcher.cpp`
- Create: `src/core/ActionEngine.h`
- Create: `src/core/ActionEngine.cpp`
- Modify: `src/agent/main.cpp`
- Test: add action decision coverage to `tests/core/test_config.cpp` or create `tests/core/test_action_engine.cpp` and update `tests/CMakeLists.txt`

- [ ] **Step 1: Add `test_action_engine` to CMake**

Modify `tests/CMakeLists.txt`:

```cmake
add_oopsjump_test(test_action_engine core/test_action_engine.cpp)
```

- [ ] **Step 2: Write failing action engine tests**

Create `tests/core/test_action_engine.cpp`:

```cpp
#include <QtTest/QtTest>
#include "core/ActionEngine.h"

using namespace oopsjump;

class ActionEngineTest : public QObject {
    Q_OBJECT

private slots:
    void noWindowsLaunchesWhenAllowed()
    {
        Binding binding;
        binding.id = "app.firefox";
        binding.desktopId = "firefox.desktop";
        binding.launchIfNotRunning = true;

        AppInfo app;
        app.desktopId = "firefox.desktop";
        app.exec = "firefox";

        const ActionDecision decision = ActionEngine::decide(binding, app, {});
        QCOMPARE(decision.type, ActionType::Launch);
    }

    void oneWindowFocuses()
    {
        Binding binding;
        binding.id = "app.firefox";
        binding.desktopId = "firefox.desktop";

        AppInfo app;
        app.desktopId = "firefox.desktop";

        WindowInfo window;
        window.id = 42;

        const ActionDecision decision = ActionEngine::decide(binding, app, { window });
        QCOMPARE(decision.type, ActionType::Focus);
        QCOMPARE(decision.windowId, static_cast<WindowId>(42));
    }
};

QTEST_MAIN(ActionEngineTest)
#include "test_action_engine.moc"
```

- [ ] **Step 3: Run the test and verify it fails**

Run:

```bash
cmake --build build --target test_action_engine -j"$(nproc)"
```

Expected: FAIL because `ActionEngine` is not implemented.

- [ ] **Step 4: Implement ActionEngine decisions**

Create `src/core/ActionEngine.h`:

```cpp
#pragma once

#include "core/AppInfo.h"
#include "core/Config.h"
#include "core/WindowInfo.h"
#include <QList>

namespace oopsjump {

enum class ActionType {
    Launch,
    Focus,
    Cycle,
    Ignore,
    Fail
};

struct ActionDecision {
    ActionType type = ActionType::Ignore;
    WindowId windowId = 0;
    QString errorCode;
    QString message;
};

class ActionEngine {
public:
    static ActionDecision decide(const Binding& binding, const AppInfo& app, const QList<WindowInfo>& windows);
};

}
```

Create `src/core/ActionEngine.cpp`:

```cpp
#include "core/ActionEngine.h"

namespace oopsjump {

ActionDecision ActionEngine::decide(const Binding& binding, const AppInfo& app, const QList<WindowInfo>& windows)
{
    if (!binding.enabled) {
        return { ActionType::Ignore, 0, "binding_disabled", "Binding is disabled." };
    }
    if (app.desktopId.isEmpty() && binding.command.isEmpty()) {
        return { ActionType::Fail, 0, "app_not_found", "Binding has no application target." };
    }
    if (windows.isEmpty()) {
        if (binding.launchIfNotRunning) {
            return { ActionType::Launch, 0, {}, {} };
        }
        return { ActionType::Fail, 0, "window_not_found", "No matching window found." };
    }
    if (windows.size() == 1) {
        return { ActionType::Focus, windows.first().id, {}, {} };
    }
    return { ActionType::Cycle, windows.first().id, {}, {} };
}

}
```

- [ ] **Step 5: Implement Launcher**

Create `src/core/Launcher.h`:

```cpp
#pragma once

#include "core/AppInfo.h"
#include "core/Result.h"

namespace oopsjump {

class Launcher {
public:
    static VoidResult launch(const AppInfo& app);
};

}
```

Create `src/core/Launcher.cpp`:

```cpp
#include "core/Launcher.h"

#include <QProcess>

namespace oopsjump {

VoidResult Launcher::launch(const AppInfo& app)
{
    if (app.exec.trimmed().isEmpty()) {
        return VoidResult::failure("launch_failed", "Application Exec is empty.");
    }

    const QString program = app.exec.section(' ', 0, 0);
    const QString argsString = app.exec.section(' ', 1);
    const QStringList args = argsString.isEmpty()
        ? QStringList {}
        : QProcess::splitCommand(argsString);

    if (!QProcess::startDetached(program, args)) {
        return VoidResult::failure("launch_failed", "Failed to start process.");
    }
    return VoidResult::success();
}

}
```

- [ ] **Step 6: Run action engine tests**

Run:

```bash
cmake --build build --target test_action_engine -j"$(nproc)"
ctest --test-dir build -R test_action_engine --output-on-failure
```

Expected: PASS for `test_action_engine`.

- [ ] **Step 7: Commit launcher and action engine**

```bash
git add src/core/ActionEngine.h src/core/ActionEngine.cpp src/core/Launcher.h src/core/Launcher.cpp tests
git commit -m "feat: add action decisions and launcher"
```

---

### Task 10: Command-Line Agent Integration

**Files:**
- Modify: `src/agent/main.cpp`

- [ ] **Step 1: Wire `--validate-config`**

Modify `src/agent/main.cpp` so `--validate-config`:

```cpp
    const QString configPath = parser.value("config").isEmpty()
        ? QDir::homePath() + "/.config/oops-jump/config.json"
        : parser.value("config");

    if (parser.isSet("validate-config")) {
        ConfigManager manager(configPath);
        const auto loaded = manager.load();
        if (!loaded.ok) {
            QTextStream(stderr) << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }
        out << "config valid\n";
        return 0;
    }
```

Add includes:

```cpp
#include "core/ConfigManager.h"
#include <QDir>
```

- [ ] **Step 2: Wire `--list-bindings`**

Add this branch:

```cpp
    if (parser.isSet("list-bindings")) {
        ConfigManager manager(configPath);
        const auto loaded = manager.load();
        if (!loaded.ok) {
            QTextStream(stderr) << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }
        for (const Binding& binding : loaded.value.bindings) {
            out << binding.id << "\t" << binding.hotkey << "\t" << binding.desktopId << "\n";
        }
        return 0;
    }
```

- [ ] **Step 3: Wire `--list-apps`**

Add this branch:

```cpp
    if (parser.isSet("list-apps")) {
        AppRegistry registry;
        const auto scanned = registry.scan();
        if (!scanned.ok) {
            QTextStream(stderr) << scanned.errorCode << ": " << scanned.message << "\n";
            return 2;
        }
        for (const AppInfo& appInfo : registry.listApplications()) {
            out << appInfo.desktopId << "\t" << appInfo.localizedName << "\t" << appInfo.exec << "\n";
        }
        return 0;
    }
```

Add includes:

```cpp
#include "core/AppRegistry.h"
#include "core/AppInfo.h"
```

- [ ] **Step 4: Wire `--trigger` to launch or focus**

Replace the temporary trigger branch with:

```cpp
    if (parser.isSet("trigger")) {
        ConfigManager manager(configPath);
        const auto loaded = manager.load();
        if (!loaded.ok) {
            QTextStream(stderr) << loaded.errorCode << ": " << loaded.message << "\n";
            return 2;
        }

        const QString actionId = parser.value("trigger");
        std::optional<Binding> binding;
        for (const Binding& candidate : loaded.value.bindings) {
            if (candidate.id == actionId) {
                binding = candidate;
                break;
            }
        }
        if (!binding.has_value()) {
            QTextStream(stderr) << "app_not_found: binding not found\n";
            return 2;
        }

        AppRegistry registry;
        registry.scan();
        const auto appInfo = registry.findByDesktopId(binding->desktopId);
        if (!appInfo.has_value()) {
            QTextStream(stderr) << "app_not_found: desktop id not found\n";
            return 2;
        }

        X11Connection connection;
        const auto opened = connection.open();
        if (!opened.ok) {
            QTextStream(stderr) << opened.errorCode << ": " << opened.message << "\n";
            return 2;
        }

        X11WindowBackend windows(connection);
        const auto listed = windows.listWindows();
        if (!listed.ok) {
            QTextStream(stderr) << listed.errorCode << ": " << listed.message << "\n";
            return 2;
        }

        QList<WindowInfo> matches;
        for (const WindowInfo& window : listed.value) {
            const MatchResult match = AppMatcher::match(appInfo.value(), window, binding->matchRules);
            if (match.matched) {
                matches.append(window);
            }
        }

        const ActionDecision decision = ActionEngine::decide(binding.value(), appInfo.value(), matches);
        if (decision.type == ActionType::Launch) {
            const auto launched = Launcher::launch(appInfo.value());
            if (!launched.ok) {
                QTextStream(stderr) << launched.errorCode << ": " << launched.message << "\n";
                return 2;
            }
            out << "launched " << appInfo->desktopId << "\n";
            return 0;
        }
        if (decision.type == ActionType::Focus || decision.type == ActionType::Cycle) {
            const auto activated = windows.activateWindow(decision.windowId);
            if (!activated.ok) {
                QTextStream(stderr) << activated.errorCode << ": " << activated.message << "\n";
                return 2;
            }
            out << "activated " << decision.windowId << "\n";
            return 0;
        }

        QTextStream(stderr) << decision.errorCode << ": " << decision.message << "\n";
        return 2;
    }
```

Add includes:

```cpp
#include "core/ActionEngine.h"
#include "core/AppMatcher.h"
#include "core/Launcher.h"
#include <optional>
```

- [ ] **Step 5: Build the agent**

Run:

```bash
cmake --build build --target oops-jump-agent -j"$(nproc)"
```

Expected: build succeeds.

- [ ] **Step 6: Create a manual config and validate it**

Create `/tmp/oops-jump-config.json`:

```json
{
  "version": 1,
  "general": {
    "enabled": true,
    "autostart": false,
    "session_backend": "auto",
    "show_overlay": true,
    "log_level": "info"
  },
  "hotkey": {
    "mode": "direct",
    "leader_key": "Alt+Space",
    "leader_timeout_ms": 1500
  },
  "window": {
    "default_multi_window_strategy": "cycle",
    "cycle_timeout_ms": 1200,
    "launch_timeout_ms": 8000,
    "include_all_workspaces": true,
    "switch_workspace_when_needed": true
  },
  "bindings": [
    {
      "id": "app.firefox",
      "enabled": true,
      "hotkey": "Alt+1",
      "desktop_id": "firefox.desktop",
      "multi_window_strategy": "cycle"
    }
  ]
}
```

Run:

```bash
./build/oops-jump-agent --config /tmp/oops-jump-config.json --validate-config
./build/oops-jump-agent --config /tmp/oops-jump-config.json --list-bindings
./build/oops-jump-agent --list-apps | head
```

Expected:

```text
config valid
app.firefox    Alt+1    firefox.desktop
```

`--list-apps` should print installed desktop apps.

- [ ] **Step 7: Manually trigger Firefox on X11**

Run:

```bash
./build/oops-jump-agent --config /tmp/oops-jump-config.json --trigger app.firefox
```

Expected:

- If Firefox has no matching window, it prints `launched firefox.desktop`.
- If Firefox has a matching window, it prints `activated <window-id>`.

- [ ] **Step 8: Commit CLI integration**

```bash
git add src/agent/main.cpp
git commit -m "feat: wire command-line agent flow"
```

---

### Task 11: Final M0-M1 Verification

**Files:**
- Modify: `docs/m0-x11-findings.md`
- Modify: `deepin-manico-like-design.md` only if actual findings change design assumptions

- [ ] **Step 1: Run all unit tests**

Run:

```bash
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 2: Verify CLI commands**

Run:

```bash
./build/oops-jump-agent --help
./build/oops-jump-agent --config /tmp/oops-jump-config.json --validate-config
./build/oops-jump-agent --config /tmp/oops-jump-config.json --list-bindings
./build/oops-jump-agent --list-apps
./build/oops-jump-agent --list-windows
```

Expected:

- `--help` prints options.
- `--validate-config` prints `config valid`.
- `--list-bindings` prints configured bindings.
- `--list-apps` prints installed apps.
- `--list-windows` prints X11 windows on an X11 session or a clear X11 unavailable error outside X11.

- [ ] **Step 3: Verify M0 acceptance**

On deepin v25 X11, run:

```bash
./build/oops-jump-agent --config /tmp/oops-jump-config.json --trigger app.firefox
```

Expected:

- Firefox starts if it is not running.
- Firefox is activated if it has an existing matched window.

- [ ] **Step 4: Fill findings with real values**

Update `docs/m0-x11-findings.md` with:

```text
XDG_SESSION_TYPE
DISPLAY
Firefox WM_CLASS
Deepin Terminal WM_CLASS
VS Code WM_CLASS
Alt+1 registration result
Activation behavior for normal, minimized, and cross-workspace windows
```

- [ ] **Step 5: Commit verification findings**

```bash
git add docs/m0-x11-findings.md deepin-manico-like-design.md
git commit -m "docs: record m0 x11 findings"
```

If `deepin-manico-like-design.md` was not changed, run:

```bash
git add docs/m0-x11-findings.md
git commit -m "docs: record m0 x11 findings"
```

---

## Self-Review Checklist

- [ ] M0.1-M0.10 are covered by Tasks 1, 7, 8, 10, and 11.
- [ ] M1.1-M1.12 are covered by Tasks 1 through 11.
- [ ] Pure logic has tests before implementation.
- [ ] X11-specific behavior has manual verification commands.
- [ ] The plan creates working command-line software before any UI work.
- [ ] The plan avoids Settings UI, D-Bus, packaging, and Wayland scope.
- [ ] Each task has file paths, commands, and expected results.
