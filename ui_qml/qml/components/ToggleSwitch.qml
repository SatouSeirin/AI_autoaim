import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Item {
    id: root
    implicitWidth: 44
    implicitHeight: 24

    property bool checked: false
    property string text: ""

    signal toggled(bool checked)

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: root.text
            font.pixelSize: Theme.fontMedium
            color: Theme.textSecondary
            Layout.fillWidth: true
            visible: text !== ""
        }

        Rectangle {
            width: 44
            height: 24
            radius: 12
            color: root.checked ? Theme.accent : Theme.bgInput
            border.color: root.checked ? Theme.accent : Theme.border
            border.width: 1

            Rectangle {
                id: knob
                width: 18
                height: 18
                radius: 9
                color: "white"
                x: root.checked ? parent.width - width - 3 : 3
                y: (parent.height - height) / 2

                Behavior on x {
                    NumberAnimation { duration: 150; easing.type: Easing.InOutQuad }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.checked = !root.checked
                    root.toggled(root.checked)
                }
            }

            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }
}
