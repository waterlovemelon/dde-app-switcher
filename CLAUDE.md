# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

DeepSwitch — keyboard-first app switcher for deepin v25, inspired by macOS Manico. One hotkey → one app. Press hotkey to launch (if not running) or focus (if running). Multi-window cycling supported.

**Status:** Pre-implementation. Repository contains design docs only — no source code, build files, or tests yet.

## Tech Stack (Planned)

- C++20, CMake 3.20+, Qt 6 (Core, Gui, Test, QML), X11/Xlib/XCB, QtDBus
- Test framework: Qt Test (`Qt6::Test`)
- Logging: Qt logging categories (`qCDebug`, `qCInfo`, etc.)

## Build (When Implemented)

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ctest --test-dir build
```

## Architecture

Core principle: **platform-agnostic core logic isolated from display-protocol backends.**

```
src/
├── agent/          # CLI daemon entrypoint (main.cpp, wires everything)
├── core/           # Pure logic — NO X11/Wayland includes
│   ├── Config, ConfigManager   # JSON config load/save
│   ├── Hotkey                  # Key sequence parsing (no X11)
│   ├── DesktopEntry            # .desktop file parsing
│   ├── AppRegistry             # App discovery from .desktop files
│   ├── AppMatcher              # Match X11 windows → apps by rules
│   ├── ActionEngine            # Decide: launch / focus / cycle / picker
│   └── Launcher                # Execute launch/focus actions
├── backends/
│   └── x11/        # X11HotkeyBackend, X11WindowBackend, X11Connection
├── ipc/            # D-Bus types and service wrappers (M2+)
├── settings/       # Qt/QML settings UI (M2+)
└── overlay/        # Window picker overlay UI (M3+)
tests/
├── core/           # Unit tests for pure core logic
└── fixtures/       # .desktop test fixtures
```

**Boundary rules:**
- `src/core/` must NOT include X11 headers
- `src/backends/x11/` is the ONLY place that includes `<X11/Xlib.h>`, `<X11/Xatom.h>`
- Tests target `deepswitch_core`, not the agent executable
- Backend interfaces (`HotkeyBackend`, `WindowBackend`) allow swapping X11 → Wayland/Treeland later

## Process Model

- `deepswitch-agent` — background daemon, owns core state, handles hotkeys
- `deepswitch-settings` — config UI, communicates with agent via D-Bus (M2+)
- Agent uses single-threaded Qt event loop; slow I/O (desktop scan, icon load) offloaded to workers

## Key Design Documents

- `deepin-manico-like-design.md` — full design spec (v0.2), ~3400 lines. Architecture, data structures, IPC schema, X11 details, Wayland roadmap, milestone definitions (M0–M4)
- `docs/superpowers/plans/2026-05-27-deepswitch-m0-m1-core.md` — M0/M1 task-by-task implementation plan with exact code to write

## Milestones

- **M0:** X11 technical validation (hotkey register, window enumerate, window activate)
- **M1:** Command-line prototype (config, .desktop parsing, window matching, multi-window cycling)
- **M2:** Settings UI (Qt/QML, D-Bus IPC)
- **M3:** Polish (autostart, overlay, deb packaging)
- **M4:** Wayland/Treeland research

## Runtime Paths

```
~/.config/deepswitch/config.json          # User config
~/.local/share/deepswitch/state.db        # State (optional, post-MVP)
~/.local/state/deepswitch/deepswitch.log  # Log
~/.config/autostart/deepswitch-agent.desktop  # Autostart
```

## Language

Design docs are written in Chinese (Simplified). Implementation code and comments should be in English.
