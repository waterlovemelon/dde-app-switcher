# deepin v25 上开发 Manico-like 应用切换器方案

> 文档版本：v0.2  
> 目标平台：deepin v25  
> 推荐路线：X11-first，Wayland/Treeland-ready  
> 暂定项目名：DeepSwitch / DManico / DDE Quick Switcher

---

## 1. 项目目标

本项目目标是在 deepin v25 上开发一个类似 macOS Manico 的键盘优先应用切换器。

v0.2 细化目标是把方案从“架构方向”推进到“可执行设计”。文档需要能够回答：

```text
1. 每个模块负责什么，不负责什么；
2. 模块之间通过什么数据结构和接口通信；
3. 快捷键触发后完整调用链是什么；
4. X11 MVP 哪些能力必须跑通，哪些能力可以降级；
5. 配置、状态、错误和日志如何落地；
6. M0-M3 如何拆成可以实现和验收的任务。
```

核心体验是：

```text
一个快捷键对应一个应用。
按下快捷键后：
- 如果应用没有启动，则启动应用；
- 如果应用已经启动，则聚焦到该应用窗口；
- 如果应用有多个窗口，则按策略循环、选择或聚焦最近窗口。
```

典型使用方式：

```text
Alt + 1  -> 浏览器
Alt + 2  -> 终端
Alt + 3  -> 编辑器
Alt + 4  -> 文件管理器
Alt + 5  -> 聊天工具
```

或者使用 Leader 模式：

```text
Alt + Space -> 1 -> 浏览器
Alt + Space -> 2 -> 终端
Alt + Space -> 3 -> 编辑器
```

项目不应一开始做成 Alfred、Raycast、Ulauncher 那种综合启动器，而应优先做好“快速应用切换”这一件事。

---

## 2. 产品定位

### 2.1 一句话定义

面向 deepin v25 的 Manico-like 快速应用启动与切换工具。

### 2.2 主要用户

- 习惯 macOS Manico / Raycast / Alfred 快捷键工作流的 Linux 用户；
- deepin 桌面环境用户；
- 重度键盘用户；
- 经常在浏览器、终端、编辑器、文件管理器、聊天软件之间切换的开发者或办公用户。

### 2.3 核心价值

相比系统默认 Alt-Tab，本工具的优势是：

```text
Alt-Tab：按窗口历史顺序循环，需要看屏幕和判断位置。
本工具：按固定快捷键直达目标应用，形成肌肉记忆。
```

### 2.4 非目标

第一阶段不做：

- 通用搜索启动器；
- 剪贴板管理器；
- 插件系统；
- 云同步；
- 跨所有 Linux 桌面环境的完整兼容；
- Wayland 下完整窗口控制；
- 替换 deepin Dock 或启动器。

---

## 3. 设计原则

### 3.1 X11-first，Wayland/Treeland-ready

deepin v25 可能使用 X11，也可能使用 Wayland/Treeland。第一版应先在 X11 下完整可用，因为 X11 下全局快捷键、窗口枚举、窗口激活的实现路径更成熟。

但架构上必须预留 Wayland/Treeland 后端，避免后续推倒重写。

### 3.2 核心逻辑和显示协议解耦

不要把 X11 代码写死在业务逻辑里。应该抽象出：

```text
HotkeyBackend：负责全局快捷键。
WindowBackend：负责窗口枚举、匹配、激活。
AppRegistry：负责应用发现和启动信息。
ActionEngine：负责启动/切换/循环策略。
```

这样后续可以替换为：

```text
X11HotkeyBackend      -> PortalHotkeyBackend / TreelandHotkeyBackend
X11WindowBackend      -> TreelandWindowBackend / DDEWindowBackend
```

### 3.3 后台常驻，设置界面可关闭

核心能力应由后台 Agent 提供。设置界面关闭后，快捷键仍然工作。

```text
Core Agent：常驻后台，处理快捷键和窗口动作。
Settings UI：仅用于配置，可以随时打开/关闭。
Overlay UI：仅在需要提示或选择窗口时短暂显示。
```

### 3.4 尽量不修改系统核心目录

deepin v25 有系统不可变/只读相关机制，设计上应避免要求用户修改 `/usr`、替换系统组件、patch DDE 或 Dock。

建议路径：

```text
配置：~/.config/deepswitch/config.json
数据：~/.local/share/deepswitch/state.db
日志：~/.local/state/deepswitch/deepswitch.log
自启动：~/.config/autostart/deepswitch-agent.desktop
```

---

## 4. 总体架构

### 4.1 架构图

```text
┌──────────────────────────────────────────────┐
│                 Settings UI                  │
│  应用选择 / 快捷键录制 / 冲突提示 / 偏好设置     │
└──────────────────────┬───────────────────────┘
                       │ IPC: D-Bus / Unix Socket
┌──────────────────────▼───────────────────────┐
│                 Core Agent                   │
│  配置加载 / 快捷键注册 / 动作调度 / 状态管理       │
└───────┬──────────────┬──────────────┬────────┘
        │              │              │
        │              │              │
┌───────▼───────┐ ┌────▼────────┐ ┌───▼────────────┐
│ HotkeyBackend │ │ AppRegistry │ │ WindowBackend  │
│ X11 / Portal  │ │ .desktop    │ │ X11 / Treeland │
└───────┬───────┘ └────┬────────┘ └───┬────────────┘
        │              │              │
        └──────────────▼──────────────┘
                    ActionEngine
              启动 / 聚焦 / 循环 / 选择器
                       │
              ┌────────▼────────┐
              │   Overlay UI    │
              │  窗口选择 / 提示 │
              └─────────────────┘
```

### 4.2 进程模型

推荐采用两个主进程：

```text
deepswitch-agent      后台常驻进程
deepswitch-settings   设置界面进程
```

可选第三个进程：

```text
deepswitch-overlay    独立浮层进程
```

但第一版可以让 Overlay 由 Agent 内部启动一个轻量窗口实现。

### 4.3 运行时线程模型

MVP 不建议一开始引入复杂多进程或多线程模型。推荐以 Qt 主事件循环为中心，按职责拆分 QObject 组件。

```text
deepswitch-agent
├── 主线程：Qt event loop
│   ├── AgentController
│   ├── ConfigManager
│   ├── ActionEngine
│   ├── DBusService
│   └── OverlayController
├── X11 事件源：通过 QSocketNotifier 或后台事件泵接入主循环
└── 可选 worker：应用扫描、图标加载、慢速 I/O

deepswitch-settings
└── 主线程：Qt/QML UI + DBus client
```

约束：

```text
1. ActionEngine 不直接阻塞 UI 或 X11 事件循环；
2. .desktop 扫描、图标解析、配置导入等慢操作可放 worker；
3. X11 连接对象由后端独占，避免多个模块直接操作同一个 Display/Connection；
4. 所有后端事件最终转换成 Agent 内部信号，再进入 ActionEngine；
5. 设置界面不持有核心状态，真实状态以 Agent 为准。
```

### 4.4 关键数据流

#### 4.4.1 Agent 启动数据流

```text
main()
  -> 创建 QCoreApplication / QApplication
  -> 初始化 Logging
  -> ConfigManager.load()
  -> SessionDetector.detect()
  -> BackendFactory.createHotkeyBackend()
  -> BackendFactory.createWindowBackend()
  -> AppRegistry.scan()
  -> AgentController.validateConfig()
  -> HotkeyManager.registerEnabledBindings()
  -> DBusService.registerService()
  -> 进入事件循环
```

任何步骤失败都不应直接静默退出。推荐行为：

```text
配置损坏：加载默认配置，备份坏配置，通知设置界面。
后端不可用：Agent 继续运行，但状态为 BackendUnavailable。
D-Bus 注册失败：记录 fatal 日志并退出，因为 Settings 无法管理 Agent。
快捷键部分失败：Agent 继续运行，失败绑定标记为 HotkeyConflict。
```

#### 4.4.2 快捷键触发数据流

```text
HotkeyBackend 捕获按键
  -> HotkeyManager 映射 actionId
  -> AgentController 检查 enabled/paused
  -> ActionEngine.handle(actionId)
  -> ConfigManager 查 binding
  -> AppRegistry 查 AppInfo
  -> WindowBackend 查匹配窗口
  -> AppMatcher 评分过滤
  -> ActionEngine 决策 launch / focus / cycle / picker
  -> Launcher 或 WindowBackend 执行动作
  -> OverlayController 显示结果
  -> RuntimeState 更新最近窗口和 cycle 状态
```

#### 4.4.3 配置变更数据流

```text
Settings UI 编辑绑定
  -> DBus SetBinding / RemoveBinding
  -> Agent 校验 hotkey、desktop_id、match_rules
  -> ConfigManager 原子写入 config.json
  -> HotkeyManager 只重注册受影响快捷键
  -> ActionEngine 清理对应运行时状态
  -> DBus 发 BindingChanged / StatusChanged
```

配置变更必须支持失败回滚：

```text
1. 新配置先在内存中校验；
2. 快捷键试注册成功后再替换旧绑定；
3. 写入配置失败时恢复旧绑定；
4. 任一步失败都返回明确错误码。
```

### 4.5 能力边界矩阵

| 能力 | X11 MVP | Wayland 有限版 | Treeland 后续版 |
| --- | --- | --- | --- |
| 全局快捷键 | XGrabKey / XCB grab | Portal 或不可用 | Portal / 私有接口 |
| 应用启动 | .desktop Exec | .desktop Exec | .desktop Exec |
| 窗口枚举 | EWMH | 不保证 | 依赖 Treeland/DDE 接口 |
| 窗口激活 | EWMH ClientMessage | 不保证 | 依赖 Treeland/DDE 接口 |
| 多窗口循环 | 支持 | 不支持或降级 | 视接口支持 |
| 设置界面 | 支持 | 支持 | 支持 |
| Overlay | 支持 | 支持 | 支持 |

实现时所有能力都应通过 BackendStatus 暴露给 Settings UI，避免用户在 Wayland 下误以为功能损坏。

---

## 5. 技术栈建议

### 5.1 技术栈

```text
语言：C++20
UI：Qt 6 + QML
X11：XCB / Xlib / xcb-ewmh
IPC：QtDBus
配置：JSON
状态数据：SQLite，可选；MVP 可先不用
打包：deb
构建：CMake
日志：Qt Logging
```

### 5.2 依赖边界

MVP 依赖应控制在 deepin v25 容易获得的基础库内。

```text
必需：
- Qt6::Core
- Qt6::Gui
- Qt6::Qml / Qt6::Quick
- Qt6::DBus
- X11 / xcb / xcb-ewmh

建议：
- Qt6::Sql：只有引入 SQLite 状态库时使用
- Qt6::Svg：图标显示需要 SVG 支持时使用
- KF6WindowSystem：如果 deepin 环境可稳定提供，可调研但不作为 MVP 强依赖

避免：
- 依赖特定窗口管理器私有库作为 X11 MVP 的核心路径
- 一开始引入插件框架、脚本运行时或跨桌面抽象大库
```

### 5.3 CMake Target 划分

建议把核心逻辑编译成静态库或对象库，Agent、Settings、测试都链接它。

```text
deepswitch_core       配置、应用索引、匹配、ActionEngine
deepswitch_x11        X11 快捷键和窗口后端
deepswitch_ipc        D-Bus 类型和服务封装
deepswitch_agent      后台进程入口
deepswitch_settings   设置界面入口
deepswitch_tests      单元测试
```

原则：

```text
core 不依赖 QML；
core 不直接依赖具体 X11 类型；
settings 不直接访问配置文件，优先通过 D-Bus 操作 Agent；
tests 可以绕过 D-Bus，直接测试 core。
```

---

## 6. 核心功能设计

### 6.1 快捷键模式

### 6.1.1 直接模式

用户按下主快捷键(比如Alt)后显示悬浮窗口,窗口内按照顺序展示应用的图标和对应的数字或者字母.应用的顺序可以在设置页面进行设置.

用户按下完整快捷键后直接触发目标应用。

示例：

```text
Alt + 1 -> Firefox
Alt + 2 -> Deepin Terminal
Alt + 3 -> VS Code
```

优点：

- 速度最快；
- 最接近 Manico 的直达体验；
- 学习成本低。

### 设置快捷键

1. 直接设置快捷键来替换Alt

2. 可以设置qwerasd等字母来替换 1 2 3

### 6.1.2 Leader 模式

Leader 模式不是 MVP 必须项，但架构应提前兼容。它把一次直接快捷键拆成两段输入：

```text
Leader 键：Alt+Space
选择键：1 / 2 / 3 / Q / W / E
完整动作：Alt+Space -> 1
```

状态机：

```text
Idle
  收到 leader_key -> LeaderActive，显示 App Hint Overlay，启动 timeout

LeaderActive
  收到有效选择键 -> 执行对应 action，关闭 Overlay，回到 Idle
  收到 Esc -> 取消，关闭 Overlay，回到 Idle
  timeout -> 关闭 Overlay，回到 Idle
  收到未知键 -> 可选择忽略或取消，MVP 建议取消
```

设计约束：

```text
1. Direct 模式和 Leader 模式共用 Binding、ActionEngine、AppRegistry、WindowBackend；
2. Leader 模式只改变 HotkeyBackend 到 ActionId 的映射方式；
3. Settings UI 中的按键录制需要区分 hotkey 和 leader_selection_key；
4. Leader 模式的 Overlay 只能作为提示，不应成为执行动作的必需条件。
```

### 6.1.3 快捷键规范化

内部存储不要直接使用用户输入字符串，应规范化为稳定结构。

```text
用户输入：alt + 1、Alt+1、Meta+Shift+Q
内部展示：Alt+1、Meta+Shift+Q
内部注册：key symbol + modifier mask
配置存储：规范化字符串
```

需要处理的别名：

```text
Ctrl / Control
Alt / Mod1
Super / Meta / Win / Mod4
Return / Enter
Esc / Escape
```

无效快捷键应在保存前拦截：

```text
只有修饰键，没有主键；
主键无法映射到当前键盘布局；
重复绑定；
与 Leader 键冲突；
当前后端不支持。
```

---

### 6.2 应用绑定

每个快捷键绑定一个 AppTarget。

```json
{
  "hotkey": "Alt+1",
  "target": {
    "desktop_id": "firefox.desktop",
    "name": "Firefox",
    "exec": "firefox %u",
    "startup_wm_class": "firefox",
    "match_rules": [
      { "type": "wm_class", "value": "firefox" },
      { "type": "process_name", "value": "firefox" }
    ]
  },
  "behavior": {
    "multi_window": "cycle",
    "launch_if_not_running": true,
    "focus_existing_window": true
  }
}
```

### 6.2.1 Binding 与 AppTarget 的关系

实现中建议把用户绑定和应用元数据分开：

```text
Binding：用户配置，表示哪个动作绑定哪个应用和行为。
AppInfo：从 .desktop 扫描得到的应用元数据。
AppTarget：ActionEngine 执行时由 Binding + AppInfo 合成的运行时目标。
```

这样 .desktop 文件更新后，不需要重写用户配置；配置里只存稳定 ID 和用户覆盖项。

```json
{
  "id": "app.firefox",
  "enabled": true,
  "hotkey": "Alt+1",
  "desktop_id": "firefox.desktop",
  "display_name_override": "",
  "exec_override": "",
  "multi_window_strategy": "cycle",
  "launch_if_not_running": true,
  "focus_existing_window": true,
  "match_rules": []
}
```

字段规则：

```text
desktop_id：优先指向 AppRegistry 中的应用；
display_name_override：仅用于 UI 展示，不影响匹配；
exec_override：手动命令模式才使用；
match_rules：用户规则优先级高于自动规则；
enabled=false：不注册快捷键，但保留配置。
```

---

### 6.3 应用启动行为

触发目标应用时，流程如下：

```text
1. 根据快捷键找到 AppTarget。
2. 查询 WindowBackend，查找该应用已有窗口。
3. 如果没有窗口：
   - 根据 .desktop Exec 启动应用；
   - 显示“正在启动”提示；
   - 在一段时间内监听新窗口出现。
4. 如果有一个窗口：
   - 激活该窗口。
5. 如果有多个窗口：
   - 根据多窗口策略处理。
```

### 6.3.1 启动器职责

Launcher 独立于 ActionEngine，负责把 AppInfo 转换成进程启动请求。

```text
输入：AppInfo / exec_override / 工作目录 / 环境变量
输出：LaunchResult
副作用：启动进程，记录启动时间，必要时写日志
```

LaunchResult：

```cpp
enum class LaunchStatus {
    Started,
    FailedInvalidDesktopFile,
    FailedInvalidExec,
    FailedProcessStart,
    FailedPermissionDenied
};

struct LaunchResult {
    LaunchStatus status;
    std::optional<qint64> pid;
    QString errorMessage;
};
```

### 6.3.2 启动后窗口追踪

很多应用启动命令返回后不会立刻出现窗口。Agent 需要短时间追踪新窗口。

```text
1. 记录 PendingLaunch(appId, startedAt, optionalPid)；
2. 监听 WindowBackend.windowCreated；
3. 对新窗口运行 AppMatcher；
4. 匹配成功后激活窗口并清理 PendingLaunch；
5. 超过 launch_timeout_ms 后显示启动失败或未发现窗口提示。
```

建议默认值：

```text
launch_timeout_ms = 8000
post_launch_focus_delay_ms = 150
```

注意：

```text
某些应用已在后台运行但没有普通窗口；
某些应用启动会先出现 splash，再出现主窗口；
某些应用启动后窗口在其他工作区；
Terminal=true 的 .desktop 需要通过终端模拟器启动。
```

MVP 对 Terminal=true 可先提示“不支持终端应用启动”，或使用 deepin-terminal 作为默认终端包装器；推荐前者，避免启动行为不可预测。

---

### 6.4 多窗口策略

### 6.4.1 最近使用优先

始终激活最近使用过的该应用窗口。

```text
Firefox 有 3 个窗口：A、B、C
最近使用的是 B
按快捷键 -> 激活 B
```

### 6.4.2 连续按键循环

短时间内连续按同一快捷键，在该应用多个窗口间循环。

```text
Alt + 1 -> Firefox 窗口 A
Alt + 1 -> Firefox 窗口 B
Alt + 1 -> Firefox 窗口 C
Alt + 1 -> Firefox 窗口 A
```

需要记录：

```text
last_action_id
last_trigger_time
last_window_index_by_app
cycle_timeout_ms
```

### 6.4.3 弹出窗口选择器

当目标应用有多个窗口时，弹出 Overlay 显示窗口列表。

```text
Firefox
1. GitHub - Firefox
2. 文档 - Firefox
3. 邮箱 - Firefox
```

用户按数字选择窗口。

### 6.4.4 MVP 建议

第一版优先实现：

```text
默认：最近使用优先
可选：连续按键循环
暂缓：窗口选择器
```

窗口选择器可以放到 V2。

---

## 7. 模块设计

### 7.1 Core Agent

### 7.1.1 职责

Core Agent 是项目核心后台进程。

职责：

```text
1. 加载配置；
2. 初始化 HotkeyBackend；
3. 初始化 AppRegistry；
4. 初始化 WindowBackend；
5. 注册全局快捷键；
6. 监听快捷键事件；
7. 调用 ActionEngine 执行动作；
8. 向 Settings UI 暴露 IPC 接口；
9. 写入日志和状态。
```

### 7.1.2 Agent 生命周期

```text
启动
  ↓
加载配置
  ↓
检测当前会话类型：X11 / Wayland
  ↓
选择对应 Backend
  ↓
扫描应用列表
  ↓
注册快捷键
  ↓
进入事件循环
```

### 7.1.3 Agent 状态

```text
Running
Paused
ReloadingConfig
BackendUnavailable
HotkeyConflict
Error
```

### 7.1.4 AgentController 接口边界

AgentController 是进程内的协调层，不直接实现 X11、配置解析或应用匹配。

```cpp
class AgentController : public QObject {
    Q_OBJECT
public:
    bool start();
    void stop();

public slots:
    void pause();
    void resume();
    ReloadResult reloadConfig();
    ActionResult triggerAction(const QString& actionId);

signals:
    void statusChanged(const AgentStatus& status);
    void bindingChanged(const QString& actionId);
    void errorOccurred(const AgentError& error);
};
```

职责边界：

```text
AgentController：
- 组合各模块；
- 管理生命周期；
- 处理配置 reload；
- 聚合状态给 D-Bus。

ActionEngine：
- 只负责动作决策；
- 不负责读取配置文件；
- 不负责注册快捷键。

Backend：
- 只负责平台能力；
- 不理解用户配置语义。
```

### 7.1.5 RuntimeState

运行时状态和用户配置分开保存。MVP 可先只放内存，不落 SQLite。

```cpp
struct RuntimeState {
    QString lastActionId;
    QElapsedTimer lastTriggerTimer;
    QHash<QString, WindowId> lastFocusedWindowByApp;
    QHash<QString, int> lastCycleIndexByApp;
    QHash<QString, PendingLaunch> pendingLaunches;
    BackendStatus backendStatus;
};
```

状态来源：

```text
lastFocusedWindowByApp：由 WindowBackend activeWindowChanged 更新；
lastCycleIndexByApp：由 ActionEngine cycle 策略更新；
pendingLaunches：由 Launcher 和窗口创建事件共同维护；
backendStatus：由后端初始化和运行时错误更新。
```

这些状态不应写回 config.json，避免配置文件被频繁写入。

---

### 7.2 HotkeyBackend

### 7.2.1 接口设计

```cpp
struct Hotkey {
    std::string sequence;   // 例如 "Alt+1"
    int keycode;
    int modifiers;
};

using ActionId = std::string;

class IHotkeyBackend {
public:
    virtual ~IHotkeyBackend() = default;

    virtual bool initialize() = 0;
    virtual RegisterHotkeyResult registerHotkey(const Hotkey& hotkey, const ActionId& actionId) = 0;
    virtual bool unregisterHotkey(const ActionId& actionId) = 0;
    virtual void unregisterAll() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};
```

推荐让具体实现通过 Qt signal 上报事件：

```cpp
struct RegisterHotkeyResult {
    bool ok;
    QString backend;
    QString errorCode;
    QString message;
};

signals:
    void hotkeyTriggered(QString actionId);
    void hotkeyRegistrationChanged(QString actionId, RegisterHotkeyResult result);
    void backendError(BackendError error);
```

HotkeyBackend 不应直接调用 ActionEngine。它只发布 actionId，由 HotkeyManager 或 AgentController 转发。

### 7.2.2 X11 实现

X11 版本可使用：

```text
XGrabKey / XUngrabKey
XNextEvent
XKeyEvent
XkbKeycodeToKeysym
```

或使用 XCB：

```text
xcb_grab_key
xcb_ungrab_key
xcb_poll_for_event
```

### 7.2.3 快捷键冲突处理

注册失败时，记录失败原因并反馈给设置界面。

```text
Alt+1 注册失败：可能已被系统或其他应用占用。
```

配置界面需要显示：

```text
可用
冲突
无效
当前后端不支持
```

### 7.2.4 HotkeyManager

HotkeyManager 位于 Agent 内部，负责把配置绑定转换成后端注册请求。

```text
输入：Binding[]
输出：注册成功/失败状态
依赖：IHotkeyBackend、HotkeyParser
不依赖：WindowBackend、AppRegistry、Overlay
```

核心操作：

```cpp
class HotkeyManager : public QObject {
public:
    ApplyBindingsResult applyBindings(const QList<Binding>& bindings);
    RegisterHotkeyResult testHotkey(const Hotkey& hotkey);
    void clear();

signals:
    void triggered(QString actionId);
    void conflictDetected(QString actionId, QString message);
};
```

配置 reload 时不必全部 unregister/register。推荐 diff：

```text
1. 旧配置有、新配置没有：unregister；
2. 旧配置没有、新配置有：register；
3. hotkey 字符串变化：unregister old，再 register new；
4. 只有行为策略变化：不重注册快捷键。
```

---

### 7.3 AppRegistry

### 7.3.1 职责

AppRegistry 负责扫描系统应用并建立索引。

扫描目录：

```text
/usr/share/applications
/usr/local/share/applications
~/.local/share/applications
/var/lib/flatpak/exports/share/applications    可选
~/.local/share/flatpak/exports/share/applications 可选
```

MVP 可以先支持前三个路径。

### 7.3.2 需要解析的 desktop 字段

```text
Name
Name[zh_CN]
GenericName
Comment
Exec
Icon
Terminal
Type
NoDisplay
Hidden
Categories
StartupWMClass
StartupNotify
```

### 7.3.3 AppInfo 数据结构

```cpp
struct AppInfo {
    std::string desktopId;
    std::string name;
    std::string localizedName;
    std::string exec;
    std::string icon;
    std::string startupWmClass;
    std::vector<std::string> categories;
    std::filesystem::path desktopFilePath;
    bool terminal;
    bool noDisplay;
};
```

### 7.3.4 Exec 字段处理

`.desktop` 的 Exec 字段可能包含：

```text
%f %F %u %U %i %c %k
```

启动前需要清理或替换这些 field codes。

MVP 可采用简单策略：

```text
删除 %f %F %u %U %i %c %k 等占位符。
保留实际命令和参数。
```

后续再严格实现 Desktop Entry 规范。

### 7.3.5 desktop_id 生成规则

desktop_id 必须稳定，否则用户配置会失效。

```text
文件：/usr/share/applications/firefox.desktop
desktop_id：firefox.desktop

文件：/usr/share/applications/org.deepin.FileManager.desktop
desktop_id：org.deepin.FileManager.desktop

文件：~/.local/share/applications/foo/bar.desktop
desktop_id：foo-bar.desktop 或记录完整相对路径映射
```

MVP 建议：

```text
1. 对标准 applications 目录直接使用文件名；
2. 如果出现同名 desktop 文件，按目录优先级选择最终 AppInfo；
3. 同名冲突项在 Settings UI 中显示 desktopFilePath；
4. 用户配置保存 desktop_id，同时可保存 desktop_file_path_hint 辅助恢复。
```

目录优先级：

```text
~/.local/share/applications
/usr/local/share/applications
/usr/share/applications
Flatpak exports 路径
```

### 7.3.6 AppRegistry 缓存和刷新

MVP 可以启动时全量扫描，设置界面打开时使用内存缓存。后续可监听目录变化。

```cpp
class AppRegistry : public QObject {
public:
    ScanResult scan();
    QList<AppInfo> listApplications(AppListFilter filter = {});
    std::optional<AppInfo> findByDesktopId(const QString& desktopId) const;
    std::optional<AppInfo> resolveBindingTarget(const Binding& binding) const;

signals:
    void applicationsChanged();
};
```

过滤规则：

```text
Type != Application：过滤；
Hidden=true：过滤；
NoDisplay=true：默认过滤，但允许 Settings 中“显示隐藏应用”；
Exec 为空：过滤；
Terminal=true：保留但标记为需要终端。
```

排序规则：

```text
1. 用户已绑定应用优先；
2. 本地化名称按 locale 排序；
3. desktop_id 作为稳定兜底排序。
```

---

### 7.4 WindowBackend

### 7.4.1 职责

WindowBackend 负责所有窗口相关操作：

```text
1. 枚举当前窗口；
2. 获取窗口属性；
3. 将窗口和应用进行匹配；
4. 激活窗口；
5. 获取当前活动窗口；
6. 获取窗口所在工作区；
7. 判断窗口是否最小化；
8. 监听窗口创建、销毁、激活变化。
```

### 7.4.2 接口设计

```cpp
using WindowId = uint64_t;
using AppId = std::string;

struct WindowInfo {
    WindowId id;
    std::string title;
    std::string wmClass;
    std::string instanceName;
    int pid;
    int desktop;
    bool minimized;
    bool active;
    int lastActiveOrder;
    std::string windowType;
    std::string appId;
    bool skipTaskbar;
};

class IWindowBackend {
public:
    virtual ~IWindowBackend() = default;

    virtual bool initialize() = 0;
    virtual std::vector<WindowInfo> listWindows() = 0;
    virtual std::optional<WindowInfo> activeWindow() = 0;
    virtual std::vector<WindowInfo> findWindowsByApp(const AppInfo& app) = 0;
    virtual ActivateWindowResult activateWindow(WindowId windowId, ActivateOptions options) = 0;
    virtual bool closeWindow(WindowId windowId) = 0;

signals:
    void windowCreated(WindowInfo window);
    void windowClosed(WindowId windowId);
    void activeWindowChanged(std::optional<WindowInfo> window);
};
```

推荐补充能力查询接口：

```cpp
struct WindowBackendCapabilities {
    bool canListWindows;
    bool canActivateWindow;
    bool canRestoreMinimized;
    bool canSwitchWorkspace;
    bool canObserveWindowEvents;
};
```

### 7.4.3 X11 窗口枚举

X11 下主要读取：

```text
_NET_CLIENT_LIST
_NET_CLIENT_LIST_STACKING
_NET_ACTIVE_WINDOW
_NET_WM_NAME
WM_NAME
WM_CLASS
_NET_WM_PID
_NET_WM_DESKTOP
WM_STATE
```

### 7.4.4 X11 窗口激活

激活窗口时可发送 `_NET_ACTIVE_WINDOW` ClientMessage 给 root window。

需要处理：

```text
最小化窗口恢复
跨工作区切换
多显示器
窗口管理器策略限制
```

MVP 可以先实现：

```text
当前工作区 + 普通窗口激活
```

后续再完善跨工作区行为。

### 7.4.5 窗口过滤规则

WindowBackend 的 listWindows 可以返回所有可见候选窗口，也可以返回原始窗口再交给 AppMatcher 过滤。为减少上层复杂度，建议在后端先过滤明显不该切换的窗口。

默认过滤：

```text
_NET_WM_WINDOW_TYPE_DOCK
_NET_WM_WINDOW_TYPE_DESKTOP
_NET_WM_WINDOW_TYPE_NOTIFICATION
_NET_WM_WINDOW_TYPE_SPLASH
_NET_WM_WINDOW_TYPE_TOOLTIP
_NET_WM_WINDOW_TYPE_MENU
_NET_WM_STATE_SKIP_TASKBAR
override_redirect=true 的窗口
```

谨慎过滤：

```text
UTILITY：部分应用偏好窗口可能是 utility，默认可过滤；
DIALOG：有些主窗口会误标 dialog，MVP 不强过滤；
没有标题的窗口：浏览器启动中可能短暂无标题，不应立即丢弃。
```

### 7.4.6 最近使用顺序

X11 可用 `_NET_CLIENT_LIST_STACKING` 推断窗口堆叠顺序，但它不完全等价于最近使用顺序。MVP 推荐两层策略：

```text
1. activeWindowChanged 事件更新 lastFocusedWindowByApp；
2. 没有运行时记录时，使用 _NET_CLIENT_LIST_STACKING 的后部窗口作为最近候选；
3. 如果仍无法判断，按窗口创建或枚举顺序兜底。
```

ActionEngine 不应依赖 WindowInfo.lastActiveOrder 一定准确，必须有兜底排序。

---

### 7.5 App 与 Window 匹配策略

这是项目最关键的稳定性问题之一。

### 7.5.1 匹配来源

可使用以下信息：

```text
.desktop 文件名
StartupWMClass
WM_CLASS
进程名
窗口 PID
Exec 命令
窗口标题关键词
用户自定义规则
```

### 7.5.2 匹配优先级

建议使用分数模型。

```text
StartupWMClass == WM_CLASS          +100
.desktop 文件名近似 WM_CLASS         +80
Exec 主命令 == 进程名                +70
用户自定义规则命中                  +120
窗口标题关键词命中                  +30
```

当分数超过阈值时，认为窗口属于该应用。

```text
score >= 80 -> 匹配
score < 80  -> 不匹配
```

### 7.5.3 用户修正规则

设置界面中应允许用户为某个应用添加匹配规则：

```text
按 WM_CLASS 匹配
按进程名匹配
按窗口标题包含文本匹配
按正则表达式匹配
```

配置示例：

```json
{
  "desktop_id": "code.desktop",
  "match_rules": [
    { "type": "wm_class", "operator": "equals_ignore_case", "value": "code" },
    { "type": "process_name", "operator": "equals", "value": "code" }
  ]
}
```

### 7.5.4 特殊应用处理

需要注意：

```text
浏览器：可能多进程、多窗口、多 profile。
Electron 应用：WM_CLASS 可能不稳定。
Wine 应用：窗口类名可能奇怪。
Flatpak 应用：desktop_id 和进程名可能不一致。
终端：不同终端窗口可能运行不同任务，但应用仍是同一个。
```

### 7.5.5 AppMatcher 输出

匹配模块不要只返回 bool，应返回可解释结果，便于日志和设置界面诊断。

```cpp
struct MatchEvidence {
    QString source;     // startup_wm_class / wm_class / process_name / user_rule
    QString expected;
    QString actual;
    int score;
};

struct MatchResult {
    bool matched;
    int totalScore;
    QList<MatchEvidence> evidence;
};
```

Settings UI 可以在高级调试页显示：

```text
Firefox -> 窗口 “GitHub - Mozilla Firefox”
总分：120
- StartupWMClass firefox == WM_CLASS firefox：+100
- Exec firefox == process firefox：+20
```

### 7.5.6 匹配流程

推荐流程：

```text
1. 对每个窗口计算基础分；
2. 应用用户自定义规则；
3. 如果用户规则包含 exclude，则命中后直接排除；
4. 分数达到阈值后纳入候选；
5. 候选窗口按最近使用顺序排序；
6. 返回给 ActionEngine。
```

规则类型扩展：

```json
{
  "type": "wm_class",
  "operator": "equals_ignore_case",
  "value": "firefox",
  "weight": 120,
  "effect": "include"
}
```

effect：

```text
include：命中后加分；
exclude：命中后排除；
neutral：只作为诊断信息，不影响结果。
```

---

### 7.6 ActionEngine

### 7.6.1 职责

ActionEngine 负责把“快捷键事件”转换成“用户期望的动作”。

输入：

```text
ActionId
当前窗口状态
应用配置
用户偏好
```

输出：

```text
启动应用
激活窗口
循环窗口
显示 Overlay
报错提示
```

### 7.6.2 核心流程

```text
onHotkeyTriggered(actionId):
    binding = config.findBinding(actionId)
    app = appRegistry.find(binding.desktopId)
    windows = windowBackend.findWindowsByApp(app)

    if windows.empty():
        launcher.launch(app)
        overlay.showLaunching(app)
        return

    if windows.size() == 1:
        windowBackend.activateWindow(windows[0].id)
        return

    strategy = binding.multiWindowStrategy

    if strategy == "recent":
        activateMostRecent(windows)
    else if strategy == "cycle":
        activateNextWindow(app, windows)
    else if strategy == "picker":
        overlay.showWindowPicker(app, windows)
```

### 7.6.3 连续按键循环逻辑

```cpp
struct CycleState {
    std::string appId;
    std::chrono::steady_clock::time_point lastTriggerTime;
    size_t lastIndex;
};
```

伪代码：

```text
if current_app == last_app and now - last_time < cycle_timeout:
    index = (last_index + 1) % windows.size()
else:
    index = most_recent_window_index

activate(windows[index])
update_cycle_state(app, index, now)
```

建议默认超时：

```text
1200ms
```

### 7.6.4 ActionResult

ActionEngine 的结果应结构化，供 Overlay、日志和 D-Bus 复用。

```cpp
enum class ActionStatus {
    Launched,
    Focused,
    Cycled,
    PickerShown,
    IgnoredDisabled,
    FailedNoBinding,
    FailedAppNotFound,
    FailedLaunch,
    FailedNoMatchingWindow,
    FailedBackendUnavailable,
    FailedActivateWindow
};

struct ActionResult {
    ActionStatus status;
    QString actionId;
    QString appId;
    std::optional<WindowId> windowId;
    QString userMessage;
    QString diagnosticMessage;
};
```

显示策略：

```text
成功切换：默认不打扰，可选显示轻提示；
启动应用：显示 “正在启动...”；
失败：显示可读错误；
诊断详情：只进日志或高级界面。
```

### 7.6.5 防抖与重入

用户可能连续快速触发快捷键。ActionEngine 需要避免重入造成重复启动。

```text
同一个 appId 已有 PendingLaunch：再次触发时不重复启动，改为显示“正在启动”或重新尝试匹配窗口；
同一个 actionId 在 50ms 内重复触发：视为键盘重复事件，可忽略；
cycle 策略只在 cycle_timeout_ms 内生效；
配置 reload 期间触发快捷键：返回 ReloadingConfig 或排队到 reload 后执行，MVP 建议直接忽略并提示。
```

### 7.6.6 策略优先级

一次触发的决策顺序：

```text
1. Agent 是否 enabled；
2. Binding 是否存在且 enabled；
3. 当前后端是否支持应用启动和窗口查询；
4. AppInfo 是否可解析；
5. 已有窗口是否可匹配；
6. 没有窗口时是否允许 launch_if_not_running；
7. 多窗口策略；
8. 动作执行结果验证。
```

如果 `focus_existing_window=false`，即使已有窗口也应启动新实例；但这依赖应用是否允许多实例，失败时只记录启动失败，不强行聚焦旧窗口。

---

### 7.7 Overlay UI

### 7.7.1 MVP Overlay

第一版 Overlay 只做轻量提示：

```text
正在启动 Firefox...
已切换到 Terminal
快捷键冲突：Alt+1
```

### 7.7.2 V2 Window Picker

V2 实现窗口选择器：

```text
Firefox
┌────────────────────────────────────┐
│ 1  GitHub - Firefox                │
│ 2  Deepin 文档 - Firefox           │
│ 3  Gmail - Firefox                 │
└────────────────────────────────────┘
```

交互：

```text
数字键选择窗口
Esc 取消
方向键选择
Enter 确认
```

### 7.7.3 App Hint Overlay

Leader 模式下可显示应用列表：

```text
Alt + Space
┌────────────────────────────────────┐
│ 1 Firefox     2 Terminal           │
│ 3 VS Code     4 Files              │
│ 5 WeChat      6 Mail               │
└────────────────────────────────────┘
```

---

### 7.8 Settings UI

### 7.8.1 页面结构

```text
设置首页
├── 快捷键绑定
├── 应用列表
├── 多窗口行为
├── 快捷键冲突
├── 后端状态
├── 开机自启
├── 导入/导出配置
└── 关于
```

### 7.8.2 快捷键绑定页

字段：

```text
启用状态
快捷键
应用名称
多窗口策略
匹配状态
操作按钮
```

示例：

```text
[启用] Alt+1  Firefox           多窗口：循环       状态：正常
[启用] Alt+2  Deepin Terminal   多窗口：最近使用   状态：正常
[禁用] Alt+3  VS Code           多窗口：循环       状态：快捷键冲突
```

### 7.8.3 应用选择器

功能：

```text
搜索应用
显示图标和名称
显示 desktop_id
显示 Exec
显示 StartupWMClass
支持手动添加命令
```

### 7.8.4 后端状态页

显示：

```text
当前会话：X11 / Wayland / Unknown
快捷键后端：X11HotkeyBackend / PortalHotkeyBackend / Unavailable
窗口后端：X11WindowBackend / TreelandWindowBackend / Unavailable
能力状态：
- 全局快捷键：可用/不可用
- 窗口枚举：可用/不可用
- 窗口激活：可用/不可用
- 应用启动：可用/不可用
```

### 7.8.5 设置界面交互约束

Settings UI 是 Agent 的客户端，不应直接修改 Agent 正在使用的配置文件。

```text
读取状态：GetStatus / ListBindings / ListApplications
修改绑定：SetBinding / RemoveBinding
测试快捷键：TestHotkey
保存偏好：SetPreferences 或 SetConfigPatch
触发重载：ReloadConfig
```

界面状态：

```text
Agent 未运行：显示启动 Agent 按钮或错误提示；
D-Bus 不可用：显示无法连接；
后端不可用：允许编辑配置，但标记功能当前不可用；
快捷键冲突：阻止保存或保存为 disabled，MVP 推荐阻止保存；
应用不存在：保留绑定但显示“应用未找到”。
```

### 7.8.6 快捷键录制

快捷键录制应在 Settings UI 内完成初步解析，然后交给 Agent 测试。

流程：

```text
1. 用户点击录制；
2. UI 捕获下一次按键组合；
3. 规范化成 Hotkey 字符串；
4. 调用 TestHotkey；
5. Agent 用当前后端尝试临时注册；
6. 成功则允许保存，失败则显示冲突或不支持。
```

临时注册必须立即释放，不能影响现有绑定。

---

### 7.9 ConfigManager

### 7.9.1 职责

ConfigManager 负责配置读写、校验、默认值和版本迁移。

```text
负责：
- 查找配置路径；
- 首次启动生成默认配置；
- 读取 JSON；
- 应用默认值；
- 校验字段；
- 版本迁移；
- 原子写入；
- 备份损坏配置。

不负责：
- 注册快捷键；
- 扫描应用；
- 执行动作；
- 显示错误。
```

### 7.9.2 接口草案

```cpp
class ConfigManager {
public:
    LoadConfigResult load();
    SaveConfigResult save(const Config& config);
    ValidationResult validate(const Config& config) const;
    MigrationResult migrate(QJsonObject& root);
    Config current() const;
};
```

结果类型：

```cpp
struct ConfigIssue {
    QString path;       // 例如 bindings[0].hotkey
    QString code;       // invalid_hotkey / duplicate_binding / unknown_strategy
    QString message;
    Severity severity;  // warning / error
};
```

### 7.9.3 原子写入策略

```text
1. 写入 config.json.tmp；
2. fsync 或 QFile::flush；
3. 备份旧 config.json 为 config.json.bak；
4. rename tmp 到 config.json；
5. 失败时保留旧配置。
```

如果 JSON 解析失败：

```text
1. 将坏文件复制为 config.invalid.<timestamp>.json；
2. 加载默认配置；
3. Agent 状态加入警告；
4. Settings UI 显示恢复提示。
```

---

### 7.10 Launcher

### 7.10.1 职责

Launcher 只负责启动应用，不负责判断是否应该启动。

```text
输入：AppTarget
输出：LaunchResult
实现：QProcess::startDetached 或 fork/exec
```

MVP 推荐使用 `QProcess::startDetached`，因为实现简单且符合 Qt 应用架构。

### 7.10.2 Exec 解析要求

`.desktop` Exec 不应简单按空格 split。即使 MVP 清理 field code，也要保留引号语义。

示例：

```text
Exec=/opt/My App/bin/app --profile "Work Profile" %u
```

解析策略：

```text
1. 先移除或替换 desktop field codes；
2. 按 shell-like quote 规则拆分 argv；
3. 不通过 /bin/sh -c 执行，除非是用户手动命令模式；
4. 命令不存在时返回 FailedInvalidExec。
```

用户手动命令模式可以允许 shell，但 UI 应明确标记为高级功能。

---

### 7.11 Logging

日志分层：

```text
agent.lifecycle
agent.config
agent.hotkey
agent.window
agent.matcher
agent.action
agent.ipc
ui.settings
```

日志内容要求：

```text
快捷键触发：actionId、hotkey、耗时；
匹配窗口：appId、windowId、score、evidence；
动作结果：launch/focus/cycle、状态、错误码；
配置变更：actionId、字段摘要，不记录敏感命令参数以外的隐私内容；
后端错误：backend、capability、原始错误。
```

默认日志等级：

```text
release：info
debug build：debug
```

日志轮转：

```text
MVP 可暂不实现复杂轮转；
至少限制单文件大小，超过后重命名为 deepswitch.log.1。
```

---

## 8. IPC 设计

Settings UI 需要和 Agent 通信。

### 8.1 推荐方案

优先使用 D-Bus。

Service 名称：

```text
org.deepin.DeepSwitch
```

Object Path：

```text
/org/deepin/DeepSwitch
```

Interface：

```text
org.deepin.DeepSwitch.Agent
```

### 8.2 主要方法

```text
GetStatus() -> AgentStatus
ReloadConfig()
Pause()
Resume()
ListBindings() -> Binding[]
SetBinding(binding)
RemoveBinding(actionId)
TestHotkey(actionId)
ListApplications() -> AppInfo[]
ListWindows(appId) -> WindowInfo[]
ActivateWindow(windowId)
LaunchApp(appId)
```

### 8.3 主要信号

```text
StatusChanged(status)
HotkeyTriggered(actionId)
BindingChanged(actionId)
BackendChanged(backendStatus)
WindowListChanged(appId)
ErrorOccurred(code, message)
```

### 8.4 D-Bus 数据类型

D-Bus 方法不要直接传原始 JSON 字符串作为唯一接口。推荐用可映射到 QVariantMap 的结构，便于 QtDBus 使用。

Binding：

```text
{
  "id": "app.firefox",
  "enabled": true,
  "hotkey": "Alt+1",
  "desktop_id": "firefox.desktop",
  "multi_window_strategy": "cycle",
  "launch_if_not_running": true,
  "focus_existing_window": true,
  "match_rules": [...]
}
```

AgentStatus：

```text
{
  "state": "running",
  "version": "0.2.0",
  "session_type": "x11",
  "backend_status": {...},
  "binding_status": [
    { "id": "app.firefox", "hotkey_status": "registered", "message": "" }
  ],
  "warnings": []
}
```

Error：

```text
{
  "code": "hotkey_conflict",
  "severity": "warning",
  "message": "Alt+1 已被占用",
  "details": "XGrabKey returned BadAccess"
}
```

### 8.5 方法语义细化

```text
GetStatus()
  返回 Agent 聚合状态，不触发扫描或重载。

ReloadConfig()
  从磁盘重读配置，校验后应用；失败时保持旧配置。

SetBinding(binding)
  校验并保存单个绑定；成功后只更新受影响快捷键。

RemoveBinding(actionId)
  删除绑定并注销快捷键。

TestHotkey(hotkey, excludeActionId)
  临时测试快捷键是否可注册；excludeActionId 用于编辑已有绑定。

ListApplications(filter)
  返回 AppRegistry 缓存；filter 可包含 include_hidden、query。

ListWindows(appId)
  返回匹配该应用的窗口和匹配分数；主要用于调试或 V2 picker。

LaunchApp(appId)
  手动启动应用，用于设置界面测试，不改变绑定。
```

### 8.6 错误码

错误码应稳定，UI 根据错误码显示本地化文案。

```text
config_parse_failed
config_validation_failed
config_write_failed
hotkey_invalid
hotkey_conflict
hotkey_backend_unavailable
app_not_found
desktop_file_invalid
launch_failed
window_backend_unavailable
window_not_found
window_activate_failed
permission_denied
agent_reloading
internal_error
```

### 8.7 权限与安全边界

D-Bus service 运行在用户 session bus，不需要 system bus。MVP 不提供远程控制接口。

```text
1. 只注册 session bus name；
2. 不以 root 运行 Agent；
3. 不写 /usr 或系统配置；
4. SetBinding 只接受当前用户配置；
5. LaunchApp 只能启动 AppRegistry 中的应用或用户明确配置的命令。
```

---

## 9. 配置文件设计

### 9.1 配置路径

```text
~/.config/deepswitch/config.json
```

### 9.2 配置示例

```json
{
  "version": 1,
  "general": {
    "enabled": true,
    "autostart": true,
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
    "include_all_workspaces": true,
    "switch_workspace_when_needed": true
  },
  "bindings": [
    {
      "id": "app.firefox",
      "enabled": true,
      "hotkey": "Alt+1",
      "desktop_id": "firefox.desktop",
      "multi_window_strategy": "cycle",
      "match_rules": [
        {
          "type": "wm_class",
          "operator": "equals_ignore_case",
          "value": "firefox"
        }
      ]
    },
    {
      "id": "app.terminal",
      "enabled": true,
      "hotkey": "Alt+2",
      "desktop_id": "deepin-terminal.desktop",
      "multi_window_strategy": "cycle",
      "match_rules": [
        {
          "type": "wm_class",
          "operator": "contains_ignore_case",
          "value": "terminal"
        }
      ]
    }
  ]
}
```

### 9.3 配置版本迁移

配置文件需要带 version 字段。以后升级时可以做迁移：

```text
v1 -> v2
v2 -> v3
```

不要直接破坏用户已有配置。

### 9.4 配置 schema 约束

字段约束：

```text
version：整数，必需；
general.enabled：bool，默认 true；
general.autostart：bool，默认 false；
general.session_backend：auto / x11 / wayland / treeland；
general.show_overlay：bool，默认 true；
general.log_level：trace / debug / info / warning / error；

hotkey.mode：direct / leader；
hotkey.leader_key：快捷键字符串，leader 模式必需；
hotkey.leader_timeout_ms：300-5000；

window.default_multi_window_strategy：recent / cycle / picker；
window.cycle_timeout_ms：300-5000；
window.include_all_workspaces：bool；
window.switch_workspace_when_needed：bool；
window.launch_timeout_ms：1000-30000；

bindings：数组；
bindings[].id：稳定字符串，必需，唯一；
bindings[].enabled：bool；
bindings[].hotkey：快捷键字符串，direct 模式必需；
bindings[].selection_key：leader 模式可选；
bindings[].desktop_id：字符串，和 command 至少一个存在；
bindings[].command：手动命令，可选；
bindings[].multi_window_strategy：recent / cycle / picker / default；
bindings[].match_rules：数组；
```

配置校验失败分两类：

```text
fatal：无法加载整体配置，例如 JSON 格式错误、version 不支持；
binding-level：某个绑定无效，例如快捷键冲突、desktop_id 不存在。
```

binding-level 错误不应导致 Agent 完全不可用，应禁用问题绑定并继续运行。

### 9.5 默认配置生成

首次启动时生成最小可用配置，而不是猜测用户安装了哪些应用。

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
    "include_all_workspaces": true,
    "switch_workspace_when_needed": true,
    "launch_timeout_ms": 8000
  },
  "bindings": []
}
```

Settings UI 可以提供“一键导入 Dock 顺序”或“推荐绑定”，但不应在 Agent 首次启动时自动写入应用绑定。

### 9.6 配置兼容策略

```text
读取旧版本：自动迁移到当前内存结构；
保存配置：写当前最新 version；
未知字段：保留或忽略。MVP 推荐忽略但不删除，避免破坏未来字段；
未知 enum：使用默认值并产生 warning；
重复 binding id：保留第一个，后续标记为 invalid。
```

如果用户手动编辑配置，Settings UI 下次打开应显示校验问题，而不是直接覆盖文件。

---

## 10. 数据结构草案

### 10.1 Binding

```cpp
struct Binding {
    std::string id;
    bool enabled;
    Hotkey hotkey;
    std::string desktopId;
    MultiWindowStrategy strategy;
    std::vector<MatchRule> matchRules;
};
```

### 10.2 MatchRule

```cpp
enum class MatchType {
    WmClass,
    ProcessName,
    WindowTitle,
    DesktopId,
    Pid,
    Regex
};

enum class MatchOperator {
    Equals,
    EqualsIgnoreCase,
    Contains,
    ContainsIgnoreCase,
    Regex
};

struct MatchRule {
    MatchType type;
    MatchOperator op;
    std::string value;
    int weight;
};
```

### 10.3 BackendStatus

```cpp
struct BackendStatus {
    std::string sessionType;       // x11 / wayland / unknown
    std::string hotkeyBackend;     // x11 / portal / none
    std::string windowBackend;     // x11 / treeland / none
    bool globalHotkeyAvailable;
    bool windowListAvailable;
    bool activateWindowAvailable;
    std::vector<std::string> warnings;
};
```

### 10.4 Config

```cpp
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
    Hotkey leaderKey;
    int leaderTimeoutMs = 1500;
};

struct WindowConfig {
    MultiWindowStrategy defaultStrategy = MultiWindowStrategy::Cycle;
    int cycleTimeoutMs = 1200;
    int launchTimeoutMs = 8000;
    bool includeAllWorkspaces = true;
    bool switchWorkspaceWhenNeeded = true;
};

struct Config {
    int version = 1;
    GeneralConfig general;
    HotkeyConfig hotkey;
    WindowConfig window;
    std::vector<Binding> bindings;
};
```

### 10.5 AppTarget

```cpp
struct AppTarget {
    QString actionId;
    QString appId;
    QString displayName;
    QString desktopId;
    QString exec;
    QString icon;
    QString startupWmClass;
    QList<MatchRule> matchRules;
    bool launchIfNotRunning;
    bool focusExistingWindow;
    MultiWindowStrategy strategy;
};
```

AppTarget 是运行时对象，不直接序列化。它由 Binding 和 AppInfo 合成。

### 10.6 Result 类型约定

跨模块方法不要只返回 bool。推荐所有可能失败的方法返回 Result。

```cpp
template <typename T>
struct Result {
    bool ok;
    T value;
    QString errorCode;
    QString message;
};
```

C++ 实现中也可以不用模板，按模块定义具体 Result。关键是保留：

```text
ok：机器判断；
errorCode：稳定错误码；
message：可读说明；
diagnostics：日志诊断信息。
```

---

## 11. X11 MVP 实现细节

### 11.1 会话检测

启动时读取环境变量：

```text
XDG_SESSION_TYPE
WAYLAND_DISPLAY
DISPLAY
```

判断逻辑：

```text
if XDG_SESSION_TYPE == "x11" and DISPLAY exists:
    use X11 backend
else if XDG_SESSION_TYPE == "wayland":
    use limited Wayland backend or show unsupported warning
else:
    show unknown session warning
```

建议同时记录原始环境：

```text
XDG_CURRENT_DESKTOP
DESKTOP_SESSION
XDG_SESSION_DESKTOP
QT_QPA_PLATFORM
```

这些字段不参与核心判断，但对 deepin v25 问题定位很有用。

---

### 11.2 快捷键注册

X11 下使用 passive grab。

需要注意：

```text
NumLock
CapsLock
ScrollLock
不同键盘布局
小键盘数字键
```

注册快捷键时建议同时注册修饰键变体：

```text
原始 modifiers
modifiers + NumLock
modifiers + CapsLock
modifiers + NumLock + CapsLock
```

还应考虑：

```text
modifiers + ScrollLock
modifiers + NumLock + ScrollLock
modifiers + CapsLock + ScrollLock
modifiers + NumLock + CapsLock + ScrollLock
```

注册流程：

```text
1. 解析 keysym；
2. 根据当前键盘映射找到 keycode；
3. 为 lock modifier 生成多个 modifier mask；
4. 对每个变体调用 grab；
5. 任一核心变体 BadAccess 即认为冲突；
6. 成功后保存 actionId -> keycode/modifier 映射。
```

X11 错误处理：

```text
BadAccess：快捷键已被占用；
BadValue：keycode 或 modifier 无效；
BadWindow：root window 异常，通常是连接问题。
```

实现时需要安装临时 X error handler 捕获 grab 错误，否则 XGrabKey 可能不会通过返回值直接表达失败。

---

### 11.3 窗口枚举

优先读取：

```text
_NET_CLIENT_LIST_STACKING
```

如果失败，再读：

```text
_NET_CLIENT_LIST
```

然后逐个窗口读取：

```text
WM_CLASS
_NET_WM_NAME
WM_NAME
_NET_WM_PID
_NET_WM_DESKTOP
_NET_WM_WINDOW_TYPE
WM_STATE
```

过滤掉：

```text
Dock
Desktop
Notification
Splash
Tooltip
Menu
Utility 可选过滤
```

属性读取优先级：

```text
标题：
1. _NET_WM_NAME UTF8_STRING
2. WM_NAME

类名：
1. WM_CLASS，拆成 instance 和 class

PID：
1. _NET_WM_PID
2. 无 PID 时留空，不强制通过 /proc 反查

工作区：
1. _NET_WM_DESKTOP
2. 0xFFFFFFFF 表示 sticky/all desktops
```

窗口类型：

```text
读取 _NET_WM_WINDOW_TYPE；
没有类型时按 normal 候选处理；
多个类型时只要包含 dock/desktop/notification 等排除类型即可排除。
```

事件监听：

```text
root window：
- PropertyChangeMask：监听 _NET_CLIENT_LIST、_NET_ACTIVE_WINDOW；
- SubstructureNotifyMask：辅助发现窗口变化。

client window：
- PropertyChangeMask：监听标题、WM_CLASS、WM_STATE 变化。
```

MVP 如果事件监听复杂，可以先在快捷键触发时即时枚举；但 M1 应补 activeWindowChanged，支持最近使用顺序。

---

### 11.4 激活窗口

激活窗口步骤：

```text
1. 如果窗口最小化，先恢复；
2. 如果窗口不在当前工作区，根据配置决定是否切换工作区；
3. 发送 _NET_ACTIVE_WINDOW 消息；
4. 可选：XRaiseWindow；
5. 验证 _NET_ACTIVE_WINDOW 是否变成目标窗口。
```

ClientMessage 建议字段：

```text
message_type = _NET_ACTIVE_WINDOW
format = 32
data.l[0] = 2              // source indication: pager/application
data.l[1] = timestamp      // 当前事件时间，无法获取时用 CurrentTime 兜底
data.l[2] = active_window  // 当前活动窗口，可为 0
```

恢复最小化：

```text
如果 WM_STATE 显示 IconicState，调用 XMapWindow；
也可发送 _NET_WM_STATE_REMOVE + _NET_WM_STATE_HIDDEN，但兼容性需测试；
MVP 先使用 XMapWindow + _NET_ACTIVE_WINDOW。
```

跨工作区：

```text
如果 window.desktop != currentDesktop：
  switch_workspace_when_needed=true：发送 _NET_CURRENT_DESKTOP，再激活窗口；
  false：显示提示“窗口在其他工作区”，不强制切换。
```

验证：

```text
发送激活消息后等待 100-300ms；
读取 _NET_ACTIVE_WINDOW；
如果不是目标窗口，返回 FailedActivateWindow，但不要重复 spam 激活。
```

多显示器一般不需要特殊处理，窗口管理器会处理焦点和显示器切换；如果 deepin WM 行为异常，再在后续版本补显示器感知逻辑。

### 11.5 X11 后端测试夹具

单元测试不应依赖真实 X11。建议拆出纯逻辑：

```text
HotkeyParserTest：字符串到 key/modifier；
ModifierVariantTest：NumLock/CapsLock/ScrollLock 组合；
WindowPropertyParserTest：模拟 EWMH 属性解析；
AppMatcherTest：WindowInfo + AppInfo 评分。
```

真实 X11 集成测试放手动或 CI 可选环境：

```text
Xvfb：可测试窗口创建和基础属性；
真实 deepin X11：测试 grab、激活、工作区、最小化。
```

---

## 12. Wayland / Treeland 适配路线

Wayland 下不能照搬 X11 的窗口枚举和激活方式。必须通过 compositor、portal 或桌面环境提供的接口。

### 12.1 分阶段目标

#### 阶段 A：Wayland 下有限可用

只提供：

```text
全局快捷键触发
启动应用
显示提示
```

暂不保证：

```text
枚举所有窗口
激活任意已有窗口
多窗口循环
```

#### 阶段 B：接入 XDG Desktop Portal Global Shortcuts

目标：

```text
Wayland 下可注册全局快捷键。
```

#### 阶段 C：研究 Treeland / DDE 私有接口

目标：

```text
获取窗口列表
激活窗口
匹配 app_id
切换工作区
```

需要调研：

```text
Treeland 是否暴露窗口管理 D-Bus 接口
DDE 是否已有窗口切换相关接口
是否可通过 KWin-like / wlroots-like 协议扩展实现
是否需要 deepin 官方配合
```

### 12.2 后端设计

```text
X11HotkeyBackend
PortalHotkeyBackend
TreelandHotkeyBackend

X11WindowBackend
TreelandWindowBackend
LimitedWaylandWindowBackend
```

### 12.3 Wayland 降级策略

如果当前后端不支持窗口激活，则行为变为：

```text
应用未运行：启动
应用已运行：尝试通过 desktop portal / DDE 接口激活
无法激活：显示提示，引导用户切换到 X11 会话或开启对应权限
```

---

## 13. 开机自启设计

### 13.1 用户级 autostart

创建：

```text
~/.config/autostart/deepswitch-agent.desktop
```

内容：

```ini
[Desktop Entry]
Type=Application
Name=DeepSwitch Agent
Exec=/usr/bin/deepswitch-agent
X-GNOME-Autostart-enabled=true
NoDisplay=true
```

### 13.2 systemd user service 可选

也可支持：

```text
~/.config/systemd/user/deepswitch-agent.service
```

但 MVP 使用 autostart 更简单。

---

## 14. 打包方案

### 14.1 Debian 包

第一优先级：deb。

安装路径：

```text
/usr/bin/deepswitch-agent
/usr/bin/deepswitch-settings
/usr/share/applications/deepswitch-settings.desktop
/usr/share/icons/hicolor/.../apps/deepswitch.png
/usr/share/doc/deepswitch/...
```

配置和数据仍放用户目录。

### 14.2 玲珑包

后续如果要贴合 deepin 生态，可以考虑适配玲珑包。

### 14.3 AppImage

可作为便携版本，但桌面集成、全局快捷键、自启动处理可能更麻烦，不建议 MVP 优先。

---

## 15. 项目目录结构

```text
deepswitch/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── architecture.md
│   ├── x11-backend.md
│   ├── wayland-treeland.md
│   └── config.md
├── packaging/
│   ├── debian/
│   └── autostart/
├── src/
│   ├── agent/
│   │   ├── main.cpp
│   │   ├── Agent.cpp
│   │   └── Agent.h
│   ├── core/
│   │   ├── ActionEngine.cpp
│   │   ├── ConfigManager.cpp
│   │   ├── AppRegistry.cpp
│   │   ├── AppMatcher.cpp
│   │   └── Launcher.cpp
│   ├── backends/
│   │   ├── hotkey/
│   │   │   ├── IHotkeyBackend.h
│   │   │   ├── X11HotkeyBackend.cpp
│   │   │   └── PortalHotkeyBackend.cpp
│   │   └── window/
│   │       ├── IWindowBackend.h
│   │       ├── X11WindowBackend.cpp
│   │       └── TreelandWindowBackend.cpp
│   ├── ipc/
│   │   ├── AgentDBusService.cpp
│   │   └── AgentDBusService.h
│   ├── settings/
│   │   ├── main.cpp
│   │   ├── qml/
│   │   └── SettingsController.cpp
│   └── overlay/
│       ├── OverlayWindow.cpp
│       └── qml/
├── tests/
│   ├── test_app_registry.cpp
│   ├── test_match_rules.cpp
│   ├── test_config.cpp
│   └── test_action_engine.cpp
└── resources/
    ├── icons/
    └── translations/
```

---

## 16. MVP 范围

### 16.1 必须实现

```text
1. X11 会话检测；
2. 配置文件加载和保存；
3. 扫描 .desktop 应用；
4. 设置 Alt+数字 快捷键；
5. 注册 X11 全局快捷键；
6. 根据快捷键启动应用；
7. 枚举 X11 窗口；
8. 根据 WM_CLASS / StartupWMClass 匹配应用窗口；
9. 激活已存在窗口；
10. 同一应用多个窗口循环；
11. 设置界面添加/删除/修改绑定；
12. 开机自启；
13. 基础日志；
14. deb 打包。
```

### 16.2 可以延后

```text
1. Wayland/Treeland 完整支持；
2. 窗口选择器；
3. 动画效果；
4. 插件系统；
5. 云同步；
6. 复杂主题；
7. 多语言完整翻译；
8. Flatpak/玲珑完整支持。
```

---

## 17. 开发里程碑

### M0：技术验证

目标：证明 X11 下核心链路可行。

任务：

```text
1. 注册 Alt+1；
2. 捕获快捷键事件；
3. 枚举窗口；
4. 找到 Firefox 窗口；
5. 激活 Firefox；
6. 如果 Firefox 不存在则启动 Firefox。
```

验收标准：

```text
在 deepin v25 X11 会话下，Alt+1 可以启动或切换 Firefox。
```

交付物：

```text
src/prototype/x11_switch_demo.cpp
docs/m0-x11-findings.md
```

细化任务：

```text
M0.1 建立最小 CMake 工程；
M0.2 打开 X11 display/connection；
M0.3 解析并注册 Alt+1；
M0.4 捕获 KeyPress 并打印日志；
M0.5 读取 _NET_CLIENT_LIST_STACKING；
M0.6 读取窗口 WM_CLASS / title / pid；
M0.7 通过硬编码 firefox WM_CLASS 找窗口；
M0.8 发送 _NET_ACTIVE_WINDOW；
M0.9 无窗口时通过 QProcess 启动 firefox；
M0.10 记录 deepin v25 X11 上的实际限制。
```

不在 M0 做：

```text
设置界面；
配置文件；
通用 .desktop 解析；
多窗口完整策略；
Wayland 支持。
```

---

### M1：命令行原型

目标：无 UI 版本可用。

任务：

```text
1. 支持 config.json；
2. 支持多个快捷键绑定；
3. 支持 .desktop 解析；
4. 支持应用启动；
5. 支持窗口匹配；
6. 支持多窗口循环；
7. 支持日志。
```

验收标准：

```text
通过手写配置，可以绑定 5 个常用应用并稳定切换。
```

细化任务：

```text
M1.1 建立 deepswitch_core target；
M1.2 实现 ConfigManager load/save/validate；
M1.3 实现 HotkeyParser；
M1.4 实现 X11HotkeyBackend；
M1.5 实现 X11WindowBackend list/activate；
M1.6 实现 AppRegistry 扫描前三个 applications 目录；
M1.7 实现 Exec field code 清理；
M1.8 实现 AppMatcher 分数模型；
M1.9 实现 ActionEngine launch/focus/cycle；
M1.10 实现基础 Overlay 或命令行提示；
M1.11 实现日志；
M1.12 编写单元测试。
```

M1 命令行参数：

```text
deepswitch-agent --config /path/to/config.json
deepswitch-agent --validate-config
deepswitch-agent --list-apps
deepswitch-agent --list-windows
deepswitch-agent --trigger app.firefox
```

M1 验收补充：

```text
配置 reload 不崩溃；
快捷键冲突能显示具体绑定；
NumLock/CapsLock 状态不影响 Alt+数字；
Firefox、Terminal、VS Code 至少三个应用可稳定切换。
```

---

### M2：设置界面

目标：普通用户无需手写配置。

任务：

```text
1. 应用列表；
2. 快捷键录制；
3. 绑定管理；
4. 多窗口策略选择；
5. 后端状态显示；
6. 保存配置并通知 Agent reload。
```

验收标准：

```text
用户可以通过界面完成 Firefox、Terminal、VS Code 的快捷键绑定。
```

细化任务：

```text
M2.1 实现 QtDBus service；
M2.2 实现 Settings DBus client；
M2.3 实现绑定列表页；
M2.4 实现应用选择器和搜索；
M2.5 实现快捷键录制；
M2.6 实现多窗口策略选择；
M2.7 实现后端状态页；
M2.8 实现配置保存和 Agent 热更新；
M2.9 实现应用未找到/快捷键冲突等 UI 状态；
M2.10 补充基础 QML 组件和图标加载。
```

设置界面 MVP 不做：

```text
复杂主题编辑；
窗口选择器；
云同步；
插件管理；
跨机器导入导出高级规则。
```

---

### M3：体验完善

目标：达到日常使用质量。

任务：

```text
1. 开机自启；
2. 托盘或状态入口；
3. 快捷键冲突提示；
4. Overlay 提示；
5. 更稳定的窗口匹配；
6. deb 安装包；
7. 崩溃恢复和错误提示。
```

验收标准：

```text
安装后开机自动可用，设置界面关闭后快捷键仍然工作。
```

细化任务：

```text
M3.1 实现用户级 autostart 开关；
M3.2 实现托盘或 DDE 可见入口；
M3.3 实现 Overlay 轻提示；
M3.4 完善启动后窗口追踪；
M3.5 完善匹配诊断和用户修正规则；
M3.6 实现日志文件大小限制；
M3.7 实现 deb packaging；
M3.8 安装后 desktop 文件和图标；
M3.9 卸载后不删除用户配置；
M3.10 补齐手动测试矩阵。
```

体验指标：

```text
快捷键触发到动作决策：通常 < 50ms；
已有窗口聚焦：通常 < 200ms；
启动应用：立即显示提示，不等待进程完成；
设置保存后生效：< 500ms。
```

---

### M4：Wayland/Treeland 调研版

目标：明确 Wayland/Treeland 能力边界。

任务：

```text
1. 检测 Wayland/Treeland 会话；
2. 尝试接入 Global Shortcuts Portal；
3. 调研 DDE/Treeland 窗口接口；
4. 实现有限能力后端；
5. 在设置界面展示能力差异。
```

验收标准：

```text
Wayland/Treeland 下至少可以完成快捷键触发和启动应用；窗口激活能力根据实际接口决定。
```

细化任务：

```text
M4.1 收集 deepin v25 X11/Wayland/Treeland 会话环境变量；
M4.2 验证 xdg-desktop-portal GlobalShortcuts 在 deepin 上是否可用；
M4.3 调研 Treeland/DDE 是否暴露窗口列表和激活接口；
M4.4 实现 LimitedWaylandWindowBackend；
M4.5 Settings UI 显示 Wayland 降级能力；
M4.6 形成 docs/wayland-treeland-findings.md。
```

---

## 18. 测试方案

### 18.1 单元测试

重点测试：

```text
配置解析
.desktop 解析
Exec 字段清理
匹配规则
多窗口循环逻辑
快捷键字符串解析
```

### 18.2 集成测试

场景：

```text
应用未启动 -> 启动
应用已启动一个窗口 -> 聚焦
应用已启动多个窗口 -> 循环
窗口最小化 -> 恢复并聚焦
目标应用在其他工作区 -> 切换并聚焦
快捷键冲突 -> 提示失败
配置变更 -> Agent reload
```

### 18.3 手动测试矩阵

应用：

```text
Firefox / Chrome / deepin-browser
Deepin Terminal
VS Code
Deepin File Manager
微信 / QQ / Electron 应用
LibreOffice
```

环境：

```text
deepin v25 X11
多显示器
多个工作区
NumLock 开启/关闭
中文输入法开启/关闭
```

### 18.4 单元测试清单

ConfigManager：

```text
缺失配置文件 -> 生成默认配置；
JSON 损坏 -> 备份并加载默认；
重复 binding id -> 校验失败；
未知 enum -> 使用默认值并产生 warning；
保存配置 -> 原子写入。
```

HotkeyParser：

```text
Alt+1 / alt + 1 / ALT+1 规范化一致；
Ctrl/Control 别名一致；
Super/Meta/Win 别名一致；
只有 Alt 判定为无效；
Leader 键与 binding 冲突能检测。
```

AppRegistry：

```text
解析 Name[zh_CN]；
过滤 Hidden=true；
默认过滤 NoDisplay=true；
Exec field code 清理；
同名 desktop_id 目录优先级。
```

AppMatcher：

```text
StartupWMClass 命中；
WM_CLASS 大小写忽略；
Exec 主命令匹配进程名；
用户 include 规则加分；
用户 exclude 规则排除；
分数低于阈值不匹配。
```

ActionEngine：

```text
无窗口 -> launch；
一个窗口 -> focus；
多个窗口 recent -> 最近窗口；
多个窗口 cycle -> 连续触发循环；
PendingLaunch 中再次触发 -> 不重复启动；
后端不可用 -> 返回明确错误。
```

### 18.5 集成测试脚本建议

可以提供脚本辅助人工测试：

```text
scripts/manual/x11-print-windows
scripts/manual/x11-test-grab Alt+1
scripts/manual/x11-activate-window <window-id>
scripts/manual/validate-config <config-path>
scripts/manual/list-apps
```

这些脚本不一定进入最终安装包，但对 M0-M2 排查问题很有价值。

### 18.6 回归测试要求

每次修改以下模块都必须跑对应测试：

```text
ConfigManager：配置解析和迁移测试；
HotkeyParser/HotkeyBackend：快捷键解析测试，真实 X11 下至少手测一次；
AppMatcher：匹配规则测试；
ActionEngine：动作决策测试；
DBusService：方法参数序列化测试；
Settings UI：至少手动验证保存绑定后 Agent 生效。
```

---

## 19. 风险分析

### 19.1 X11 与 Wayland 能力差异

风险：

```text
X11 下能做的窗口控制，Wayland 下不一定允许。
```

应对：

```text
后端抽象；
第一版明确 X11-first；
Wayland 下显示能力状态和降级提示。
```

### 19.2 App 与 Window 匹配不准确

风险：

```text
某些应用的 .desktop、进程名、WM_CLASS 不一致。
```

应对：

```text
使用多规则评分；
提供用户手动修正规则；
内置常见应用规则库。
```

### 19.3 快捷键冲突

风险：

```text
系统、DDE、应用都可能占用快捷键。
```

应对：

```text
注册失败时提示；
提供 Leader 模式；
推荐默认快捷键避开系统常用组合。
```

### 19.4 deepin v25 系统机制变化

风险：

```text
deepin v25 的 Treeland、DDE 接口、系统只读机制可能变化。
```

应对：

```text
避免依赖私有系统修改；
优先用户级配置；
将 Treeland 支持放到独立后端；
跟踪官方接口变化。
```

### 19.5 窗口激活策略受窗口管理器限制

风险：

```text
某些窗口管理器可能拒绝外部激活窗口。
```

应对：

```text
遵循 EWMH；
激活后验证状态；
失败时给用户提示；
保留 fallback 行为。
```

---

## 20. 验收标准

### 20.1 MVP 验收

在 deepin v25 X11 会话下：

```text
1. 用户可以打开设置界面；
2. 用户可以选择应用并绑定快捷键；
3. 按快捷键可以启动未运行应用；
4. 按快捷键可以切换到已运行应用；
5. 同一个应用多个窗口时可以循环；
6. 配置重启后仍然保留；
7. 系统重启登录后 Agent 自动运行；
8. 快捷键冲突有明确提示；
9. 设置界面关闭后快捷键仍然生效；
10. deb 包可以安装和卸载。
```

### 20.2 体验验收

用户主观体验应满足：

```text
按键响应无明显延迟；
常用应用切换稳定；
无需频繁看窗口列表；
配置过程不需要写命令；
出错时能看懂原因。
```

---

## 21. 第一版推荐默认配置

```text
Alt + 1  -> 浏览器
Alt + 2  -> 终端
Alt + 3  -> 编辑器
Alt + 4  -> 文件管理器
Alt + 5  -> 聊天工具
Alt + 6  -> 邮件/笔记
```

Leader 模式默认：

```text
Alt + Space -> 1  浏览器
Alt + Space -> 2  终端
Alt + Space -> 3  编辑器
Alt + Space -> 4  文件管理器
```

多窗口默认策略：

```text
连续按同一快捷键时循环，否则切换到最近使用窗口。
```

---

## 22. 后续可扩展功能

### 22.1 窗口选择器

用于目标应用有多个窗口时快速选择。

### 22.2 Dock 顺序导入

自动读取用户 Dock 上固定应用的顺序，生成 Alt+1、Alt+2、Alt+3 绑定。

### 22.3 应用规则库

内置常见应用匹配规则：

```text
Firefox
Chrome / Chromium
VS Code
Deepin Terminal
Deepin File Manager
WPS
LibreOffice
微信
QQ
Telegram
Obsidian
```

### 22.4 配置导入导出

方便用户备份和迁移配置。

### 22.5 快捷键 Cheat Sheet

按住 Leader 键时展示当前绑定表。

### 22.6 插件系统

暂不建议早期实现。等核心稳定后再考虑。

---

## 23. 实施建议

建议按以下顺序推进：

```text
1. 先写 X11 技术验证 Demo。
2. 再封装 HotkeyBackend 和 WindowBackend。
3. 然后实现配置文件和 ActionEngine。
4. 接着做无 UI 的 Agent。
5. 再做 Settings UI。
6. 最后做 Overlay 和打包。
```

不要一开始就做漂亮 UI。这个项目真正的难点是：

```text
窗口匹配准确性
快捷键稳定性
多窗口切换行为
不同会话协议的能力边界
```

UI 可以后置，核心链路必须先跑通。

---

## 24. 最终推荐基线方案

推荐采用以下基线：

```text
项目形态：后台 Agent + 设置 UI + Overlay
目标平台：deepin v25
首发支持：X11
后续适配：Wayland/Treeland
语言框架：C++20 + Qt6/QML
窗口后端：X11/XCB/EWMH
快捷键后端：X11 passive grab
应用索引：解析 .desktop 文件
配置格式：JSON
IPC：D-Bus
打包：deb
```

第一版目标不是“功能最多”，而是“核心体验稳定”：

```text
按固定快捷键，快速启动或切换到目标应用。
```

只要这个体验做到稳定，后续再加窗口选择器、Treeland 支持、深度 DDE 集成，都会更有基础。

---

## 25. 实施任务分解建议

### 25.1 第一轮代码骨架

第一轮不要急于实现完整功能，先建立可测试的边界。

```text
1. 创建 CMake 工程和 target；
2. 定义 core 数据结构；
3. 实现 Result/Error 类型；
4. 实现 ConfigManager；
5. 实现 HotkeyParser；
6. 编写 Config 和 Hotkey 单元测试；
7. 加入最小 agent main；
8. 加入日志初始化。
```

建议第一轮结束时，即使还不能切换窗口，也应该能够：

```text
deepswitch-agent --validate-config
deepswitch-agent --list-bindings
```

### 25.2 第二轮 X11 核心链路

```text
1. 实现 X11Connection 封装；
2. 实现 X11HotkeyBackend；
3. 实现 X11WindowBackend listWindows；
4. 实现 X11WindowBackend activateWindow；
5. 实现 AppMatcher；
6. 实现 ActionEngine focus/launch/cycle；
7. 用硬编码配置验证 Alt+1。
```

第二轮完成后进入 M0/M1 验收。

### 25.3 第三轮应用与配置闭环

```text
1. 实现 AppRegistry；
2. 实现 Launcher；
3. 实现配置中的 bindings；
4. 实现配置 reload；
5. 实现 HotkeyManager diff 注册；
6. 实现 PendingLaunch 追踪；
7. 补充手写配置测试。
```

第三轮完成后，用户应能不改代码只改 config.json 来绑定应用。

### 25.4 第四轮 UI 与 IPC

```text
1. 定义 D-Bus 类型；
2. 实现 AgentDBusService；
3. 实现 SettingsController；
4. 实现绑定列表；
5. 实现应用选择器；
6. 实现快捷键录制；
7. 实现后端状态页；
8. 实现保存后热更新。
```

第四轮完成后，进入 M2 验收。

### 25.5 第五轮体验和发布

```text
1. 实现 Overlay 轻提示；
2. 实现 autostart；
3. 实现 deb packaging；
4. 补充日志轮转；
5. 完成手动测试矩阵；
6. 写 README 和故障排查文档；
7. 准备首个可试用包。
```

### 25.6 每个阶段的完成定义

每个阶段完成前都要满足：

```text
1. 对应单元测试通过；
2. deepin v25 X11 手动验证通过；
3. 日志里能看出关键动作链路；
4. Settings UI 或命令行能显示失败原因；
5. 新增配置字段有默认值和校验；
6. 文档中的验收标准已更新为实际结果。
```

---

## 26. 参考资料入口

以下资料适合实现阶段查阅：

- Manico 官方介绍：<https://manico.mariuti.com/>
- Desktop Entry 规范：<https://specifications.freedesktop.org/desktop-entry-spec/latest/>
- EWMH / NetWM 规范：<https://specifications.freedesktop.org/wm-spec/latest/>
- XCB grab key 文档：<https://www.x.org/archive/current/doc/man/man3/xcb_grab_key.3.xhtml>
- XDG Desktop Portal Global Shortcuts：<https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.GlobalShortcuts.html>
- Treeland 项目：<https://github.com/linuxdeepin/treeland>
- deepin 官网：<https://www.deepin.org/>
