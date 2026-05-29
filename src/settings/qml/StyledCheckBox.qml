import QtQuick
import QtQuick.Controls

CheckBox {
    id: control

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 4
        color: control.checked ? "#00857a" : "#ffffff"
        border.width: 1
        border.color: control.checked ? "#00695c" : (control.hovered ? "#bbbbbb" : "#cccccc")

        Behavior on color { ColorAnimation { duration: 100 } }

        Text {
            anchors.centerIn: parent
            text: "✓"
            color: "#ffffff"
            font.pixelSize: 12
            font.weight: Font.Bold
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        color: "#1a1a1a"
        font.pixelSize: 13
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + 8
    }
}
