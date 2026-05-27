# DeepSwitch M2-M4 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 M0/M1 已完成的命令行原型基础上，交付可视化设置、日常使用质量、安装打包，以及 Wayland/Treeland 能力边界验证。

**Architecture:** 后续版本应先把 `deepswitch-agent` 从 CLI 主循环拆成可复用的服务对象，再通过 D-Bus 暴露状态、配置和触发能力；Settings UI 只作为 D-Bus 客户端，不直接改写 agent 正在使用的配置。M3 在 M2 的 IPC 和状态模型上增加 autostart、overlay、日志轮转、打包；M4 不承诺完整 Wayland 窗口控制，先做能力探测和有限降级后端。

**Tech Stack:** C++20, CMake, Qt 6 Core/Gui/Test/DBus/Qml/Quick/QuickControls2, X11, Qt Test, Debian packaging.

---

## Current Baseline

当前仓库已有：

- `deepswitch_core`：配置、`.desktop` 解析、应用匹配、动作决策、启动逻辑。
- `deepswitch_x11`：X11 连接、全局快捷键、窗口枚举和激活。
- `deepswitch-agent`：CLI 命令和默认 hotkey loop。
- `tests/core`：核心逻辑单元测试。

当前缺口：

- agent 缺少可被 D-Bus 和测试复用的运行时控制器。
- `ConfigManager::validate()` 仍偏基础，缺少 UI 需要的结构化 issue 列表。
- 没有 IPC target、Settings target、QML 资源、overlay target、安装规则和包元数据。
- X11 后端能力状态没有统一结构，Settings 无法展示“当前可用/降级/冲突”。

## Version Strategy

### M2: Settings + IPC MVP

目标：普通用户无需手写 `config.json`，可以通过设置界面完成 Firefox、Terminal、VS Code 等应用绑定，并让 agent 热更新生效。

范围：

- 新增 D-Bus service/client。
- 拆出 `AgentController` 管理配置、注册快捷键、应用扫描、动作触发和状态聚合。
- 新增 `deepswitch-settings` Qt/QML 应用。
- 支持绑定列表、应用选择、快捷键录制、多窗口策略、后端状态、保存配置、reload。

不做：

- 托盘入口、安装包、overlay 动效。
- Wayland 后端实现。
- 复杂导入导出、主题编辑、云同步。

验收标准：

- `deepswitch-agent` 启动后注册 `org.deepin.DeepSwitch` session bus name。
- `deepswitch-settings` 能列出 bindings 和 applications。
- UI 能新增、编辑、禁用、删除绑定。
- UI 保存后 agent 在 500ms 内 reload 或局部更新快捷键。
- 快捷键冲突、无效 hotkey、应用不存在能在 UI 中显示明确状态。
- `ctest --test-dir build` 通过。

### M3: Daily-Use Polish + Packaging

目标：达到日常使用质量，安装后开机可用，设置界面关闭后快捷键仍工作。

范围：

- 用户级 autostart 开关。
- DDE 可见入口或托盘入口。
- Overlay 轻提示。
- 更完整的窗口追踪、匹配诊断、日志轮转。
- `.deb` packaging、desktop 文件、图标、安装/卸载行为。
- 手动测试矩阵。

不做：

- Wayland/Treeland 完整窗口激活。
- 插件系统。
- 系统级服务或 root 权限安装逻辑。

验收标准：

- 安装 deb 后 `deepswitch-agent` 可在登录后自动启动。
- `deepswitch-settings` 可从桌面启动器打开。
- 触发快捷键时已有窗口聚焦通常 < 200ms，动作决策通常 < 50ms。
- 日志文件超过限制后轮转，不无限增长。
- 卸载不删除 `~/.config/deepswitch/config.json`。

### M4: Wayland/Treeland Research + Limited Backend

目标：明确 deepin v25 Wayland/Treeland 的能力边界，并交付有限降级路径。

范围：

- 会话类型和能力检测。
- 验证 `xdg-desktop-portal` GlobalShortcuts 可用性。
- 调研 DDE/Treeland 是否有窗口列表/激活接口。
- 新增 limited backend：至少支持启动应用和能力状态展示；窗口枚举/激活按实际接口决定。
- Settings UI 显示 X11/Wayland/Treeland 能力差异。
- 输出 `docs/wayland-treeland-findings.md`。

不做：

- 伪造不可用能力。
- 依赖不稳定私有接口作为默认路径。
- 在 Wayland 下承诺任意窗口激活。

验收标准：

- X11 行为不回退。
- Wayland/Treeland 下 agent 不崩溃，Settings 明确显示可用和不可用能力。
- 如果 portal global shortcuts 可用，则可触发启动应用。
- 如果窗口控制不可用，则 UI 明确降级为“启动应用，不支持聚焦/循环”。

---

## Target File Structure

### M2 Files

- Modify: `CMakeLists.txt`，新增 Qt DBus/QML/Quick 依赖和 targets。
- Modify: `src/agent/main.cpp`，仅保留 CLI 解析和 controller wiring。
- Create: `src/agent/AgentController.h`
- Create: `src/agent/AgentController.cpp`
- Create: `src/ipc/AgentTypes.h`
- Create: `src/ipc/AgentTypes.cpp`
- Create: `src/ipc/AgentDBusService.h`
- Create: `src/ipc/AgentDBusService.cpp`
- Create: `src/ipc/AgentDBusClient.h`
- Create: `src/ipc/AgentDBusClient.cpp`
- Create: `src/settings/main.cpp`
- Create: `src/settings/SettingsController.h`
- Create: `src/settings/SettingsController.cpp`
- Create: `src/settings/qml/Main.qml`
- Create: `src/settings/qml/BindingsPage.qml`
- Create: `src/settings/qml/ApplicationPicker.qml`
- Create: `src/settings/qml/HotkeyRecorder.qml`
- Create: `src/settings/qml/BackendStatusPage.qml`
- Create: `src/settings/qml/Components.qml`
- Create: `src/settings/qml/qml.qrc`
- Create: `tests/ipc/test_agent_types.cpp`
- Create: `tests/agent/test_agent_controller.cpp`

### M3 Files

- Create: `src/core/AutostartManager.h`
- Create: `src/core/AutostartManager.cpp`
- Create: `src/core/LogFileManager.h`
- Create: `src/core/LogFileManager.cpp`
- Create: `src/overlay/main.cpp`
- Create: `src/overlay/OverlayWindow.h`
- Create: `src/overlay/OverlayWindow.cpp`
- Create: `src/overlay/qml/Overlay.qml`
- Create: `packaging/debian/control`
- Create: `packaging/debian/rules`
- Create: `packaging/debian/changelog`
- Create: `packaging/debian/copyright`
- Create: `packaging/debian/deepswitch.install`
- Create: `packaging/linux/org.deepin.DeepSwitch.desktop`
- Create: `packaging/linux/deepswitch-agent.desktop`
- Create: `packaging/linux/icons/hicolor/scalable/apps/deepswitch.svg`
- Create: `docs/manual-test-matrix.md`
- Create: `tests/core/test_autostart_manager.cpp`
- Create: `tests/core/test_log_file_manager.cpp`

### M4 Files

- Create: `src/backends/BackendStatus.h`
- Create: `src/backends/SessionDetector.h`
- Create: `src/backends/SessionDetector.cpp`
- Create: `src/backends/wayland/LimitedWaylandHotkeyBackend.h`
- Create: `src/backends/wayland/LimitedWaylandHotkeyBackend.cpp`
- Create: `src/backends/wayland/LimitedWaylandWindowBackend.h`
- Create: `src/backends/wayland/LimitedWaylandWindowBackend.cpp`
- Create: `src/backends/portal/PortalGlobalShortcutsProbe.h`
- Create: `src/backends/portal/PortalGlobalShortcutsProbe.cpp`
- Create: `docs/wayland-treeland-findings.md`
- Create: `tests/backends/test_session_detector.cpp`

---

## M2 Detailed Plan

### Task M2.1: Add Structured Runtime Types

**Files:**

- Create: `src/ipc/AgentTypes.h`
- Create: `src/ipc/AgentTypes.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/ipc/test_agent_types.cpp`

- [ ] Define `BindingDto`, `AppInfoDto`, `WindowInfoDto`, `BackendStatusDto`, `AgentStatusDto`, `ConfigIssueDto`.
- [ ] Implement conversion between core structs and `QVariantMap`/`QVariantList`.
- [ ] Add tests that round-trip a binding containing `id`, `enabled`, `hotkey`, `desktop_id`, `multi_window_strategy`, `launch_if_not_running`, `focus_existing_window`.
- [ ] Run `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.
- [ ] Run `cmake --build build`.
- [ ] Run `ctest --test-dir build -R test_agent_types --output-on-failure`.

Stable D-Bus map keys:

```text
id
enabled
hotkey
selection_key
desktop_id
command
multi_window_strategy
launch_if_not_running
focus_existing_window
match_rules
```

### Task M2.2: Split AgentController From main.cpp

**Files:**

- Create: `src/agent/AgentController.h`
- Create: `src/agent/AgentController.cpp`
- Modify: `src/agent/main.cpp`
- Test: `tests/agent/test_agent_controller.cpp`

- [ ] Move config loading, app registry scanning, hotkey registration, action trigger, and status aggregation out of `main.cpp`.
- [ ] Keep CLI commands in `main.cpp`, but call controller methods instead of duplicating action logic.
- [ ] Add `AgentController::reloadConfig()`, `pause()`, `resume()`, `triggerAction(QString)`, `listBindings()`, `listApplications()`, `listWindows(QString)`, `status()`.
- [ ] Add tests using fake hotkey/window adapters where possible; X11 integration remains manual.
- [ ] Run `ctest --test-dir build -R test_agent_controller --output-on-failure`.

Controller status states:

```text
starting
running
paused
degraded
error
```

### Task M2.3: Implement Agent D-Bus Service

**Files:**

- Create: `src/ipc/AgentDBusService.h`
- Create: `src/ipc/AgentDBusService.cpp`
- Modify: `src/agent/main.cpp`
- Modify: `CMakeLists.txt`

- [ ] Add `find_package(Qt6 REQUIRED COMPONENTS Core Test Gui DBus)`.
- [ ] Register service name `org.deepin.DeepSwitch`.
- [ ] Register object path `/org/deepin/DeepSwitch`.
- [ ] Expose interface methods using Qt slots:
  `GetStatus`, `ReloadConfig`, `Pause`, `Resume`, `ListBindings`, `SetBinding`, `RemoveBinding`, `TestHotkey`, `ListApplications`, `ListWindows`, `ActivateWindow`, `LaunchApp`.
- [ ] Emit `StatusChanged`, `HotkeyTriggered`, `BindingChanged`, `BackendChanged`, `WindowListChanged`, `ErrorOccurred`.
- [ ] On D-Bus registration failure, log fatal and exit non-zero in daemon mode.
- [ ] Keep CLI one-shot commands functional without requiring D-Bus service registration.

D-Bus contract:

```text
Service: org.deepin.DeepSwitch
Object: /org/deepin/DeepSwitch
Interface: org.deepin.DeepSwitch.Agent
```

### Task M2.4: Add Settings D-Bus Client

**Files:**

- Create: `src/ipc/AgentDBusClient.h`
- Create: `src/ipc/AgentDBusClient.cpp`
- Create: `src/settings/SettingsController.h`
- Create: `src/settings/SettingsController.cpp`

- [ ] Wrap all D-Bus calls behind `AgentDBusClient`.
- [ ] Convert `QDBusPendingReply` results into QML-friendly properties and signals.
- [ ] Expose `SettingsController` to QML with properties:
  `connected`, `status`, `bindings`, `applications`, `backendStatus`, `lastError`.
- [ ] Add methods:
  `refresh()`, `saveBinding(map)`, `removeBinding(id)`, `testHotkey(hotkey, excludeId)`, `launchApp(desktopId)`.
- [ ] If agent is unavailable, show connection error and keep UI read-only except retry.

### Task M2.5: Build Settings UI Shell

**Files:**

- Create: `src/settings/main.cpp`
- Create: `src/settings/qml/Main.qml`
- Create: `src/settings/qml/qml.qrc`
- Modify: `CMakeLists.txt`

- [ ] Add executable `deepswitch-settings`.
- [ ] Link `Qt6::Core`, `Qt6::Gui`, `Qt6::Qml`, `Qt6::Quick`, `Qt6::QuickControls2`, `Qt6::DBus`.
- [ ] Load `qrc:/qml/Main.qml`.
- [ ] Add pages: bindings, applications, backend status, about.
- [ ] Start with a narrow but usable 900x640 window.
- [ ] Show agent connection state in the header.

### Task M2.6: Binding List and Editor

**Files:**

- Create: `src/settings/qml/BindingsPage.qml`
- Modify: `src/settings/SettingsController.cpp`

- [ ] Render one row per binding: enabled, hotkey, app name, strategy, status, edit/delete.
- [ ] Add binding editor dialog with fields:
  enabled, hotkey, desktop_id, strategy, launch_if_not_running, focus_existing_window.
- [ ] On save, call `SettingsController::saveBinding()`.
- [ ] On delete, call `SettingsController::removeBinding()`.
- [ ] Disable save when `id`, `hotkey`, or `desktop_id` is invalid.
- [ ] Display error codes without losing the user's unsaved edits.

### Task M2.7: Application Picker and Search

**Files:**

- Create: `src/settings/qml/ApplicationPicker.qml`
- Modify: `src/settings/SettingsController.cpp`

- [ ] Load applications through `ListApplications`.
- [ ] Filter by localized name, desktop id, executable, category.
- [ ] Show icon name/path, localized name, desktop id, exec, startup WM class.
- [ ] Allow selecting an app into the binding editor.
- [ ] Keep hidden/noDisplay apps hidden by default, with an explicit “show hidden apps” toggle.

### Task M2.8: Hotkey Recorder and Conflict Testing

**Files:**

- Create: `src/settings/qml/HotkeyRecorder.qml`
- Modify: `src/ipc/AgentDBusService.cpp`
- Modify: `src/agent/AgentController.cpp`

- [ ] Capture the next key combination in the Settings UI.
- [ ] Normalize modifiers into the same format accepted by `Hotkey::parse()`.
- [ ] Call `TestHotkey(hotkey, excludeActionId)`.
- [ ] In agent, temporarily test registration and immediately release it.
- [ ] Return structured result codes: `ok`, `hotkey_invalid`, `hotkey_conflict`, `hotkey_backend_unavailable`.
- [ ] Do not disturb existing registered hotkeys during testing.

### Task M2.9: Backend Status Page

**Files:**

- Create: `src/settings/qml/BackendStatusPage.qml`
- Modify: `src/agent/AgentController.cpp`

- [ ] Display session type, hotkey backend, window backend.
- [ ] Display capabilities:
  global hotkey, window list, activate window, launch app.
- [ ] Display binding-level status:
  registered, disabled, invalid, conflict, app_not_found.
- [ ] Include raw warning strings for debugging.

### Task M2.10: M2 Verification

**Files:**

- Modify: `docs/manual-test-matrix.md` if it already exists; otherwise defer full matrix to M3.

- [ ] Run `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.
- [ ] Run `cmake --build build`.
- [ ] Run `ctest --test-dir build --output-on-failure`.
- [ ] Start `build/deepswitch-agent --config /tmp/deepswitch-m2.json`.
- [ ] Confirm `qdbus org.deepin.DeepSwitch /org/deepin/DeepSwitch org.deepin.DeepSwitch.Agent.GetStatus` returns a status map.
- [ ] Start `build/deepswitch-settings`.
- [ ] Create bindings for Firefox, Terminal, VS Code.
- [ ] Confirm pressing configured hotkeys launches or focuses each app.
- [ ] Confirm editing a hotkey takes effect without restarting agent.

---

## M3 Detailed Plan

### Task M3.1: User Autostart Manager

**Files:**

- Create: `src/core/AutostartManager.h`
- Create: `src/core/AutostartManager.cpp`
- Create: `tests/core/test_autostart_manager.cpp`
- Modify: `src/agent/AgentController.cpp`
- Modify: `src/settings/qml/Main.qml`

- [ ] Implement writes to `~/.config/autostart/deepswitch-agent.desktop`.
- [ ] Never write system autostart paths.
- [ ] Preserve user config on disable by removing only the generated autostart file.
- [ ] Add tests using a temporary config home.
- [ ] Add Settings toggle bound to `general.autostart`.

Autostart desktop entry:

```ini
[Desktop Entry]
Type=Application
Name=DeepSwitch Agent
Exec=deepswitch-agent
X-GNOME-Autostart-enabled=true
NoDisplay=true
```

### Task M3.2: DDE Visible Entry

**Files:**

- Create: `packaging/linux/org.deepin.DeepSwitch.desktop`
- Create: `packaging/linux/icons/hicolor/scalable/apps/deepswitch.svg`
- Modify: `CMakeLists.txt`

- [ ] Install a visible desktop entry for `deepswitch-settings`.
- [ ] Install icon under hicolor scalable apps.
- [ ] Ensure the settings app has a stable application id.
- [ ] Confirm it appears in launcher search after installation.

### Task M3.3: Overlay Light Hint

**Files:**

- Create: `src/overlay/main.cpp`
- Create: `src/overlay/OverlayWindow.h`
- Create: `src/overlay/OverlayWindow.cpp`
- Create: `src/overlay/qml/Overlay.qml`
- Modify: `CMakeLists.txt`
- Modify: `src/agent/AgentController.cpp`

- [ ] Add an overlay executable or lightweight in-agent window; prefer separate executable if it keeps agent headless by default.
- [ ] Show action result: launched, focused, cycled, failed.
- [ ] Auto-hide within 900ms.
- [ ] Respect `general.showOverlay`.
- [ ] Do not block hotkey handling while overlay is visible.

### Task M3.4: Window Tracking and Match Diagnostics

**Files:**

- Modify: `src/backends/x11/X11WindowBackend.h`
- Modify: `src/backends/x11/X11WindowBackend.cpp`
- Modify: `src/core/AppMatcher.h`
- Modify: `src/core/AppMatcher.cpp`
- Modify: `src/ipc/AgentTypes.h`
- Modify: `src/settings/qml/BackendStatusPage.qml`

- [ ] Track active window order using `_NET_ACTIVE_WINDOW`.
- [ ] Add match evidence to `MatchResult`: rule type, value, score delta.
- [ ] Expose `ListWindows(appId)` with scores and evidence.
- [ ] Display diagnostics in Settings for “why this window matched or did not match”.
- [ ] Keep fallback behavior: if event tracking fails, enumerate on trigger.

### Task M3.5: Log File Manager

**Files:**

- Create: `src/core/LogFileManager.h`
- Create: `src/core/LogFileManager.cpp`
- Create: `tests/core/test_log_file_manager.cpp`
- Modify: `src/agent/main.cpp`

- [ ] Write logs to `~/.local/state/deepswitch/deepswitch.log`.
- [ ] Rotate to `deepswitch.log.1` when file exceeds configured size.
- [ ] Default max size: 2 MiB.
- [ ] Keep console logging for CLI one-shot commands.
- [ ] Do not log full environment or sensitive command arguments.

### Task M3.6: Debian Packaging

**Files:**

- Create: `packaging/debian/control`
- Create: `packaging/debian/rules`
- Create: `packaging/debian/changelog`
- Create: `packaging/debian/copyright`
- Create: `packaging/debian/deepswitch.install`
- Modify: `CMakeLists.txt`

- [ ] Add install rules for `deepswitch-agent`, `deepswitch-settings`, desktop files, icon, docs.
- [ ] Package as user-session application, not system daemon.
- [ ] Declare runtime dependencies for Qt6 Core/Gui/Qml/Quick/DBus and X11.
- [ ] Build package with `dpkg-buildpackage -us -uc` or CPack Debian if the team chooses CPack.
- [ ] Confirm installed binaries run from PATH.

### Task M3.7: Manual Test Matrix

**Files:**

- Create: `docs/manual-test-matrix.md`

- [ ] Document test environment: deepin version, session type, display server, Qt version.
- [ ] Cover Firefox, Deepin Terminal, VS Code, Files.
- [ ] Cover launch, focus, cycle, minimized window, other workspace, disabled binding, hotkey conflict.
- [ ] Cover settings save/reload and agent restart persistence.
- [ ] Cover install, autostart, uninstall, config preservation.

### Task M3.8: M3 Verification

- [ ] Run `cmake -B build -DCMAKE_BUILD_TYPE=Release`.
- [ ] Run `cmake --build build`.
- [ ] Run `ctest --test-dir build --output-on-failure`.
- [ ] Install package in a clean user session.
- [ ] Enable autostart from Settings.
- [ ] Log out and log in.
- [ ] Confirm hotkeys work before opening Settings.
- [ ] Confirm log rotation by lowering max size in a test config and generating events.
- [ ] Confirm uninstall leaves `~/.config/deepswitch/config.json` untouched.

---

## M4 Detailed Plan

### Task M4.1: Session and Capability Detection

**Files:**

- Create: `src/backends/BackendStatus.h`
- Create: `src/backends/SessionDetector.h`
- Create: `src/backends/SessionDetector.cpp`
- Create: `tests/backends/test_session_detector.cpp`
- Modify: `src/agent/AgentController.cpp`

- [ ] Read `XDG_SESSION_TYPE`, `WAYLAND_DISPLAY`, `DISPLAY`, `XDG_CURRENT_DESKTOP`, `DESKTOP_SESSION`, `XDG_SESSION_DESKTOP`, `QT_QPA_PLATFORM`.
- [ ] Classify session as `x11`, `wayland`, `treeland`, or `unknown`.
- [ ] Report capability booleans:
  globalHotkeyAvailable, windowListAvailable, activateWindowAvailable, launchAppAvailable.
- [ ] Ensure missing env vars never crash agent.
- [ ] Add tests for X11, Wayland, mixed, and unknown environments.

### Task M4.2: Portal Global Shortcuts Probe

**Files:**

- Create: `src/backends/portal/PortalGlobalShortcutsProbe.h`
- Create: `src/backends/portal/PortalGlobalShortcutsProbe.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/wayland-treeland-findings.md`

- [ ] Probe `org.freedesktop.portal.Desktop` on session bus.
- [ ] Detect whether GlobalShortcuts interface is present.
- [ ] Record portal version and method availability.
- [ ] Do not make portal the default hotkey backend until registration is proven manually on deepin v25.

### Task M4.3: Limited Wayland Backends

**Files:**

- Create: `src/backends/wayland/LimitedWaylandHotkeyBackend.h`
- Create: `src/backends/wayland/LimitedWaylandHotkeyBackend.cpp`
- Create: `src/backends/wayland/LimitedWaylandWindowBackend.h`
- Create: `src/backends/wayland/LimitedWaylandWindowBackend.cpp`
- Modify: `src/agent/AgentController.cpp`

- [ ] If portal shortcuts are unavailable, return `hotkey_backend_unavailable` with a clear message.
- [ ] If portal shortcuts are available, implement registration only after a successful manual proof.
- [ ] Window backend returns empty window list with capability `windowListAvailable=false` unless a verified DDE/Treeland API exists.
- [ ] Activation returns `window_backend_unavailable` instead of pretending success.
- [ ] Launching applications remains available through core `Launcher`.

### Task M4.4: Treeland/DDE Interface Research

**Files:**

- Create: `docs/wayland-treeland-findings.md`

- [ ] Record environment variables for deepin v25 X11 session.
- [ ] Record environment variables for deepin v25 Wayland/Treeland session.
- [ ] Run `qdbus` service inspection for DDE and Treeland-related names.
- [ ] Inspect whether any API exposes window list, active window, focus/activate.
- [ ] For each discovered API, record stability, permission model, sample call, and failure behavior.
- [ ] Conclude whether it is safe for default use, optional use, or research-only.

### Task M4.5: Settings Capability UI

**Files:**

- Modify: `src/settings/qml/BackendStatusPage.qml`
- Modify: `src/settings/qml/BindingsPage.qml`
- Modify: `src/settings/SettingsController.cpp`

- [ ] Show session type prominently.
- [ ] If hotkeys unavailable, disable hotkey save and show reason.
- [ ] If window activation unavailable, allow binding creation but label behavior as “launch only”.
- [ ] If app launch remains available, keep launch test button enabled.
- [ ] Include a copyable diagnostics block for bug reports.

### Task M4.6: M4 Verification

- [ ] Run full test suite on X11 and confirm no regression.
- [ ] Start agent on Wayland/Treeland session and confirm it does not crash.
- [ ] Confirm Settings shows correct degraded capability state.
- [ ] If portal hotkeys are available, manually bind one app and confirm trigger launches it.
- [ ] If window activation is unavailable, confirm action result says so and does not claim focus success.
- [ ] Commit `docs/wayland-treeland-findings.md` with exact deepin build/session details.

---

## Cross-Version Technical Decisions

- Keep `src/core/` free of X11, D-Bus, QML, and Wayland headers.
- Keep `src/settings/` as a client; it should not mutate live config files directly.
- Use stable error codes and localized UI text outside core logic.
- Prefer explicit degraded states over silent fallback.
- Preserve user config on parse failures by backing up invalid files instead of overwriting.
- Do not add system-level services or root-only behavior before packaging requirements prove it is necessary.

## Recommended Execution Order

1. M2.1-M2.2: runtime DTOs and `AgentController`.
2. M2.3-M2.4: D-Bus service/client.
3. M2.5-M2.9: Settings UI features.
4. M2.10: full M2 verification.
5. M3.1-M3.5: daily-use runtime polish.
6. M3.6-M3.8: packaging and manual test matrix.
7. M4.1-M4.2: session/portal probing.
8. M4.3-M4.6: limited backend, UI capability display, findings doc.

## Risk Register

- D-Bus type complexity: mitigate by using `QVariantMap`/`QVariantList` at the boundary and testing round trips.
- Hotkey conflict testing can disturb live grabs: mitigate with temporary registration and immediate unregistration, excluding the edited action id.
- Settings UI can drift from agent state: mitigate by treating agent as source of truth and refreshing after every mutation.
- Overlay may make the headless agent depend on GUI runtime: mitigate by keeping overlay separate or optional.
- Wayland/Treeland may not expose window activation: mitigate by reporting degraded capabilities and keeping launch-only behavior usable.

## Final Verification Commands

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

M2 adds manual verification:

```bash
build/deepswitch-agent --config /tmp/deepswitch-m2.json
qdbus org.deepin.DeepSwitch /org/deepin/DeepSwitch org.deepin.DeepSwitch.Agent.GetStatus
build/deepswitch-settings
```

M3 adds packaging verification:

```bash
cmake --build build --target install
dpkg-buildpackage -us -uc
```

M4 adds environment verification:

```bash
env | grep -E 'XDG_SESSION_TYPE|WAYLAND_DISPLAY|DISPLAY|XDG_CURRENT_DESKTOP|DESKTOP_SESSION|XDG_SESSION_DESKTOP|QT_QPA_PLATFORM'
qdbus | grep -Ei 'portal|deepin|dde|treeland'
```

## Self-Review

- Spec coverage: M2.1-M2.10 covers D-Bus service/client, settings pages, hotkey recording, backend status, config save/reload, and UI error states. M3.1-M3.8 covers autostart, visible entry, overlay, tracking diagnostics, log rotation, packaging, uninstall/config preservation, and manual tests. M4.1-M4.6 covers session detection, portal probing, Treeland/DDE research, limited backend, UI degradation, and findings documentation.
- Placeholder scan: no task depends on undefined “later” work; M4 explicitly gates unverified portal/Treeland behavior behind findings and degraded status.
- Type consistency: the plan uses `AgentController`, `AgentDBusService`, `AgentDBusClient`, DTO maps, `BackendStatusDto`, and stable error codes consistently across agent, IPC, and Settings.

