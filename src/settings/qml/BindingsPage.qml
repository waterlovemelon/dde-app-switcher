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
            return qsTr("Application target not set")
        }

        for (var i = 0; i < controller.applications.length; ++i) {
            var app = controller.applications[i]
            if (app.desktop_id === id) {
                return displayText(app.localized_name || app.name, id)
            }
        }
        return id
    }

    function openEditor(binding) {
        editorDialog.openForBinding(binding)
    }

    component SectionCard: Rectangle {
        color: "#fbfdfd"
        radius: 18
        border.width: 1
        border.color: "#d8e3e8"
    }

    component MutedText: Text {
        color: "#667985"
        font.pixelSize: 13
        elide: Text.ElideRight
    }

    component FieldLabel: Text {
        color: "#405863"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }

    component InfoChip: Rectangle {
        property string label: ""
        property string value: ""

        implicitWidth: Math.min(chipContent.implicitWidth + 20, 260)
        implicitHeight: 32
        width: implicitWidth
        height: implicitHeight
        radius: 10
        color: "#eef6f8"
        border.width: 1
        border.color: "#d4e4e9"

        RowLayout {
            id: chipContent

            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 6

            Text {
                text: label
                color: "#667985"
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }

            Text {
                Layout.maximumWidth: 160
                text: value
                color: "#214a58"
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("Configured bindings")
                    color: "#17313c"
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }

                MutedText {
                    Layout.fillWidth: true
                    text: qsTr("Edit hotkeys, target desktop entries, and switching behavior.")
                }
            }

            Button {
                text: qsTr("Add Binding")
                onClicked: page.openEditor({})
            }
        }

        SectionCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                ListView {
                    id: bindingList

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 10
                    model: controller.bindings

                    delegate: SectionCard {
                        required property var modelData

                        width: bindingList.width
                        height: cardContent.implicitHeight + 24
                        color: "#fbfdfd"

                        ColumnLayout {
                            id: cardContent

                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Rectangle {
                                    Layout.preferredWidth: 34
                                    Layout.preferredHeight: 34
                                    radius: 10
                                    color: (modelData.enabled === undefined || modelData.enabled) ? "#dff3e7" : "#f2e8df"
                                    border.width: 1
                                    border.color: (modelData.enabled === undefined || modelData.enabled) ? "#9ed5b8" : "#dcc1a8"

                                    Text {
                                        anchors.centerIn: parent
                                        text: (modelData.enabled === undefined || modelData.enabled) ? qsTr("On") : qsTr("Off")
                                        color: (modelData.enabled === undefined || modelData.enabled) ? "#1f6c43" : "#86552c"
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Text {
                                        Layout.fillWidth: true
                                        text: page.appNameForDesktopId(modelData.desktop_id)
                                        color: "#17313c"
                                        font.pixelSize: 15
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    MutedText {
                                        Layout.fillWidth: true
                                        text: page.displayText(modelData.desktop_id, page.displayText(modelData.id, qsTr("No desktop id")))
                                    }
                                }

                                RowLayout {
                                    spacing: 8

                                    Button {
                                        text: qsTr("Edit")
                                        onClicked: page.openEditor(modelData)
                                    }

                                    Button {
                                        text: qsTr("Delete")
                                        enabled: page.displayText(modelData.id, "").length > 0
                                        onClicked: {
                                            if (controller.removeBinding(modelData.id)) {
                                                controller.refresh()
                                            }
                                        }
                                    }
                                }
                            }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 8

                                InfoChip {
                                    label: qsTr("Hotkey")
                                    value: page.displayText(modelData.hotkey, qsTr("Unassigned"))
                                }

                                InfoChip {
                                    label: qsTr("Strategy")
                                    value: page.strategyValue(modelData)
                                }

                                InfoChip {
                                    label: qsTr("Status")
                                    value: (modelData.enabled === undefined || modelData.enabled) ? qsTr("Enabled") : qsTr("Disabled")
                                }

                                InfoChip {
                                    label: qsTr("Binding")
                                    value: page.displayText(modelData.id, qsTr("Unnamed"))
                                }

                                InfoChip {
                                    label: qsTr("Target")
                                    value: page.displayText(modelData.desktop_id, qsTr("Unset"))
                                }
                            }
                        }
                    }

                    footer: Item {
                        width: bindingList.width
                        height: controller.bindings.length === 0 ? 140 : 0

                        ColumnLayout {
                            anchors.centerIn: parent
                            width: parent.width - 40
                            visible: controller.bindings.length === 0
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                text: qsTr("No bindings loaded")
                                color: "#17313c"
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                            }

                            MutedText {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                text: qsTr("Start the agent, refresh, or add a binding with a desktop id.")
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? Math.max(42, errorMessage.implicitHeight + 22) : 0
            radius: 12
            visible: controller.lastError.length > 0
                     && controller.lastErrorCode !== "hotkey_backend_unavailable"
            color: "#fff1e9"
            border.width: 1
            border.color: "#e2b199"

            Text {
                id: errorMessage
                anchors.fill: parent
                anchors.margins: 12
                text: controller.lastError
                color: "#93421e"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
        }
    }

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

        title: originalId.length > 0 ? qsTr("Edit Binding") : qsTr("Add Binding")
        modal: true
        width: Math.min(page.width - 48, 560)
        x: Math.round((page.width - width) / 2)
        y: Math.round((page.height - height) / 2)

        contentItem: ColumnLayout {
            spacing: 14

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 10
                columnSpacing: 16

                FieldLabel { text: qsTr("Binding ID") }
                TextField {
                    id: idField
                    Layout.fillWidth: true
                    readOnly: editorDialog.originalId.length > 0
                    placeholderText: qsTr("terminal")
                }

                FieldLabel { text: qsTr("Enabled") }
                CheckBox {
                    id: enabledField
                    checked: true
                }

                FieldLabel { text: qsTr("Hotkey") }
                HotkeyRecorder {
                    id: hotkeyField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Alt+Return")
                    controller: page.controller
                }

                FieldLabel { text: qsTr("Desktop ID") }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: desktopIdField
                        Layout.fillWidth: true
                        placeholderText: qsTr("org.deepin.Terminal.desktop")
                    }

                    Button {
                        text: qsTr("Pick")
                        onClicked: appPickerDialog.open()
                    }
                }

                FieldLabel { text: qsTr("Strategy") }
                ComboBox {
                    id: strategyField
                    Layout.fillWidth: true
                    textRole: "label"
                    valueRole: "value"
                    model: [
                        { "label": qsTr("Default"), "value": "default" },
                        { "label": qsTr("Recent"), "value": "recent" },
                        { "label": qsTr("Cycle"), "value": "cycle" },
                        { "label": qsTr("Picker"), "value": "picker" }
                    ]
                }

                FieldLabel { text: qsTr("Launch if not running") }
                CheckBox {
                    id: launchIfNotRunningField
                    checked: true
                }

                FieldLabel { text: qsTr("Focus existing window") }
                CheckBox {
                    id: focusExistingWindowField
                    checked: true
                }
            }

            Text {
                Layout.fillWidth: true
                visible: editorDialog.errorText.length > 0
                text: editorDialog.errorText
                color: "#93421e"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("Cancel")
                    onClicked: editorDialog.close()
                }

                Button {
                    text: qsTr("Save")
                    enabled: editorDialog.canSave()
                    onClicked: {
                        hotkeyField.testCurrentHotkey()
                        if (hotkeyField.conflict) {
                            editorDialog.errorText = page.displayText(hotkeyField.statusText, qsTr("Hotkey validation failed."))
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
                            editorDialog.errorText = page.displayText(controller.lastError, qsTr("Saving the binding failed."))
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: appPickerDialog

        title: qsTr("Choose Application")
        modal: true
        width: Math.min(page.width - 48, 720)
        height: Math.min(page.height - 48, 560)
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
