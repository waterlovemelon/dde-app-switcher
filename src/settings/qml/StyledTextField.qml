import QtQuick
import QtQuick.Controls

TextField {
    id: control

    implicitHeight: 32
    color: "#333333"
    placeholderTextColor: "#bbbbbb"
    font.pixelSize: 13
    selectByMouse: true
    selectedTextColor: "#ffffff"
    selectionColor: "#00857a"

    leftPadding: 10
    rightPadding: 10

    background: Rectangle {
        radius: 8
        color: control.hovered || control.activeFocus ? "#ffffff" : "#fafbfc"
        border.width: 1
        border.color: control.activeFocus ? "#00857a" : (control.hovered ? "#bbbbbb" : "#dddddd")

        Behavior on border.color { ColorAnimation { duration: 100 } }
    }
}
