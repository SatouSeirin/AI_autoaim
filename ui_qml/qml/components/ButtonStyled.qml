import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Rectangle {
    id: root
    implicitHeight: 36
    implicitWidth: Math.max(80, label.implicitWidth + 24)
    radius: Theme.radiusMedium
    color: root.enabled ? (pressed ? root.accentColor : (hovered ? root.hoverColor : root.accentColor)) : Theme.bgHover
    opacity: root.enabled ? 1.0 : 0.5

    property string text: ""
    property string type: "primary"  // primary, secondary, danger
    property bool hovered: false
    property bool pressed: false
    property color accentColor: type === "primary" ? Theme.accent :
                                type === "danger" ? Theme.error :
                                Theme.bgInput
    property color hoverColor: type === "primary" ? Theme.accentHover :
                               type === "danger" ? "#FF6659" :
                               Theme.bgHover

    signal clicked()

    Label {
        id: label
        anchors.centerIn: parent
        text: root.text
        font.pixelSize: Theme.fontMedium
        font.weight: Font.Medium
        color: type === "secondary" ? Theme.textPrimary : "white"
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onEntered: root.hovered = true
        onExited: { root.hovered = false; root.pressed = false }
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: root.clicked()
    }

    Behavior on color { ColorAnimation { duration: 100 } }
}
