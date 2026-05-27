# M0 X11 Findings

## Environment

- `XDG_SESSION_TYPE`: (record from test machine)
- `DISPLAY`: (record from test machine)
- deepin version: Deepin 25 (crimson)

## Window Listing

- `./build/deepswitch-agent --list-windows` result: (record output)
- Firefox `WM_CLASS`: (record actual value)
- Deepin Terminal `WM_CLASS`: (record actual value)
- VS Code `WM_CLASS`: (record actual value)

## Window Activation

- `_NET_ACTIVE_WINDOW` activation result: (record whether focus changes)
- Minimized window behavior: (record whether activation restores the window)
- Cross-workspace behavior: (record whether activation switches workspace)

## Hotkey Registration

- `Alt+1` registration: (record success or failure)
- NumLock on: (record whether `Alt+1` still triggers)
- CapsLock on: (record whether `Alt+1` still triggers)
- Conflict behavior: (record what happens if the desktop already owns the shortcut)

## Limits Observed

- (Record concrete X11 or deepin behavior that affects implementation)
