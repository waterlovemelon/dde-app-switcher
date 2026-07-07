import QtQuick
import QtQuick.Controls
import "."

Button {
    id: control

    property string style: "default" // "default", "primary", "ghost", "warn"

    contentItem: Text {
        text: control.text
        color: {
            if (control.style === "primary") return DTKTheme.primaryButtonText
            if (control.style === "warn") return "#ffffff"
            if (control.style === "ghost") return control.hovered ? DTKTheme.accentTextHovered : DTKTheme.textSecondary
            return control.hovered ? DTKTheme.buttonTextHovered : DTKTheme.buttonText
        }
        font.pixelSize: 13
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitWidth: 72
        implicitHeight: 34
        radius: 8

        color: {
            if (control.style === "primary") return control.hovered ? DTKTheme.primaryButtonBackgroundHovered : DTKTheme.primaryButtonBackground
            if (control.style === "warn") return control.hovered ? DTKTheme.errorText : DTKTheme.errorBorder
            if (control.style === "ghost") return "transparent"
            return control.hovered ? DTKTheme.buttonBackgroundHovered : DTKTheme.buttonBackground
        }

        border.width: 1
        border.color: {
            if (control.style === "primary") return control.hovered ? DTKTheme.primaryButtonBackgroundHovered : DTKTheme.primaryButtonBackground
            if (control.style === "warn") return control.hovered ? DTKTheme.errorText : DTKTheme.errorBorder
            if (control.style === "ghost") return control.hovered ? DTKTheme.accentText : DTKTheme.buttonBorder
            return control.hovered ? DTKTheme.buttonBorderHovered : DTKTheme.buttonBorder
        }
    }
}
