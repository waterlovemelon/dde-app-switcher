import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page

    required property var controller

    // Editor data (set before opening)
    property string editorOriginalId: ""
    property var editorOriginalBinding: ({})
    property string editorDesktopId: ""
    property string editorHotkey: ""
    property string editorStrategy: "default"
    property bool editorLaunchIfNotRunning: true
    property bool editorFocusExistingWindow: true
    property string editorErrorText: ""
    property bool editorShowingPicker: false

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
        if (id.length === 0) return qsTr("未设置应用")
        for (var i = 0; i < controller.applications.length; ++i) {
            var app = controller.applications[i]
            if (app.desktop_id === id)
                return displayText(app.localized_name || app.name, id)
        }
        return id
    }

    function iconLetter(binding) {
        var name = appNameForDesktopId(binding.desktop_id)
        return name.charAt(0).toLocaleUpperCase()
    }

    function appIconForDesktopId(desktopId) {
        var id = displayText(desktopId, "")
        if (id.length === 0) return "application-x-executable"
        for (var i = 0; i < controller.applications.length; ++i) {
            var app = controller.applications[i]
            if (app.desktop_id === id)
                return app.icon || "application-x-executable"
        }
        return "application-x-executable"
    }

    function isEnabled(binding) {
        return binding.enabled === undefined || binding.enabled
    }

    function normalize(value) {
        return displayText(value, "").trim()
    }

    function copyBinding(binding) {
        var copy = {}
        for (var key in binding) copy[key] = binding[key]
        return copy
    }

    function generateId(desktopId) {
        var id = normalize(desktopId)
        if (id.toLowerCase().endsWith(".desktop"))
            id = id.substring(0, id.length - 8)
        id = id.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase()
        return id
    }

    function openEditor(binding) {
        editorOriginalBinding = copyBinding(binding)
        editorOriginalId = displayText(binding.id, "")
        editorDesktopId = displayText(binding.desktop_id, "")
        editorHotkey = displayText(binding.hotkey, "")
        editorStrategy = strategyValue(binding)
        editorLaunchIfNotRunning = binding.launch_if_not_running === undefined ? true : binding.launch_if_not_running
        editorFocusExistingWindow = binding.focus_existing_window === undefined ? true : binding.focus_existing_window
        editorErrorText = ""
        editorShowingPicker = false
        editorLoader.active = true
    }

    function closeEditor() {
        editorShowingPicker = false
        editorLoader.active = false
    }

    function saveFromEditor(desktopId, hotkey, strategy, launch, focus, hotkeyField) {
        hotkeyField.testCurrentHotkey()
        if (hotkeyField.conflict) {
            editorErrorText = displayText(hotkeyField.statusText, qsTr("快捷键验证失败。"))
            return
        }

        var binding = copyBinding(editorOriginalBinding)
        var normDesktopId = normalize(desktopId)
        binding.id = editorOriginalId.length > 0 ? editorOriginalId : generateId(normDesktopId)
        binding.enabled = binding.enabled === undefined ? true : binding.enabled
        binding.hotkey = normalize(hotkey)
        binding.desktop_id = normDesktopId
        binding.multi_window_strategy = strategy
        binding.launch_if_not_running = launch
        binding.focus_existing_window = focus

        if (controller.saveBinding(binding)) {
            controller.refresh()
            closeEditor()
        } else {
            editorErrorText = displayText(controller.lastError, qsTr("保存失败。"))
        }
    }

    component MutedText: Text {
        color: "#999999"
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    // ═══════════════════════════════════════
    //  Main content
    // ═══════════════════════════════════════
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

                    Image {
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 34
                        sourceSize.width: 34
                        sourceSize.height: 34
                        source: "image://theme/" + page.appIconForDesktopId(modelData.desktop_id)
                        fillMode: Image.PreserveAspectFit
                        opacity: page.isEnabled(modelData) ? 1.0 : 0.4
                    }

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

                    StyledSwitch {
                        checked: page.isEnabled(modelData)
                        onToggled: {
                            var binding = {}
                            for (var key in modelData) binding[key] = modelData[key]
                            binding.enabled = checked
                            if (controller.saveBinding(binding))
                                controller.refresh()
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

    // Dialog loaded dynamically, destroyed on close.
    Loader {
        id: editorLoader
        active: false
        parent: Overlay.overlay
        anchors.fill: parent
        z: 1000

        sourceComponent: Component {
            Item {
                anchors.fill: parent

                // Mask
                Rectangle {
                    anchors.fill: parent
                    color: "#66000000"

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.AllButtons
                        hoverEnabled: true
                        onClicked: page.closeEditor()
                    }
                }

                Item {
                    id: win
                    width: Math.min(parent.width - 48, 560)
                    height: Math.min(parent.height - 48, 420)
                    x: Math.round((parent.width - width) / 2)
                    y: Math.round((parent.height - height) / 2)

                    Rectangle {
                        id: dialogShadow
                        anchors.fill: dialogPanel
                        anchors.topMargin: 8
                        anchors.bottomMargin: -8
                        radius: dialogPanel.radius
                        color: "#26000000"
                    }

                    Rectangle {
                        anchors.fill: dialogPanel
                        anchors.margins: -1
                        anchors.topMargin: 1
                        radius: dialogPanel.radius + 1
                        color: "#10000000"
                    }

                    Rectangle {
                        id: dialogPanel
                        anchors.fill: parent
                        radius: 14
                        color: "#ffffff"
                        border.width: 1
                        border.color: "#d7dce0"
                        clip: true

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.AllButtons
                            hoverEnabled: true
                        }

                        ColumnLayout {
                            id: winContent
                            anchors.fill: parent
                            spacing: 0

                            // Header
                            Item {
                                id: dialogHeader
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    height: 1
                                    color: "#e8e8e8"
                                }
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 16
                                    anchors.rightMargin: 12
                                    spacing: 10
                                    Rectangle {
                                        Layout.preferredWidth: 24
                                        Layout.preferredHeight: 24
                                        radius: 6
                                        color: "#e0f2f1"
                                        Text {
                                            anchors.centerIn: parent
                                        text: page.editorShowingPicker
                                              ? "A"
                                              : page.editorOriginalId.length > 0
                                                ? page.iconLetter(page.editorOriginalBinding)
                                                : "+"
                                            color: "#00695c"
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                        }
                                    }
                                    Text {
                                        text: page.editorShowingPicker
                                              ? qsTr("选择应用")
                                              : page.editorOriginalId.length > 0 ? qsTr("编辑绑定") : qsTr("添加绑定")
                                        color: "#1a1a1a"
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }
                                    Item { Layout.fillWidth: true }
                                    Rectangle {
                                        Layout.preferredWidth: 28
                                        Layout.preferredHeight: 28
                                        radius: 14
                                        visible: page.editorShowingPicker
                                        color: backBtnMouse.containsMouse ? "#f2f3f5" : "transparent"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "<"
                                            color: "#777777"
                                            font.pixelSize: 16
                                            font.weight: Font.DemiBold
                                        }
                                        MouseArea {
                                            id: backBtnMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: page.editorShowingPicker = false
                                        }
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: 28
                                        Layout.preferredHeight: 28
                                        radius: 14
                                        color: closeBtnMouse.containsMouse ? "#fee" : "transparent"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "x"
                                            color: closeBtnMouse.containsMouse ? "#c62828" : "#999"
                                            font.pixelSize: 14
                                        }
                                        MouseArea {
                                            id: closeBtnMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: page.closeEditor()
                                        }
                                    }
                                }
                            }

                            StackLayout {
                                id: editorStack
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                currentIndex: page.editorShowingPicker ? 1 : 0

                                Item {
                                    id: editorPage
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 20
                                        spacing: 12

                                        GridLayout {
                                            Layout.fillWidth: true
                                            columns: 2
                                            rowSpacing: 8
                                            columnSpacing: 14

                                            Text { text: qsTr("应用"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 8
                                                StyledTextField {
                                                    id: fldDesktopId
                                                    Layout.fillWidth: true
                                                    text: page.editorDesktopId
                                                    placeholderText: qsTr("org.deepin.Terminal.desktop")
                                                }
                                                StyledButton {
                                                    text: qsTr("选择")
                                                    font.pixelSize: 12
                                                    onClicked: page.editorShowingPicker = true
                                                }
                                            }

                                            Text { text: qsTr("快捷键"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                                            HotkeyRecorder {
                                                id: fldHotkey
                                                Layout.fillWidth: true
                                                text: page.editorHotkey
                                                placeholderText: qsTr("Alt+Return")
                                                controller: page.controller
                                                excludeActionId: page.editorOriginalId
                                            }

                                            Rectangle {
                                                Layout.columnSpan: 2
                                                Layout.fillWidth: true
                                                Layout.topMargin: 4
                                                Layout.bottomMargin: 4
                                                height: 1
                                                color: "#f0f0f0"
                                            }

                                            Text { text: qsTr("多窗口策略"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                                            StyledComboBox {
                                                id: fldStrategy
                                                Layout.fillWidth: true
                                                textRole: "label"
                                                valueRole: "value"
                                                model: [
                                                    { "label": qsTr("默认"), "value": "default" },
                                                    { "label": qsTr("最近"), "value": "recent" },
                                                    { "label": qsTr("循环"), "value": "cycle" },
                                                    { "label": qsTr("选择器"), "value": "picker" }
                                                ]
                                                Component.onCompleted: currentIndex = Math.max(0, indexOfValue(page.editorStrategy))
                                            }

                                            Text { text: qsTr("未运行时"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                                            StyledCheckBox {
                                                id: fldLaunch
                                                checked: page.editorLaunchIfNotRunning
                                                text: qsTr("自动启动应用")
                                            }

                                            Text { text: qsTr("已有窗口"); color: "#405863"; font.pixelSize: 13; font.weight: Font.DemiBold }
                                            StyledCheckBox {
                                                id: fldFocus
                                                checked: page.editorFocusExistingWindow
                                                text: qsTr("直接聚焦已有窗口")
                                            }
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            visible: page.editorErrorText.length > 0
                                            text: page.editorErrorText
                                            color: "#c62828"
                                            font.pixelSize: 13
                                            wrapMode: Text.WordWrap
                                        }

                                        Item { Layout.fillHeight: true }
                                    }
                                }

                                Item {
                                    id: pickerPage
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true

                                    ApplicationPicker {
                                        anchors.fill: parent
                                        anchors.margins: 16
                                        applications: controller.applications
                                        selectedDesktopId: fldDesktopId.text
                                        onApplicationSelected: function(application) {
                                            fldDesktopId.text = page.displayText(application.desktop_id, "")
                                            page.editorShowingPicker = false
                                        }
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 52

                                Rectangle {
                                    anchors.top: parent.top
                                    width: parent.width
                                    height: 1
                                    color: "#e8e8e8"
                                }
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 10
                                    Item { Layout.fillWidth: true }
                                    StyledButton {
                                        text: page.editorShowingPicker ? qsTr("返回") : qsTr("取消")
                                        onClicked: {
                                            if (page.editorShowingPicker) {
                                                page.editorShowingPicker = false
                                            } else {
                                                page.closeEditor()
                                            }
                                        }
                                    }
                                    StyledButton {
                                        visible: !page.editorShowingPicker
                                        text: qsTr("保存")
                                        style: "primary"
                                        enabled: fldDesktopId.text.trim().length > 0
                                                  && fldHotkey.text.trim().length > 0
                                                  && !fldHotkey.conflict
                                        onClicked: page.saveFromEditor(
                                            fldDesktopId.text, fldHotkey.text,
                                            fldStrategy.currentValue,
                                            fldLaunch.checked, fldFocus.checked,
                                            fldHotkey
                                        )
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
