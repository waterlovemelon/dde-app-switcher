import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: recorder

    property string text: ""
    property string placeholderText: qsTr("Alt+Enter")
    property bool recording: false
    property var controller: null
    property string excludeActionId: ""
    property string statusText: ""
    property bool conflict: false

    signal accepted(string hotkey)

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    function modifierName(key) {
        if (key === Qt.Key_Control) {
            return "Ctrl"
        }
        if (key === Qt.Key_Alt) {
            return "Alt"
        }
        if (key === Qt.Key_Shift) {
            return "Shift"
        }
        if (key === Qt.Key_Meta || key === Qt.Key_Super_L || key === Qt.Key_Super_R) {
            return "Meta"
        }
        return ""
    }

    function keyName(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            return "Enter"
        }
        if (event.key === Qt.Key_Escape) {
            return "Esc"
        }
        if (event.key === Qt.Key_Space) {
            return "Space"
        }
        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
            return "Tab"
        }
        if (event.key === Qt.Key_Backspace) {
            return "BackSpace"
        }
        if (event.key === Qt.Key_Delete) {
            return "Delete"
        }
        if (event.key === Qt.Key_Left) {
            return "Left"
        }
        if (event.key === Qt.Key_Right) {
            return "Right"
        }
        if (event.key === Qt.Key_Up) {
            return "Up"
        }
        if (event.key === Qt.Key_Down) {
            return "Down"
        }
        if (event.key === Qt.Key_Home) {
            return "Home"
        }
        if (event.key === Qt.Key_End) {
            return "End"
        }
        if (event.key === Qt.Key_PageUp) {
            return "Page_Up"
        }
        if (event.key === Qt.Key_PageDown) {
            return "Page_Down"
        }
        if (event.key === Qt.Key_Insert) {
            return "Insert"
        }
        if (event.key >= Qt.Key_F1 && event.key <= Qt.Key_F35) {
            return "F" + (event.key - Qt.Key_F1 + 1)
        }
        if (event.key >= Qt.Key_A && event.key <= Qt.Key_Z) {
            return String.fromCharCode("A".charCodeAt(0) + event.key - Qt.Key_A)
        }
        if (event.key >= Qt.Key_0 && event.key <= Qt.Key_9) {
            return String.fromCharCode("0".charCodeAt(0) + event.key - Qt.Key_0)
        }
        if (event.key === Qt.Key_Minus) {
            return "-"
        }
        if (event.key === Qt.Key_Equal) {
            return "="
        }
        if (event.key === Qt.Key_BracketLeft) {
            return "["
        }
        if (event.key === Qt.Key_BracketRight) {
            return "]"
        }
        if (event.key === Qt.Key_Backslash) {
            return "\\"
        }
        if (event.key === Qt.Key_Semicolon) {
            return ";"
        }
        if (event.key === Qt.Key_Apostrophe) {
            return "'"
        }
        if (event.key === Qt.Key_Comma) {
            return ","
        }
        if (event.key === Qt.Key_Period) {
            return "."
        }
        if (event.key === Qt.Key_Slash) {
            return "/"
        }
        if (event.key === Qt.Key_QuoteLeft) {
            return "`"
        }
        if (event.text && event.text.length === 1) {
            return event.text.toUpperCase()
        }
        return ""
    }

    function normalizedHotkey(event) {
        var key = keyName(event)
        if (key.length === 0 || modifierName(event.key).length > 0) {
            return ""
        }

        var parts = []
        if (event.modifiers & Qt.ControlModifier) {
            parts.push("Ctrl")
        }
        if (event.modifiers & Qt.AltModifier) {
            parts.push("Alt")
        }
        if (event.modifiers & Qt.ShiftModifier) {
            parts.push("Shift")
        }
        if (event.modifiers & Qt.MetaModifier) {
            parts.push("Meta")
        }
        parts.push(key)
        return parts.join("+")
    }

    function beginRecording() {
        statusText = qsTr("Press a key combination")
        conflict = false
        recording = true
        forceActiveFocus()
    }

    function testCurrentHotkey() {
        if (!controller || text.length === 0) {
            return
        }

        if (controller.testHotkey(text, excludeActionId)) {
            statusText = qsTr("Hotkey is available")
            conflict = false
            return
        }

        statusText = controller.lastError
        conflict = controller.lastErrorCode !== "hotkey_backend_unavailable"
    }

    Keys.onPressed: function(event) {
        if (!recording) {
            return
        }

        event.accepted = true
        if (event.isAutoRepeat) {
            return
        }

        var hotkey = normalizedHotkey(event)
        if (hotkey.length === 0) {
            statusText = qsTr("Press a non-modifier key")
            return
        }

        text = hotkey
        recording = false
        accepted(hotkey)
        testCurrentHotkey()
    }

    ColumnLayout {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        width: recorder.width
        spacing: 5

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 8
                color: recorder.recording ? "#fff8e8" : "#ffffff"
                border.width: 1
                border.color: recorder.conflict ? "#d98d72" : recorder.recording ? "#d5a94f" : "#c8d7de"

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    verticalAlignment: Text.AlignVCenter
                    text: recorder.text.length > 0 ? recorder.text : recorder.placeholderText
                    color: recorder.text.length > 0 ? "#17313c" : "#8aa0aa"
                    elide: Text.ElideRight
                    font.pixelSize: 14
                }
            }

            Button {
                text: recorder.recording ? qsTr("Listening") : qsTr("Record")
                onClicked: recorder.beginRecording()
            }
        }

        Text {
            Layout.fillWidth: true
            visible: recorder.statusText.length > 0
            text: recorder.statusText
            color: recorder.conflict ? "#93421e" : "#667985"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
    }
}
