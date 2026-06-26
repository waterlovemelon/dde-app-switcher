import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: window

    width: 720
    height: 580
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: qsTr("Oops Jump Settings")
    color: DTKTheme.windowBackground

    property int selectedPage: 0

    background: Rectangle {
        color: DTKTheme.windowBackground
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Title bar ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 52
            color: DTKTheme.titleBarBackground

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: qsTr("Oops Jump")
                    color: DTKTheme.textPrimary
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: statusLabel.implicitWidth + 20
                    Layout.preferredHeight: 22
                    radius: 11
                    color: settingsController.connected ? DTKTheme.statusConnectedBg : DTKTheme.statusDisconnectedBg

                    Text {
                        id: statusLabel
                        anchors.centerIn: parent
                        text: settingsController.connected ? qsTr("● 已连接") : qsTr("● 未连接")
                        color: settingsController.connected ? DTKTheme.successText : DTKTheme.errorText
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: DTKTheme.cardBorder
            }
        }

        // ── Tab bar ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 42
            color: DTKTheme.titleBarBackground

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
                            color: window.selectedPage === tabItem.index ? DTKTheme.tabActive : DTKTheme.tabInactive
                            font.pixelSize: 14
                            font.weight: window.selectedPage === tabItem.index ? Font.DemiBold : Font.Normal
                        }

                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: tabLabel.implicitWidth + 16
                            height: 2
                            radius: 1
                            color: DTKTheme.tabIndicator
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
                color: DTKTheme.cardBorder
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
            color: DTKTheme.footerBackground

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: DTKTheme.cardBorder
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: qsTr("按 Esc 关闭窗口")
                    color: DTKTheme.textDisabled
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
