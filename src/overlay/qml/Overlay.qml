import QtQuick

Rectangle {
    id: root
    width: 360
    height: 84
    radius: 22
    color: "#e6202428"
    border.color: "#33ffffff"
    border.width: 1

    readonly property color accentColor: {
        if (overlayKind === "failed")
            return "#ff6464"
        if (overlayKind === "launched")
            return "#79d890"
        if (overlayKind === "cycled")
            return "#8cc8ff"
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
        width: 42
        height: 42
        radius: 21
        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        color: root.accentColor

        Text {
            anchors.centerIn: parent
            text: {
                if (overlayKind === "failed")
                    return "!"
                if (overlayKind === "launched")
                    return "+"
                if (overlayKind === "cycled")
                    return ">"
                return "*"
            }
            color: "#202428"
            font.pixelSize: 22
            font.bold: true
        }
    }

    Column {
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

    SequentialAnimation on opacity {
        running: true
        NumberAnimation { to: 1; duration: 90; easing.type: Easing.OutCubic }
        PauseAnimation { duration: 630 }
        NumberAnimation { to: 0; duration: 120; easing.type: Easing.InCubic }
    }
}
