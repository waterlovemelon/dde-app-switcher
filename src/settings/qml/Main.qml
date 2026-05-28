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

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: autostartContent.implicitHeight + 22
                    radius: 14
                    color: "#edf4f6"
                    border.width: 1
                    border.color: "#d4e2e7"

                    RowLayout {
                        id: autostartContent

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 12
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Launch agent at login")
                                color: "#18333f"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                wrapMode: Text.WordWrap
                            }

                            MutedText {
                                Layout.fillWidth: true
                                text: qsTr("Uses your user autostart folder.")
                                wrapMode: Text.WordWrap
                            }
                        }

                        Switch {
                            id: autostartSwitch

                            checked: settingsController.autostartEnabled
                            onToggled: {
                                if (checked !== settingsController.autostartEnabled) {
                                    settingsController.setAutostartEnabled(checked)
                                }
                            }
                        }
                    }
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
                    BindingsPage {
                        anchors.fill: parent
                        controller: settingsController
                    }
                }

                SectionCard {
                    ApplicationPicker {
                        anchors.fill: parent
                        anchors.margins: 18
                        applications: settingsController.applications
                        selectable: false
                    }
                }

                SectionCard {
                    BackendStatusPage {
                        anchors.fill: parent
                        anchors.margins: 18
                        controller: settingsController
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
