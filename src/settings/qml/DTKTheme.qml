// Oops Jump — DTK-style theme constants
// Ported from dtkdeclarative FlowStyle.qml for visual consistency.
pragma Singleton

import QtQuick

QtObject {
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
}
