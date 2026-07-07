// Oops Jump — DTK-style theme constants
// Ported from dtkdeclarative FlowStyle.qml for visual consistency.
pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    // ── Dark mode detection ──
    readonly property bool isDark: Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    // ── Global control defaults ──
    readonly property int radius: 8
    readonly property int spacing: 6
    readonly property int padding: 6
    readonly property int borderWidth: 1
    readonly property real focusBorderWidth: 2
    readonly property real focusBorderPaddings: 1

    // ── Brand / highlight colors (deepin default) ──
    readonly property color highlight: "#0081ff"
    readonly property color highlightLight: "#1a9aff"
    readonly property color highlightDark: "#006adb"
    readonly property color highlightedText: "#ffffff"

    // ── Button ──
    readonly property QtObject button: QtObject {
        readonly property int width: 140
        readonly property int height: 36
        readonly property int hPadding: 8
        readonly property int vPadding: 4
        readonly property int iconSize: 24

        readonly property color background1: "#f7f7f7"
        readonly property color background1Hovered: "#e1e1e1"
        readonly property color background1Pressed: "#bcc4d0"
        readonly property color background2: "#f0f0f0"
        readonly property color background2Hovered: "#d2d2d2"
        readonly property color background2Pressed: "#cdd6e0"

        readonly property color insideBorder: Qt.rgba(1, 1, 1, 0.1)
        readonly property color insideBorderHovered: Qt.rgba(1, 1, 1, 0.2)
        readonly property color insideBorderPressed: Qt.rgba(1, 1, 1, 0.03)
        readonly property color outsideBorder: Qt.rgba(0, 0, 0, 0.08)
        readonly property color outsideBorderHovered: Qt.rgba(0, 0, 0, 0.2)
        readonly property color outsideBorderPressed: "transparent"

        readonly property color textNormal: Qt.rgba(0, 0, 0, 0.7)
        readonly property color textHovered: Qt.rgba(0, 0, 0, 1)
        readonly property color textPressed: highlight

        // Highlighted (primary) button
        readonly property color highlightedBg: highlightLight
        readonly property color highlightedBgHovered: "#33a1ff"
        readonly property color highlightedBgPressed: highlightDark
        readonly property color highlightedText: "#ffffff"

        // Checked button
        readonly property color checkedBg: highlight
        readonly property color checkedBgHovered: highlightLight
        readonly property color checkedBgPressed: highlightDark
        readonly property color checkedText: highlightedText
    }

    // ── Edit / TextField ──
    readonly property QtObject edit: QtObject {
        readonly property int width: 180
        readonly property int textFieldHeight: 36
        readonly property color background: Qt.rgba(0, 0, 0, 0.08)
        readonly property color backgroundFocused: Qt.rgba(0, 0, 0, 0.08)
        readonly property color alertBackground: Qt.rgba(0.95, 0.22, 0.20, 0.15)
        readonly property color placeholderText: Qt.rgba(0, 0, 0, 0.4)
    }

    // ── CheckBox ──
    readonly property QtObject checkBox: QtObject {
        readonly property int indicatorWidth: 16
        readonly property int indicatorHeight: 16
        readonly property int padding: 2
        readonly property int iconSize: 16
        readonly property int focusRadius: 4
    }

    // ── ComboBox ──
    readonly property QtObject comboBox: QtObject {
        readonly property int width: 240
        readonly property int height: 36
        readonly property int padding: 8
        readonly property int spacing: 10
        readonly property int iconSize: 12
        readonly property int maxVisibleItems: 16
    }

    // ── Switch ──
    readonly property QtObject switchButton: QtObject {
        readonly property int indicatorWidth: 50
        readonly property int indicatorHeight: 24
        readonly property int handleWidth: 30
        readonly property int handleHeight: 24
        readonly property color background: Qt.rgba(50 / 255, 50 / 255, 50 / 255, 0.2)
        readonly property color handle: "#8c8c8c"
    }

    // ── Popup / Dialog ──
    readonly property QtObject popup: QtObject {
        readonly property int radius: 18
        readonly property int padding: 10
        readonly property int margin: 10
    }

    readonly property QtObject dialogWindow: QtObject {
        readonly property int titleBarHeight: 50
        readonly property int iconSize: 32
        readonly property int contentHMargin: 10
        readonly property int contentVMargin: 10
        readonly property int footerMargin: 10
    }

    // ── FloatingPanel (shared popup/menu background) ──
    readonly property QtObject floatingPanel: QtObject {
        readonly property int radius: 14
        readonly property color background: Qt.rgba(235 / 255, 235 / 255, 235 / 255, 0.6)
        readonly property color backgroundNoBlur: Qt.rgba(235 / 255, 235 / 255, 235 / 255, 1.0)
        readonly property color outsideBorder: Qt.rgba(0, 0, 0, 0.05)
        readonly property color insideBorder: Qt.rgba(1, 1, 1, 0.05)
        readonly property color dropShadow: Qt.rgba(0, 0, 0, 0.2)
    }

    // ── Menu ──
    readonly property QtObject menu: QtObject {
        readonly property int padding: 6
        readonly property int topPadding: 8
        readonly property int radius: 12
        readonly property int margins: 10
        readonly property int itemHeight: 30
        readonly property int itemRadius: 6
        readonly property color background: Qt.rgba(235 / 255, 235 / 255, 235 / 255, 0.6)
        readonly property color backgroundNoBlur: Qt.rgba(235 / 255, 235 / 255, 235 / 255, 1.0)
        readonly property color itemText: "black"
    }

    // ── BehindWindowBlur ──
    readonly property QtObject behindWindowBlur: QtObject {
        readonly property color lightColor: Qt.rgba(235 / 255, 235 / 255, 235 / 255, 0.6)
        readonly property color lightNoBlurColor: Qt.rgba(235 / 255, 235 / 255, 235 / 255, 1.0)
        readonly property color darkColor: "#55000000"
        readonly property color darkNoBlurColor: Qt.rgba(35 / 255, 35 / 255, 35 / 255, 1.0)
    }

    // ── Theme-aware colors (light / dark) ──
    readonly property color windowBackground: isDark ? "#1a1a1a" : "#f5f6f8"
    readonly property color titleBarBackground: isDark ? "#252525" : "#ffffff"
    readonly property color cardBackground: isDark ? "#2a2a2a" : "#fafbfc"
    readonly property color cardBorder: isDark ? "#3a3a3a" : "#e8e8e8"
    readonly property color dialogBackground: isDark ? "#2d2d2d" : "#ffffff"
    readonly property color dialogBorder: isDark ? "#3d3d3d" : "#d7dce0"
    readonly property color separator: isDark ? "#3a3a3a" : "#f0f0f0"
    readonly property color footerBackground: isDark ? "#222222" : "#fafbfc"

    readonly property color textPrimary: isDark ? "#e0e0e0" : "#1a1a1a"
    readonly property color textSecondary: isDark ? "#a0a0a0" : "#888888"
    readonly property color textMuted: isDark ? "#808080" : "#999999"
    readonly property color textDisabled: isDark ? "#606060" : "#bbbbbb"

    readonly property color inputBackground: isDark ? "#353535" : "#ffffff"
    readonly property color inputBorder: isDark ? "#4a4a4a" : "#c8d7de"
    readonly property color inputText: isDark ? "#e0e0e0" : "#17313c"
    readonly property color inputPlaceholder: isDark ? "#707070" : "#8aa0aa"

    readonly property color hotkeyBackground: isDark ? "#353535" : "#f0f0f0"
    readonly property color hotkeyText: isDark ? "#c0c0c0" : "#555555"

    readonly property color buttonBackground: isDark ? "#353535" : "#ffffff"
    readonly property color buttonBackgroundHovered: isDark ? "#404040" : "#f5f5f5"
    readonly property color buttonBorder: isDark ? "#4a4a4a" : "#dddddd"
    readonly property color buttonBorderHovered: isDark ? "#5a5a5a" : "#cccccc"
    readonly property color buttonText: isDark ? "#c0c0c0" : "#666666"
    readonly property color buttonTextHovered: isDark ? "#e0e0e0" : "#00857a"

    readonly property color primaryButtonBackground: isDark ? "#007060" : "#00857a"
    readonly property color primaryButtonBackgroundHovered: isDark ? "#00857a" : "#00695c"
    readonly property color primaryButtonText: isDark ? "#ffffff" : "#ffffff"

    readonly property color accentText: isDark ? "#4db8ff" : "#00857a"
    readonly property color accentTextHovered: isDark ? "#6cc7ff" : "#00695c"

    readonly property color errorBackground: isDark ? "#3d1a1a" : "#fbe9e7"
    readonly property color errorBorder: isDark ? "#5a2a2a" : "#e2b199"
    readonly property color errorText: isDark ? "#fca5a5" : "#dc2626"
    readonly property color errorBg: isDark ? "#450a0a" : "#fee2e2"

    readonly property color successText: isDark ? "#66bb6a" : "#2e7d32"
    readonly property color warningText: isDark ? "#ffb74d" : "#e65100"

    readonly property color warnBackground: isDark ? "#3d2a10" : "#fff3e0"
    readonly property color warnBorder: isDark ? "#6a4a20" : "#e6a04f"
    readonly property color warnText: isDark ? "#fbbf24" : "#b45309"
    readonly property color warnBg: isDark ? "#451a03" : "#fef3c7"

    readonly property color debugCardBackground: isDark ? "#2a2a2a" : "#ffffff"
    readonly property color debugCardBorder: isDark ? "#3a3a3a" : "#eeeeee"

    readonly property color iconLetterBackground: isDark ? "#1a3a35" : "#e0f2f1"
    readonly property color iconLetterText: isDark ? "#4db8a0" : "#00695c"

    readonly property color statusConnectedBg: isDark ? "#1b3d1b" : "#e8f5e9"
    readonly property color statusDisconnectedBg: isDark ? "#3d1b1b" : "#fbe9e7"

    readonly property color tabActive: isDark ? "#4db8ff" : "#00857a"
    readonly property color tabInactive: isDark ? "#808080" : "#888888"
    readonly property color tabIndicator: isDark ? "#4db8ff" : "#00857a"

    readonly property color recordingBackground: isDark ? "#3d3020" : "#fff8e8"
    readonly property color recordingBorder: isDark ? "#8a6a30" : "#d5a94f"
    readonly property color recordingText: isDark ? "#ffcc00" : "#c62828"

    readonly property color conflictBorder: isDark ? "#8a3a2a" : "#d98d72"
    readonly property color conflictText: isDark ? "#ff8a65" : "#93421e"
}
