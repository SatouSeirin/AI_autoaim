import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

ColumnLayout {
    id: root
    property string label: ""
    property string text: ""
    property string placeholderText: ""
    property alias readOnly: textField.readOnly

    spacing: 2

    Label {
        text: root.label
        font.pixelSize: Theme.fontSmall
        color: Theme.textSecondary
        visible: root.label !== ""
    }

    TextField {
        id: textField
        Layout.fillWidth: true
        implicitHeight: 34
        text: root.text
        placeholderText: root.placeholderText
        color: Theme.textPrimary
        font.pixelSize: Theme.fontMedium
        font.family: "Consolas"
        leftPadding: 10
        rightPadding: 10

        background: Rectangle {
            radius: Theme.radiusMedium
            color: Theme.bgInput
            border.color: textField.activeFocus ? Theme.accent : Theme.border
            border.width: 1
        }

        // 占位符颜色
        PlaceholderText {
            color: Theme.textMuted
        }
    }
}
