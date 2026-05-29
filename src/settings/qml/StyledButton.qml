import QtQuick
import QtQuick.Controls

Button {
    id: control

    property string style: "default" // "default", "primary", "ghost"

    contentItem: Text {
        text: control.text
        color: {
            if (control.style === "primary") return "#ffffff"
            if (control.style === "ghost") return control.hovered ? "#00857a" : "#888888"
            return control.hovered ? "#00857a" : "#333333"
        }
        font.pixelSize: 13
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitWidth: 72
        implicitHeight: 32
        radius: 8

        color: {
            if (control.style === "primary") return control.hovered ? "#00695c" : "#00857a"
            if (control.style === "ghost") return "transparent"
            return control.hovered ? "#f5f5f5" : "#ffffff"
        }

        border.width: control.style === "ghost" ? 1 : 1
        border.color: {
            if (control.style === "primary") return control.hovered ? "#00695c" : "#00857a"
            if (control.style === "ghost") return control.hovered ? "#00857a" : "#d0d0d0"
            return control.hovered ? "#cccccc" : "#dddddd"
        }
    }
}
