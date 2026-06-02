import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    width: 720
    height: 580
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: qsTr("Oops Jump Settings")
    color: "#f5f6f8"

    property int selectedPage: 0

    background: Rectangle {
        color: "#f5f6f8"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Title bar ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 52
            color: "#ffffff"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: qsTr("Oops Jump")
                    color: "#1a1a1a"
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: statusLabel.implicitWidth + 20
                    Layout.preferredHeight: 22
                    radius: 11
                    color: settingsController.connected ? "#e8f5e9" : "#fbe9e7"

                    Text {
                        id: statusLabel
                        anchors.centerIn: parent
                        text: settingsController.connected ? qsTr("● 已连接") : qsTr("● 未连接")
                        color: settingsController.connected ? "#2e7d32" : "#c62828"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#e8e8e8"
            }
        }

        // ── Tab bar ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 42
            color: "#ffffff"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                spacing: 0

                Repeater {
                    model: [
                        qsTr("快捷绑定"),
                        qsTr("设置"),
                        qsTr("关于")
                    ]

                    delegate: Item {
                        id: tabItem

                        required property int index
                        required property string modelData

                        Layout.preferredWidth: tabLabel.implicitWidth + 32
                        Layout.fillHeight: true

                        Text {
                            id: tabLabel
                            anchors.centerIn: parent
                            text: tabItem.modelData
                            color: window.selectedPage === tabItem.index ? "#00857a" : "#888888"
                            font.pixelSize: 14
                            font.weight: window.selectedPage === tabItem.index ? Font.DemiBold : Font.Normal
                        }

                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: tabLabel.implicitWidth + 16
                            height: 2
                            radius: 1
                            color: "#00857a"
                            visible: window.selectedPage === tabItem.index
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: window.selectedPage = tabItem.index
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#e8e8e8"
            }
        }

        // ── Content area ──
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.selectedPage

            // Page 0: Bindings
            BindingsPage {
                controller: settingsController
            }

            // Page 1: Settings
            SettingsPage {
                controller: settingsController
            }

            // Page 2: About
            AboutPage {
                controller: settingsController
            }
        }

        // ── Footer ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 44
            color: "#fafbfc"

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: "#e8e8e8"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: qsTr("按 Esc 关闭窗口")
                    color: "#bbbbbb"
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                StyledButton {
                    text: qsTr("刷新")
                    onClicked: settingsController.refresh()
                }
            }
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: window.close()
    }
}
