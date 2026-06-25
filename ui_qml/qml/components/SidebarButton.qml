import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Rectangle {
    id: root
    implicitHeight: 44
    color: checked ? Theme.accentDim : (hovered ? Theme.bgHover : "transparent")
    radius: Theme.radiusMedium

    property string text: ""
    property bool checked: false
    property bool hovered: false
    signal clicked()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        spacing: 8

        Label {
            text: root.text
            font.pixelSize: Theme.fontMedium
            color: root.checked ? Theme.accent : Theme.textSecondary
            Layout.fillWidth: true
        }

        // 选中指示条
        Rectangle {
            visible: root.checked
            width: 3
            Layout.fillHeight: true
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            radius: 2
            color: Theme.accent
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onEntered: root.hovered = true
        onExited: root.hovered = false
        onClicked: root.clicked()
    }

    Behavior on color { ColorAnimation { duration: 150 } }
}
