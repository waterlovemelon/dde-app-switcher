import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
    id: page

    required property var controller

    property bool showDebug: false

    function statusMap() {
        return controller && controller.status ? controller.status : ({})
    }

    function capabilities() {
        return statusMap()["capabilities"] || ({})
    }

    function yesNo(value) {
        return value ? qsTr("✓ 可用") : qsTr("✗ 不可用")
    }

    function bindingCount() {
        var total = controller.bindings.length
        var enabled = 0
        for (var i = 0; i < controller.bindings.length; i++) {
            if (controller.bindings[i].enabled === undefined || controller.bindings[i].enabled) {
                enabled++
            }
        }
        return qsTr("%1 个（%2 启用 / %3 停用").arg(total).arg(enabled).arg(total - enabled)
    }

    contentHeight: aboutColumn.implicitHeight + 40
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ColumnLayout {
        id: aboutColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 24
        spacing: 0

        // ── Logo + info ──
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: logoColumn.implicitHeight + 32

            ColumnLayout {
                id: logoColumn
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56
                    radius: 14
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#00897b" }
                        GradientStop { position: 1.0; color: "#00acc1" }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "D"
                        color: "white"
                        font.pixelSize: 24
                        font.weight: Font.Bold
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Oops Jump"
                    color: "#1a1a1a"
                    font.pixelSize: 20
                    font.weight: Font.Bold
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("版本 0.1.0")
                    color: "#999999"
                    font.pixelSize: 13
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: 360
                    text: qsTr("为 deepin v25 打造的键盘优先应用切换器。\n一个快捷键，一个应用。按快捷键启动或聚焦，支持多窗口循环切换。")
                    color: "#666666"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.5
                }
            }
        }

        // ── Debug toggle ──
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Layout.topMargin: 16

            Text {
                anchors.centerIn: parent
                text: page.showDebug ? qsTr("隐藏运行状态 ▲") : qsTr("查看运行状态 ▼")
                color: "#aaaaaa"
                font.pixelSize: 12

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: page.showDebug = !page.showDebug
                }
            }
        }

        // ── Debug panel ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: page.showDebug ? debugGrid.implicitHeight + 32 : 0
            Layout.topMargin: page.showDebug ? 8 : 0
            radius: 12
            color: "#fafbfc"
            border.width: 1
            border.color: "#e8e8e8"
            clip: true
            visible: page.showDebug || height > 0

            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 150; easing.type: Easing.InOutQuad }
            }

            GridLayout {
                id: debugGrid
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 16
                columns: 2
                rowSpacing: 10
                columnSpacing: 12

                // Agent status
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#eeeeee"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2

                        Text {
                            text: qsTr("Agent 状态")
                            color: "#999999"
                            font.pixelSize: 11
                        }

                        Text {
                            text: controller.connected ? qsTr("已连接") : qsTr("未连接")
                            color: controller.connected ? "#2e7d32" : "#c62828"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }

                // Session type
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#eeeeee"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2

                        Text {
                            text: qsTr("会话类型")
                            color: "#999999"
                            font.pixelSize: 11
                        }

                        Text {
                            text: statusMap()["session_type"] || qsTr("未知")
                            color: "#333333"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }

                // Active backend
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#eeeeee"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2

                        Text {
                            text: qsTr("活跃后端")
                            color: "#999999"
                            font.pixelSize: 11
                        }

                        Text {
                            text: statusMap()["active_backend"] || qsTr("不可用")
                            color: "#333333"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }

                // Binding count
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#eeeeee"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2

                        Text {
                            text: qsTr("绑定数量")
                            color: "#999999"
                            font.pixelSize: 11
                        }

                        Text {
                            text: bindingCount()
                            color: "#333333"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }

                // Global hotkey
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#eeeeee"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2

                        Text {
                            text: qsTr("全局热键")
                            color: "#999999"
                            font.pixelSize: 11
                        }

                        Text {
                            text: yesNo(capabilities()["global_hotkey"])
                            color: capabilities()["global_hotkey"] ? "#2e7d32" : "#c62828"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }

                // Window list
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#eeeeee"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2

                        Text {
                            text: qsTr("窗口列表")
                            color: "#999999"
                            font.pixelSize: 11
                        }

                        Text {
                            text: yesNo(capabilities()["window_list"])
                            color: capabilities()["window_list"] ? "#2e7d32" : "#c62828"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }

                // Activate window
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#eeeeee"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2

                        Text {
                            text: qsTr("窗口激活")
                            color: "#999999"
                            font.pixelSize: 11
                        }

                        Text {
                            text: yesNo(capabilities()["activate_window"])
                            color: capabilities()["activate_window"] ? "#2e7d32" : "#c62828"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }

                // Launch app
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#eeeeee"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2

                        Text {
                            text: qsTr("应用启动")
                            color: "#999999"
                            font.pixelSize: 11
                        }

                        Text {
                            text: yesNo(capabilities()["launch_app"])
                            color: capabilities()["launch_app"] ? "#2e7d32" : "#c62828"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }
        }
    }
}
