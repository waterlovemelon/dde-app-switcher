import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var controller

    function valueOr(value, fallback) {
        return value === undefined || value === null || value === "" ? fallback : value
    }

    function statusMap() {
        return controller && controller.status ? controller.status : ({})
    }

    function backendMap(key) {
        const status = statusMap()
        return status[key] || ({})
    }

    function capabilities() {
        return statusMap()["capabilities"] || ({})
    }

    function bindingStatuses() {
        return statusMap()["binding_statuses"] || []
    }

    function warnings() {
        return statusMap()["warnings"] || []
    }

    function yesNo(value) {
        return value ? qsTr("Yes") : qsTr("No")
    }

    function statusColor(status) {
        if (status === "registered")
            return "#dff3e7"
        if (status === "disabled" || status === "not_registered")
            return "#e7edf1"
        if (status === "invalid" || status === "conflict" || status === "app_not_found")
            return "#f8e0d8"
        return "#edf4f6"
    }

    function statusTextColor(status) {
        if (status === "registered")
            return "#1f6c43"
        if (status === "disabled" || status === "not_registered")
            return "#536773"
        if (status === "invalid" || status === "conflict" || status === "app_not_found")
            return "#93421e"
        return "#405863"
    }

    component LabelText: Text {
        color: "#667985"
        font.pixelSize: 13
        elide: Text.ElideRight
    }

    component BodyText: Text {
        color: "#17313c"
        font.pixelSize: 14
        wrapMode: Text.WordWrap
    }

    component Panel: Rectangle {
        color: "#f8fbfc"
        radius: 14
        border.width: 1
        border.color: "#d8e3e8"
    }

    component StatusPill: Rectangle {
        property string status: "unknown"
        property string label: status

        radius: 12
        implicitWidth: pillText.implicitWidth + 20
        implicitHeight: 24
        color: root.statusColor(status)
        border.width: 1
        border.color: Qt.darker(color, 1.08)

        Text {
            id: pillText
            anchors.centerIn: parent
            text: parent.label
            color: root.statusTextColor(parent.status)
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }
    }

    component BackendPanel: Panel {
        required property string title
        required property var backend

        Layout.fillWidth: true
        implicitHeight: backendContent.implicitHeight + 28

        GridLayout {
            id: backendContent
            anchors.fill: parent
            anchors.margins: 14
            columns: 2
            rowSpacing: 8
            columnSpacing: 14

            Text {
                Layout.columnSpan: 2
                text: title
                color: "#17313c"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            LabelText { text: qsTr("Name") }
            BodyText {
                Layout.fillWidth: true
                text: root.valueOr(backend["name"], qsTr("Unavailable"))
            }

            LabelText { text: qsTr("Available") }
            BodyText { text: root.yesNo(backend["available"]) }

            LabelText { text: qsTr("Running") }
            BodyText { text: root.yesNo(backend["running"]) }

            LabelText { text: qsTr("Message") }
            BodyText {
                Layout.fillWidth: true
                text: root.valueOr(backend["message"], qsTr("No message"))
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: root.width
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Panel {
                    Layout.fillWidth: true
                    implicitHeight: sessionGrid.implicitHeight + 28

                    GridLayout {
                        id: sessionGrid
                        anchors.fill: parent
                        anchors.margins: 14
                        columns: 2
                        rowSpacing: 8
                        columnSpacing: 14

                        Text {
                            Layout.columnSpan: 2
                            text: qsTr("Session")
                            color: "#17313c"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                        }

                        LabelText { text: qsTr("Agent") }
                        BodyText {
                            text: controller.connected ? qsTr("Connected") : qsTr("Disconnected")
                        }

                        LabelText { text: qsTr("Session type") }
                        BodyText {
                            text: root.valueOr(root.statusMap()["session_type"], qsTr("unknown"))
                        }

                        LabelText { text: qsTr("Active backend") }
                        BodyText {
                            text: root.valueOr(root.statusMap()["active_backend"], qsTr("Unavailable"))
                        }
                    }
                }

                Panel {
                    Layout.fillWidth: true
                    implicitHeight: capabilityGrid.implicitHeight + 28

                    GridLayout {
                        id: capabilityGrid
                        anchors.fill: parent
                        anchors.margins: 14
                        columns: 2
                        rowSpacing: 8
                        columnSpacing: 14

                        Text {
                            Layout.columnSpan: 2
                            text: qsTr("Capabilities")
                            color: "#17313c"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                        }

                        LabelText { text: qsTr("Global hotkey") }
                        BodyText { text: root.yesNo(root.capabilities()["global_hotkey"]) }

                        LabelText { text: qsTr("Window list") }
                        BodyText { text: root.yesNo(root.capabilities()["window_list"]) }

                        LabelText { text: qsTr("Activate window") }
                        BodyText { text: root.yesNo(root.capabilities()["activate_window"]) }

                        LabelText { text: qsTr("Launch app") }
                        BodyText { text: root.yesNo(root.capabilities()["launch_app"]) }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                BackendPanel {
                    title: qsTr("Hotkey Backend")
                    backend: root.backendMap("hotkey_backend")
                }

                BackendPanel {
                    title: qsTr("Window Backend")
                    backend: root.backendMap("window_backend")
                }
            }

            Panel {
                Layout.fillWidth: true
                implicitHeight: bindingsColumn.implicitHeight + 28

                ColumnLayout {
                    id: bindingsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 14
                    spacing: 10

                    Text {
                        text: qsTr("Binding Status")
                        color: "#17313c"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    LabelText {
                        Layout.fillWidth: true
                        visible: root.bindingStatuses().length === 0
                        text: qsTr("No bindings reported by the agent.")
                    }

                    Repeater {
                        model: root.bindingStatuses()

                        delegate: Rectangle {
                            required property var modelData

                            Layout.fillWidth: true
                            implicitHeight: bindingRow.implicitHeight + 18
                            radius: 12
                            color: "#ffffff"
                            border.width: 1
                            border.color: "#e2ebef"

                            RowLayout {
                                id: bindingRow
                                anchors.fill: parent
                                anchors.margins: 9
                                spacing: 10

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    BodyText {
                                        Layout.fillWidth: true
                                        text: root.valueOr(modelData["id"], qsTr("Unnamed binding"))
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    LabelText {
                                        Layout.fillWidth: true
                                        text: root.valueOr(modelData["hotkey"], qsTr("No hotkey"))
                                            + " | "
                                            + root.valueOr(modelData["desktop_id"], qsTr("No desktop id"))
                                    }
                                }

                                StatusPill {
                                    status: root.valueOr(modelData["status"], "unknown")
                                    label: root.valueOr(modelData["status"], qsTr("unknown"))
                                }
                            }
                        }
                    }
                }
            }

            Panel {
                Layout.fillWidth: true
                implicitHeight: warningsColumn.implicitHeight + 28

                ColumnLayout {
                    id: warningsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 14
                    spacing: 8

                    Text {
                        text: qsTr("Raw Warnings")
                        color: "#17313c"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    LabelText {
                        Layout.fillWidth: true
                        visible: root.warnings().length === 0
                        text: qsTr("No warnings reported.")
                    }

                    Repeater {
                        model: root.warnings()

                        delegate: Text {
                            required property string modelData

                            Layout.fillWidth: true
                            text: modelData
                            color: "#93421e"
                            font.family: "monospace"
                            font.pixelSize: 12
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }
            }
        }
    }
}
