import QtQuick
import QtQuick.Controls

Switch {
    id: control

    indicator: Rectangle {
        implicitWidth: 40
        implicitHeight: 22
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 11
        color: control.checked ? "#00857a" : "#cccccc"
        border.width: 1
        border.color: control.checked ? "#00695c" : "#bbbbbb"

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }

        Rectangle {
            x: control.checked ? parent.width - width - 2 : 2
            y: 2
            width: 18
            height: 18
            radius: 9
            color: "#ffffff"
            border.width: 1
            border.color: control.checked ? "#00695c" : "#aaaaaa"

            Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.InOutQuad } }
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
