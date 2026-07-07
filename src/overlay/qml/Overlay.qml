import QtQuick

Rectangle {
    id: root
    radius: overlayMode === "appbar" ? 16 : 22
    color: overlayMode === "appbar" ? "#e6202428" : "#e6202428"
    border.color: overlayMode === "appbar" ? Qt.rgba(1, 1, 1, 0.15) : "#33ffffff"
    border.width: 1
    focus: true

    signal appClicked(int index)
    signal hideRequested()

    // ── Keyboard support ──────────────────────────────────────────
    Keys.onEscapePressed: Qt.quit()
    Keys.onDigit1Pressed: { if (appEntries.length >= 1) root.appClicked(0) }
    Keys.onDigit2Pressed: { if (appEntries.length >= 2) root.appClicked(1) }
    Keys.onDigit3Pressed: { if (appEntries.length >= 3) root.appClicked(2) }
    Keys.onDigit4Pressed: { if (appEntries.length >= 4) root.appClicked(3) }
    Keys.onDigit5Pressed: { if (appEntries.length >= 5) root.appClicked(4) }
    Keys.onDigit6Pressed: { if (appEntries.length >= 6) root.appClicked(5) }
    Keys.onDigit7Pressed: { if (appEntries.length >= 7) root.appClicked(6) }
    Keys.onDigit8Pressed: { if (appEntries.length >= 8) root.appClicked(7) }
    Keys.onDigit9Pressed: { if (appEntries.length >= 9) root.appClicked(8) }

    // ── App Bar Mode ──────────────────────────────────────────────
    Row {
        id: barRow
        visible: overlayMode === "appbar"
        anchors.centerIn: parent
        spacing: 4

        Repeater {
            model: overlayMode === "appbar" ? appEntries : []

            delegate: Item {
                width: 104
                height: 104

                property bool isActive: modelData.active || false
                property bool isRunning: modelData.running || false
                property string appIcon: modelData.icon || ""
                property string appHotkey: modelData.hotkey || ""

                Rectangle {
                    id: slotBg
                    anchors.fill: parent
                    radius: 16
                    color: isActive ? Qt.rgba(1, 1, 1, 0.14) : "transparent"
                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                Rectangle {
                    anchors.fill: iconBox
                    radius: iconBox.radius
                    color: "transparent"
                    border.width: 2
                    border.color: isRunning ? Qt.rgba(0, 0.506, 1, 0.35) : "transparent"
                    visible: isRunning
                }

                Rectangle {
                    id: iconBox
                    width: 80
                    height: 80
                    radius: 20
                    anchors.centerIn: parent
                    color: Qt.rgba(0, 0, 0, 0.3)

                    Image {
                        anchors.centerIn: parent
                        width: 56
                        height: 56
                        source: "image://theme/" + appIcon
                        sourceSize: Qt.size(56, 56)
                        smooth: true
                    }
                }

                Rectangle {
                    width: Math.max(22, badgeText.implicitWidth + 10)
                    height: 22
                    radius: 11
                    anchors.top: iconBox.top
                    anchors.right: iconBox.right
                    anchors.topMargin: -5
                    anchors.rightMargin: -5
                    color: "#0081ff"

                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: appHotkey
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.weight: Font.Bold
                    }
                }

                Rectangle {
                    width: 22
                    height: 4
                    radius: 2
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: Qt.rgba(1, 1, 1, 0.6)
                    visible: isActive
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: { slotBg.color = Qt.rgba(1,1,1,0.1); parent.scale = 1.06; }
                    onExited: { slotBg.color = isActive ? Qt.rgba(1,1,1,0.14) : "transparent"; parent.scale = 1.0; }
                    onPressed: parent.scale = 0.94;
                    onReleased: parent.scale = containsMouse ? 1.06 : 1.0;
                    onClicked: root.appClicked(index)
                }

                Behavior on scale {
                    NumberAnimation { duration: 150; easing.type: Easing.OutBack; easing.overshoot: 1.5 }
                }
            }
        }
    }

    // ── Toast Mode (legacy) ──────────────────────────────────────
    readonly property color toastAccent: {
        if (overlayKind === "failed") return "#ff6464"
        if (overlayKind === "launched") return "#79d890"
        if (overlayKind === "cycled") return "#8cc8ff"
        return "#ffd166"
    }

    function kindLabel(kind) {
        if (kind === "failed") return qsTr("Failed")
        if (kind === "launched") return qsTr("Launched")
        if (kind === "cycled") return qsTr("Cycled")
        return qsTr("Focused")
    }

    Rectangle {
        id: glyph
        visible: overlayMode === "toast"
        width: 42
        height: 42
        radius: 21
        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        color: toastAccent

        Text {
            anchors.centerIn: parent
            text: {
                if (overlayKind === "failed") return "!"
                if (overlayKind === "launched") return "+"
                if (overlayKind === "cycled") return ">"
                return "*"
            }
            color: "#202428"
            font.pixelSize: 22
            font.bold: true
        }
    }

    Column {
        visible: overlayMode === "toast"
        anchors.left: glyph.right
        anchors.leftMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3

        Text {
            width: parent.width
            text: kindLabel(overlayKind)
            color: "#ffffff"
            font.pixelSize: 17
            font.bold: true
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: overlayMessage
            color: "#cfd6df"
            font.pixelSize: 13
            elide: Text.ElideRight
            visible: overlayMessage.length > 0
        }
    }

    opacity: 0
    Component.onCompleted: { fadeIn.start(); root.forceActiveFocus() }

    NumberAnimation {
        id: fadeIn
        target: root
        property: "opacity"
        to: 1.0
        duration: overlayMode === "appbar" ? 300 : 90
        easing.type: Easing.OutCubic
    }

    NumberAnimation {
        id: fadeOut
        target: root
        property: "opacity"
        to: 0.0
        duration: overlayMode === "appbar" ? 200 : 90
        easing.type: Easing.InCubic
        onStopped: root.hideRequested()
    }

    function hide() {
        fadeIn.stop()
        fadeOut.start()
    }
}
