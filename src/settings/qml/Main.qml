import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    width: 900
    height: 640
    minimumWidth: 820
    minimumHeight: 560
    visible: true
    title: qsTr("DeepSwitch Settings")
    color: "#eef3f6"

    property int selectedPage: 0
    readonly property var pages: [
        { "title": qsTr("Bindings"), "subtitle": qsTr("Hotkeys that switch or launch applications.") },
        { "title": qsTr("Applications"), "subtitle": qsTr("Detected desktop entries from the agent.") },
        { "title": qsTr("Backend Status"), "subtitle": qsTr("Runtime status for window and hotkey backends.") },
        { "title": qsTr("About"), "subtitle": qsTr("DeepSwitch desktop utility.") }
    ]

    background: Rectangle {
        color: "#eef3f6"

        Rectangle {
            width: 360
            height: parent.height
            color: "#dce9ed"
            opacity: 0.55
            radius: 48
            anchors.left: parent.left
            anchors.leftMargin: -96
            anchors.top: parent.top
            anchors.topMargin: -40
        }

        Rectangle {
            width: 520
            height: 180
            radius: 90
            color: "#f8fbfc"
            opacity: 0.75
            anchors.right: parent.right
            anchors.rightMargin: -140
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -56
        }
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

    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        SectionCard {
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            color: "#f7fafb"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 16

                ColumnLayout {
                    spacing: 4
                    Layout.fillWidth: true

                    Text {
                        text: qsTr("DeepSwitch")
                        color: "#18333f"
                        font.pixelSize: 24
                        font.weight: Font.DemiBold
                    }

                    MutedText {
                        text: qsTr("Application switcher")
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: window.pages

                    delegate: Button {
                        id: navButton

                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        text: modelData.title
                        checkable: true
                        checked: window.selectedPage === index
                        onClicked: window.selectedPage = index

                        contentItem: Text {
                            text: navButton.text
                            color: navButton.checked ? "#08313e" : "#405863"
                            font.pixelSize: 14
                            font.weight: navButton.checked ? Font.DemiBold : Font.Normal
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: 12
                            color: navButton.checked ? "#d9ecef" : (navButton.hovered ? "#edf4f6" : "transparent")
                            border.width: navButton.checked ? 1 : 0
                            border.color: "#b8d4db"
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Refresh Agent")
                    onClicked: settingsController.refresh()
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            SectionCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 96

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            text: window.pages[window.selectedPage].title
                            color: "#142c36"
                            font.pixelSize: 28
                            font.weight: Font.DemiBold
                        }

                        MutedText {
                            Layout.fillWidth: true
                            text: window.pages[window.selectedPage].subtitle
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: statusText.implicitWidth + 34
                        Layout.preferredHeight: 34
                        radius: 17
                        color: settingsController.connected ? "#dff3e7" : "#f6e4dc"
                        border.width: 1
                        border.color: settingsController.connected ? "#9ed5b8" : "#e2b199"

                        Text {
                            id: statusText
                            anchors.centerIn: parent
                            text: settingsController.connected ? qsTr("Agent connected") : qsTr("Agent disconnected")
                            color: settingsController.connected ? "#1f6c43" : "#93421e"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: window.selectedPage

                SectionCard {
                    ListView {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 10
                        clip: true
                        model: settingsController.bindings.length > 0 ? settingsController.bindings : [ { "id": qsTr("No bindings loaded"), "hotkey": qsTr("Later tasks will add editing."), "desktop_id": "" } ]

                        delegate: SectionCard {
                            required property var modelData

                            width: ListView.view.width
                            height: 72
                            color: "#ffffff"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 14

                                Rectangle {
                                    Layout.preferredWidth: 42
                                    Layout.preferredHeight: 42
                                    radius: 12
                                    color: "#e5f0f3"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "DS"
                                        color: "#225365"
                                        font.pixelSize: 18
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.id || qsTr("Unnamed binding")
                                        color: "#17313c"
                                        font.pixelSize: 15
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    MutedText {
                                        Layout.fillWidth: true
                                        text: modelData.desktop_id || qsTr("Application target not set")
                                    }
                                }

                                Label {
                                    text: modelData.hotkey || qsTr("Unassigned")
                                    color: "#214a58"
                                    padding: 8
                                    background: Rectangle {
                                        radius: 8
                                        color: "#eef6f8"
                                        border.color: "#d4e4e9"
                                    }
                                }
                            }
                        }
                    }
                }

                SectionCard {
                    ListView {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 10
                        clip: true
                        model: settingsController.applications.length > 0 ? settingsController.applications : [ { "name": qsTr("No applications loaded"), "desktop_id": qsTr("Start the agent and refresh to scan desktop entries.") } ]

                        delegate: SectionCard {
                            required property var modelData

                            width: ListView.view.width
                            height: 66
                            color: "#ffffff"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 2

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.localized_name || modelData.name || qsTr("Unnamed application")
                                    color: "#17313c"
                                    font.pixelSize: 15
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }

                                MutedText {
                                    Layout.fillWidth: true
                                    text: modelData.desktop_id || ""
                                }
                            }
                        }
                    }
                }

                SectionCard {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 22
                        spacing: 14

                        Text {
                            text: qsTr("Runtime")
                            color: "#17313c"
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 10
                            columnSpacing: 18

                            MutedText { text: qsTr("Agent") }
                            Text {
                                text: settingsController.connected ? qsTr("Connected") : qsTr("Disconnected")
                                color: "#17313c"
                                font.pixelSize: 14
                            }

                            MutedText { text: qsTr("Active backend") }
                            Text {
                                text: settingsController.status["active_backend"] || qsTr("Unavailable")
                                color: "#17313c"
                                font.pixelSize: 14
                            }

                            MutedText { text: qsTr("Backend message") }
                            Text {
                                Layout.fillWidth: true
                                text: settingsController.backendStatus["message"] || settingsController.lastError || qsTr("No status message")
                                color: "#17313c"
                                font.pixelSize: 14
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                SectionCard {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 10

                        Text {
                            text: qsTr("DeepSwitch")
                            color: "#17313c"
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                        }

                        MutedText {
                            Layout.fillWidth: true
                            text: qsTr("A small desktop utility for hotkey-driven application switching.")
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("This shell exposes the controller state and reserves space for the binding editor and application picker planned in later tasks.")
                            color: "#405863"
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }
}
