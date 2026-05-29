import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page

    required property var controller

    function displayText(value, fallback) {
        if (value === undefined || value === null || String(value).trim().length === 0) {
            return fallback
        }
        return String(value)
    }

    function strategyValue(binding) {
        return displayText(binding.multi_window_strategy || binding.strategy, qsTr("default"))
    }

    function appNameForDesktopId(desktopId) {
        var id = displayText(desktopId, "")
        if (id.length === 0) {
            return qsTr("未设置应用")
        }

        for (var i = 0; i < controller.applications.length; ++i) {
            var app = controller.applications[i]
            if (app.desktop_id === id) {
                return displayText(app.localized_name || app.name, id)
            }
        }
        return id
    }

    function iconLetter(binding) {
        var name = appNameForDesktopId(binding.desktop_id)
        return name.charAt(0).toLocaleUpperCase()
    }

    function isEnabled(binding) {
        return binding.enabled === undefined || binding.enabled
    }

    function openEditor(binding) {
        editorDialog.openForBinding(binding)
    }

    component MutedText: Text {
        color: "#999999"
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        Text {
            text: qsTr("按快捷键启动或切换到对应应用，点击卡片可编辑。")
            color: "#aaaaaa"
            font.pixelSize: 12
        }

        ListView {
            id: bindingList

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: controller.bindings

            delegate: Rectangle {
                required property var modelData

                width: bindingList.width
                height: 58
                radius: 12
                color: "#fafbfc"
                border.width: 1
                border.color: "#e8e8e8"

                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 60
                    cursorShape: Qt.PointingHandCursor
                    onClicked: page.openEditor(modelData)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 12

                    // Icon letter
                    Rectangle {
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 34
                        radius: 8
                        color: page.isEnabled(modelData) ? "#e0f2f1" : "#f5f5f5"

                        Text {
                            anchors.centerIn: parent
                            text: page.iconLetter(modelData)
                            color: page.isEnabled(modelData) ? "#00695c" : "#999999"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                    }

                    // App name + desktop id
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: page.appNameForDesktopId(modelData.desktop_id)
                            color: "#1a1a1a"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        MutedText {
                            Layout.fillWidth: true
                            text: page.displayText(modelData.desktop_id, "")
                            visible: text.length > 0
                        }
                    }

                    // Hotkey badge
                    Rectangle {
                        Layout.preferredWidth: hotkeyLabel.implicitWidth + 16
                        Layout.preferredHeight: 26
                        radius: 6
                        color: "#f0f0f0"

                        Text {
                            id: hotkeyLabel
                            anchors.centerIn: parent
                            text: page.displayText(modelData.hotkey, qsTr("未设置"))
                            color: "#555555"
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            font.family: "SF Mono, Cascadia Code, monospace"
                        }
                    }

                    // Toggle switch
                    StyledSwitch {
                        checked: page.isEnabled(modelData)
                        onToggled: {
                            var binding = {}
                            for (var key in modelData) binding[key] = modelData[key]
                            binding.enabled = checked
                            if (controller.saveBinding(binding)) {
                                controller.refresh()
                            }
                        }
                    }
                }
            }

            footer: Item {
                width: bindingList.width
                height: controller.bindings.length === 0 ? 160 : 60

                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    visible: controller.bindings.length === 0
                    spacing: 8

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("暂无快捷绑定")
                        color: "#1a1a1a"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    MutedText {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("点击下方按钮添加你的第一个快捷绑定。")
                    }
                }
            }
        }

        // Add button
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            radius: 12
            color: "transparent"
            border.width: 2
            border.color: addMouse.containsMouse ? "#00857a" : "#d0d0d0"

            Text {
                anchors.centerIn: parent
                text: "+ " + qsTr("添加快捷绑定")
                color: addMouse.containsMouse ? "#00857a" : "#888888"
                font.pixelSize: 14
            }

            MouseArea {
                id: addMouse
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onClicked: page.openEditor({})
            }
        }

        // Error bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? Math.max(36, errorMessage.implicitHeight + 16) : 0
            radius: 8
            visible: controller.lastError.length > 0
                     && controller.lastErrorCode !== "hotkey_backend_unavailable"
            color: "#fbe9e7"
            border.width: 1
            border.color: "#e2b199"

            Text {
                id: errorMessage
                anchors.fill: parent
                anchors.margins: 10
                text: controller.lastError
                color: "#c62828"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
        }
    }

    // ── Editor Dialog ──
    Dialog {
        id: editorDialog

        property string originalId: ""
        property string errorText: ""
        property var originalBinding: ({})

        function normalize(value) {
            return page.displayText(value, "").trim()
        }

        function copyBinding(binding) {
            var copy = {}
            for (var key in binding) {
                copy[key] = binding[key]
            }
            return copy
        }

        function isValidId(value) {
            return normalize(value).length > 0 && normalize(value).indexOf(" ") === -1
        }

        function isValidRequiredText(value) {
            return normalize(value).length > 0
        }

        function canSave() {
            return isValidId(idField.text)
                   && isValidRequiredText(hotkeyField.text)
                   && isValidRequiredText(desktopIdField.text)
                   && !hotkeyField.conflict
        }

        function openForBinding(binding) {
            originalBinding = copyBinding(binding)
            originalId = page.displayText(binding.id, "")
            errorText = ""
            idField.text = originalId
            enabledField.checked = binding.enabled === undefined ? true : binding.enabled
            hotkeyField.text = page.displayText(binding.hotkey, "")
            hotkeyField.excludeActionId = originalId
            hotkeyField.statusText = ""
            hotkeyField.conflict = false
            desktopIdField.text = page.displayText(binding.desktop_id, "")
            var strategy = page.strategyValue(binding)
            strategyField.currentIndex = Math.max(0, strategyField.indexOfValue(strategy))
            launchIfNotRunningField.checked = binding.launch_if_not_running === undefined ? true : binding.launch_if_not_running
            focusExistingWindowField.checked = binding.focus_existing_window === undefined ? true : binding.focus_existing_window
            open()
        }

        function selectApplication(application) {
            desktopIdField.text = page.displayText(application.desktop_id, "")
            appPickerDialog.close()
        }

        title: originalId.length > 0 ? qsTr("编辑绑定") : qsTr("添加绑定")
        modal: true
        width: Math.min(page.width - 48, 500)
        x: Math.round((page.width - width) / 2)
        y: Math.round((page.height - height) / 2)

        contentItem: ColumnLayout {
            spacing: 12

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 8
                columnSpacing: 14

                Text { text: qsTr("绑定 ID"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                StyledTextField {
                    id: idField
                    Layout.fillWidth: true
                    readOnly: editorDialog.originalId.length > 0
                    placeholderText: qsTr("terminal")
                }

                Text { text: qsTr("启用"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                StyledCheckBox {
                    id: enabledField
                    checked: true
                }

                Text { text: qsTr("快捷键"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                HotkeyRecorder {
                    id: hotkeyField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Alt+Return")
                    controller: page.controller
                }

                Text { text: qsTr("应用"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    StyledTextField {
                        id: desktopIdField
                        Layout.fillWidth: true
                        placeholderText: qsTr("org.deepin.Terminal.desktop")
                    }

                    StyledButton {
                        text: qsTr("选择")
                        onClicked: appPickerDialog.open()
                    }
                }

                Text { text: qsTr("多窗口策略"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                StyledComboBox {
                    id: strategyField
                    Layout.fillWidth: true
                    textRole: "label"
                    valueRole: "value"
                    model: [
                        { "label": qsTr("默认"), "value": "default" },
                        { "label": qsTr("最近"), "value": "recent" },
                        { "label": qsTr("循环"), "value": "cycle" },
                        { "label": qsTr("选择器"), "value": "picker" }
                    ]
                }

                Text { text: qsTr("未运行时启动"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                StyledCheckBox {
                    id: launchIfNotRunningField
                    checked: true
                }

                Text { text: qsTr("聚焦已有窗口"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                StyledCheckBox {
                    id: focusExistingWindowField
                    checked: true
                }
            }

            Text {
                Layout.fillWidth: true
                visible: editorDialog.errorText.length > 0
                text: editorDialog.errorText
                color: "#c62828"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                StyledButton {
                    text: qsTr("取消")
                    onClicked: editorDialog.close()
                }

                StyledButton {
                    text: qsTr("保存")
                    style: "primary"
                    enabled: editorDialog.canSave()
                    onClicked: {
                        hotkeyField.testCurrentHotkey()
                        if (hotkeyField.conflict) {
                            editorDialog.errorText = page.displayText(hotkeyField.statusText, qsTr("快捷键验证失败。"))
                            return
                        }

                        var binding = editorDialog.copyBinding(editorDialog.originalBinding)
                        binding.id = editorDialog.normalize(idField.text)
                        binding.enabled = enabledField.checked
                        binding.hotkey = editorDialog.normalize(hotkeyField.text)
                        binding.desktop_id = editorDialog.normalize(desktopIdField.text)
                        binding.multi_window_strategy = strategyField.currentValue
                        binding.launch_if_not_running = launchIfNotRunningField.checked
                        binding.focus_existing_window = focusExistingWindowField.checked

                        if (controller.saveBinding(binding)) {
                            controller.refresh()
                            editorDialog.close()
                        } else {
                            editorDialog.errorText = page.displayText(controller.lastError, qsTr("保存失败。"))
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: appPickerDialog

        title: qsTr("选择应用")
        modal: true
        width: Math.min(page.width - 48, 640)
        height: Math.min(page.height - 48, 480)
        x: Math.round((page.width - width) / 2)
        y: Math.round((page.height - height) / 2)

        contentItem: ApplicationPicker {
            applications: controller.applications
            selectedDesktopId: desktopIdField.text
            selectable: true
            onApplicationSelected: function(application) {
                editorDialog.selectApplication(application)
            }
        }
    }
}
