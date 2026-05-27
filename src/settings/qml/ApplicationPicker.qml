import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: picker

    property var applications: []
    property bool selectable: true
    property string selectedDesktopId: ""
    property int resultCount: filteredApplications.length
    property var filteredApplications: []

    signal applicationSelected(var application)

    function textValue(value, fallback) {
        if (value === undefined || value === null || String(value).trim().length === 0) {
            return fallback
        }
        return String(value)
    }

    function categoriesText(application) {
        if (application.categories === undefined || application.categories === null) {
            return ""
        }
        if (Array.isArray(application.categories)) {
            return application.categories.join(", ")
        }
        return String(application.categories)
    }

    function isHiddenApplication(application) {
        return application.hidden === true || application.no_display === true
    }

    function searchableText(application) {
        return [
            application.localized_name,
            application.name,
            application.desktop_id,
            application.exec,
            categoriesText(application)
        ].join(" ").toLocaleLowerCase()
    }

    function applicationMatches(application, query) {
        if (!showHiddenApps.checked && isHiddenApplication(application)) {
            return false
        }
        if (query.length === 0) {
            return true
        }
        return searchableText(application).indexOf(query) !== -1
    }

    function refreshFilter() {
        var query = searchField.text.trim().toLocaleLowerCase()
        var nextApplications = []
        for (var i = 0; i < applications.length; ++i) {
            var application = applications[i]
            if (applicationMatches(application, query)) {
                nextApplications.push(application)
            }
        }
        filteredApplications = nextApplications
    }

    onApplicationsChanged: refreshFilter()
    Component.onCompleted: refreshFilter()

    component MutedText: Text {
        color: "#667985"
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    component DetailChip: Rectangle {
        property string label: ""
        property string value: ""

        visible: value.length > 0
        implicitWidth: Math.min(chipText.implicitWidth + 20, 280)
        implicitHeight: 28
        width: implicitWidth
        height: implicitHeight
        radius: 9
        color: "#eef6f8"
        border.width: 1
        border.color: "#d4e4e9"

        Text {
            id: chipText

            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            verticalAlignment: Text.AlignVCenter
            text: label + ": " + value
            color: "#214a58"
            font.pixelSize: 12
            elide: Text.ElideRight
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: searchField

                Layout.fillWidth: true
                placeholderText: qsTr("Search by name, desktop id, command, or category")
                onTextChanged: picker.refreshFilter()
            }

            CheckBox {
                id: showHiddenApps

                text: qsTr("Show hidden apps")
                onCheckedChanged: picker.refreshFilter()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            MutedText {
                Layout.fillWidth: true
                text: qsTr("%1 applications").arg(picker.resultCount)
            }
        }

        ListView {
            id: applicationList

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            model: picker.filteredApplications

            delegate: Rectangle {
                required property var modelData

                width: applicationList.width
                height: applicationCard.implicitHeight + 24
                radius: 16
                color: modelData.desktop_id === picker.selectedDesktopId ? "#e1f1f3" : "#ffffff"
                border.width: 1
                border.color: modelData.desktop_id === picker.selectedDesktopId ? "#9dcbd3" : "#d8e3e8"

                MouseArea {
                    anchors.fill: parent
                    enabled: picker.selectable
                    acceptedButtons: Qt.LeftButton
                    onDoubleClicked: picker.applicationSelected(modelData)
                    onClicked: picker.selectedDesktopId = picker.textValue(modelData.desktop_id, "")
                }

                RowLayout {
                    id: applicationCard

                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        radius: 14
                        color: "#eaf2f4"
                        border.width: 1
                        border.color: "#d4e4e9"

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 10
                            horizontalAlignment: Text.AlignHCenter
                            text: picker.textValue(modelData.icon, picker.textValue(modelData.localized_name || modelData.name, "?")).charAt(0).toLocaleUpperCase()
                            color: "#214a58"
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    Layout.fillWidth: true
                                    text: picker.textValue(modelData.localized_name || modelData.name, qsTr("Unnamed application"))
                                    color: "#17313c"
                                    font.pixelSize: 15
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }

                                MutedText {
                                    Layout.fillWidth: true
                                    text: picker.textValue(modelData.desktop_id, qsTr("No desktop id"))
                                }
                            }

                            Button {
                                visible: picker.selectable
                                text: qsTr("Select")
                                onClicked: picker.applicationSelected(modelData)
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            DetailChip {
                                label: qsTr("Exec")
                                value: picker.textValue(modelData.exec, "")
                            }

                            DetailChip {
                                label: qsTr("Icon")
                                value: picker.textValue(modelData.icon, "")
                            }

                            DetailChip {
                                label: qsTr("WM Class")
                                value: picker.textValue(modelData.startup_wm_class, "")
                            }

                            DetailChip {
                                label: qsTr("Categories")
                                value: picker.categoriesText(modelData)
                            }

                            DetailChip {
                                label: qsTr("Hidden")
                                value: picker.isHiddenApplication(modelData) ? qsTr("Yes") : ""
                            }
                        }
                    }
                }
            }

            footer: Item {
                width: applicationList.width
                height: picker.filteredApplications.length === 0 ? 160 : 0

                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    visible: picker.filteredApplications.length === 0
                    spacing: 8

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: picker.applications.length === 0 ? qsTr("No applications loaded") : qsTr("No matching applications")
                        color: "#17313c"
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    MutedText {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: picker.applications.length === 0
                              ? qsTr("Start the agent and refresh to scan desktop entries.")
                              : qsTr("Try a different search or enable hidden apps.")
                    }
                }
            }
        }
    }
}
