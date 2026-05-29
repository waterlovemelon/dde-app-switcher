import QtQuick
import QtQuick.Controls

ComboBox {
    id: control

    implicitWidth: 160
    implicitHeight: 34

    property real popupWidth: control.width

    Component.onCompleted: {
        var max = control.width
        for (var i = 0; i < model.length; i++) {
            var label = model[i].label || model[i]
            textMetrics.text = label
            max = Math.max(max, textMetrics.advanceWidth + 36)
        }
        popupWidth = Math.min(max, 360)
    }

    TextMetrics {
        id: textMetrics
        font.pixelSize: 13
    }

    contentItem: Text {
        leftPadding: 12
        rightPadding: 28
        text: control.displayText
        color: "#1a1a1a"
        font.pixelSize: 13
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 160
        implicitHeight: 34
        radius: 8
        color: control.hovered ? "#f8f8f8" : "#fafbfc"
        border.width: 1
        border.color: control.hovered ? "#bbbbbb" : "#dddddd"
    }

    indicator: Canvas {
        x: control.width - width - 12
        y: control.height / 2 - height / 2
        width: 10
        height: 6
        contextType: "2d"
        onPaint: {
            context.reset()
            context.moveTo(0, 0)
            context.lineTo(width, 0)
            context.lineTo(width / 2, height)
            context.closePath()
            context.fillStyle = "#666666"
            context.fill()
        }
        Connections {
            target: control
            function onPressedChanged() { indicator.requestPaint() }
        }
    }

    popup: Popup {
        id: comboPopup
        y: control.height + 4
        width: control.popupWidth
        height: Math.min(listView.contentHeight + 16, 260)
        padding: 6
        closePolicy: Popup.CloseOnPressOutside

        contentItem: ListView {
            id: listView
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            radius: 10
            color: "#ffffff"
            border.width: 1
            border.color: "#dddddd"
        }
    }

    delegate: Rectangle {
        id: del
        required property var modelData
        required property int index
        property bool isHighlighted: control.highlightedIndex === index

        width: comboPopup.width - 12
        height: 36
        color: "transparent"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 3
            radius: 6
            color: del.isHighlighted ? "#e0f2f1" : (mouseArea.containsMouse ? "#f5f5f5" : "transparent")
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.right: parent.right
            anchors.rightMargin: 14
            text: modelData.label
            color: "#1a1a1a"
            font.pixelSize: 13
            elide: Text.ElideRight
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                control.currentIndex = del.index
                control.popup.close()
            }
        }
    }
}
