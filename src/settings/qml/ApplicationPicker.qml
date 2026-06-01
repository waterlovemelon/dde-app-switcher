import QtQuick
import QtQuick.Layouts

Item {
    id: picker

    property var applications: []
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

    function isHiddenApplication(application) {
        return application.hidden === true || application.no_display === true
    }

    function searchableText(application) {
        return [
            application.localized_name,
            application.name,
            application.desktop_id,
            application.exec
        ].join(" ").toLocaleLowerCase()
    }

    function applicationMatches(application, query) {
        if (isHiddenApplication(application)) {
            return false
        }
        return query.length === 0 || searchableText(application).indexOf(query) !== -1
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
        color: "#8a8f99"
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        StyledTextField {
            id: searchField

            Layout.fillWidth: true
            placeholderText: qsTr("搜索应用名称、desktop id、命令")
            onTextChanged: picker.refreshFilter()
        }

        MutedText {
            Layout.fillWidth: true
            text: qsTr("%1 个应用").arg(picker.resultCount)
        }

        ListView {
            id: applicationList

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: picker.filteredApplications

            delegate: Rectangle {
                required property var modelData

                width: applicationList.width
                height: 58
                radius: 10
                color: rowMouse.containsMouse
                       ? "#f2f8f8"
                       : modelData.desktop_id === picker.selectedDesktopId ? "#e7f4f3" : "#ffffff"
                border.width: 1
                border.color: modelData.desktop_id === picker.selectedDesktopId ? "#8fc9c2" : "#e5e8eb"

                MouseArea {
                    id: rowMouse

                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: picker.applicationSelected(modelData)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 34
                        radius: 8
                        color: "#e0f2f1"

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 8
                            horizontalAlignment: Text.AlignHCenter
                            text: picker.textValue(modelData.localized_name || modelData.name, "?").charAt(0).toLocaleUpperCase()
                            color: "#00695c"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: picker.textValue(modelData.localized_name || modelData.name, qsTr("未命名应用"))
                            color: "#1a1a1a"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        MutedText {
                            Layout.fillWidth: true
                            text: picker.textValue(modelData.desktop_id, qsTr("无 desktop id"))
                        }
                    }
                }
            }

            footer: Item {
                width: applicationList.width
                height: picker.filteredApplications.length === 0 ? 150 : 0

                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    visible: picker.filteredApplications.length === 0
                    spacing: 8

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: picker.applications.length === 0 ? qsTr("未加载应用") : qsTr("没有匹配的应用")
                        color: "#1a1a1a"
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }

                    MutedText {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: picker.applications.length === 0 ? qsTr("启动代理后刷新应用列表。") : qsTr("换个关键词试试。")
                    }
                }
            }
        }
    }
}
