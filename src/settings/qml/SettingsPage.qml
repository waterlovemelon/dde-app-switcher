import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Flickable {
    id: page

    required property var controller

    contentHeight: settingsColumn.implicitHeight + 40
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ColumnLayout {
        id: settingsColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 24
        spacing: 0

        // ── General ──
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: sectionGeneralLabel.implicitHeight + 12 + 1
            Layout.bottomMargin: 12

            Rectangle {
                width: 3
                height: sectionGeneralLabel.implicitHeight
                radius: 1
                color: DTKTheme.accentText
                anchors.left: parent.left
                anchors.verticalCenter: sectionGeneralLabel.verticalCenter
            }

            Text {
                id: sectionGeneralLabel
                text: qsTr("通用")
                color: DTKTheme.textSecondary
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 0.5
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.top: parent.top
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: DTKTheme.separator
            }
        }

        // Autostart
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("开机自启")
                    color: DTKTheme.textPrimary
                    font.pixelSize: 14
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("登录时自动启动 Oops Jump 后台服务")
                    color: DTKTheme.textMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }

            StyledSwitch {
                checked: controller.autostartEnabled
                onToggled: {
                    if (checked !== controller.autostartEnabled) {
                        controller.setAutostartEnabled(checked)
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: DTKTheme.separator; Layout.topMargin: 12; Layout.bottomMargin: 12 }

        // Show overlay
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("显示浮窗")
                    color: DTKTheme.textPrimary
                    font.pixelSize: 14
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("切换应用时显示覆盖层提示")
                    color: DTKTheme.textMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }

            StyledSwitch {
                checked: controller.showOverlay
                onToggled: {
                    if (checked !== controller.showOverlay) {
                        controller.setShowOverlay(checked)
                    }
                }
            }
        }

        // ── Window ──
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: sectionWindowLabel.implicitHeight + 12 + 1
            Layout.topMargin: 24
            Layout.bottomMargin: 12

            Rectangle {
                width: 3
                height: sectionWindowLabel.implicitHeight
                radius: 1
                color: DTKTheme.accentText
                anchors.left: parent.left
                anchors.verticalCenter: sectionWindowLabel.verticalCenter
            }

            Text {
                id: sectionWindowLabel
                text: qsTr("窗口切换")
                color: DTKTheme.textSecondary
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 0.5
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.top: parent.top
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: DTKTheme.separator
            }
        }

        // Multi-window strategy
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("多窗口策略")
                    color: DTKTheme.textPrimary
                    font.pixelSize: 14
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("应用有多个窗口时的默认切换行为")
                    color: DTKTheme.textMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }

            StyledComboBox {
                textRole: "label"
                valueRole: "value"
                model: [
                    { "label": qsTr("最近使用的窗口"), "value": "recent" },
                    { "label": qsTr("循环切换"), "value": "cycle" },
                    { "label": qsTr("弹窗选择"), "value": "picker" }
                ]
                currentIndex: {
                    for (var i = 0; i < model.length; i++) {
                        if (model[i].value === controller.defaultWindowStrategy) return i
                    }
                    return 1
                }
                onActivated: {
                    if (currentValue !== controller.defaultWindowStrategy) {
                        controller.setDefaultWindowStrategy(currentValue)
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: DTKTheme.separator; Layout.topMargin: 12; Layout.bottomMargin: 12 }

        // Switch workspace
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("跨工作区切换")
                    color: DTKTheme.textPrimary
                    font.pixelSize: 14
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("目标窗口在其他工作区时自动切换过去")
                    color: DTKTheme.textMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }

            StyledSwitch {
                checked: controller.switchWorkspaceWhenNeeded
                onToggled: {
                    if (checked !== controller.switchWorkspaceWhenNeeded) {
                        controller.setSwitchWorkspaceWhenNeeded(checked)
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: DTKTheme.separator; Layout.topMargin: 12; Layout.bottomMargin: 12 }

        // Include all workspaces
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("包含所有工作区")
                    color: DTKTheme.textPrimary
                    font.pixelSize: 14
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("查找窗口时搜索所有工作区，而非仅当前工作区")
                    color: DTKTheme.textMuted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }

            StyledSwitch {
                checked: controller.includeAllWorkspaces
                onToggled: {
                    if (checked !== controller.includeAllWorkspaces) {
                        controller.setIncludeAllWorkspaces(checked)
                    }
                }
            }
        }
    }
}
