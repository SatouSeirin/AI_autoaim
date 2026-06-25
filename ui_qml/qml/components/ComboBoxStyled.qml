import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

ColumnLayout {
    id: root
    property string label: ""
    property var model: []
    property int currentIndex: 0
    spacing: 4

    signal activated(int index)

    Label {
        text: root.label
        font.pixelSize: Theme.fontSmall
        color: Theme.textSecondary
        visible: root.label !== ""
    }

    ComboBox {
        id: comboBox
        Layout.fillWidth: true
        implicitHeight: 34
        model: root.model
        currentIndex: root.currentIndex
        onActivated: {
            root.currentIndex = index
            root.activated(index)
        }

        // 深色 palette（影响 popup 背景）
        palette.button: Theme.bgInput
        palette.base: Theme.bgCard
        palette.window: Theme.bgCard
        palette.text: Theme.textPrimary
        palette.buttonText: Theme.textPrimary
        palette.highlightedText: Theme.accent

        delegate: ItemDelegate {
            required property int index
            required property string modelData
            width: comboBox.width
            implicitHeight: 32
            highlighted: comboBox.highlightedIndex === index
            text: modelData
            palette.text: comboBox.currentIndex === index ? Theme.accent : Theme.textPrimary
            palette.highlightedText: Theme.accent
            palette.base: "transparent"
            palette.highlight: Theme.bgHover
            font.pixelSize: Theme.fontMedium
            font.weight: comboBox.currentIndex === index ? Font.DemiBold : Font.Normal
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: comboBox.currentIndex === index ? Theme.accent : Theme.textPrimary
                verticalAlignment: Text.AlignVCenter
                leftPadding: 8
                elide: Text.ElideRight
            }
            background: Rectangle {
                color: parent.highlighted ? Theme.bgHover : "transparent"
                radius: Theme.radiusSmall
            }
        }

        contentItem: Label {
            text: comboBox.displayText
            font.pixelSize: Theme.fontMedium
            color: Theme.textPrimary
            verticalAlignment: Text.AlignVCenter
            leftPadding: 12
            elide: Text.ElideRight
        }

        background: Rectangle {
            implicitHeight: 34
            radius: Theme.radiusMedium
            color: Theme.bgInput
            border.color: comboBox.pressed || comboBox.down ? Theme.accent : Theme.border
            border.width: 1

            Label {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: "\u25BC"
                font.pixelSize: 10
                color: Theme.textMuted
            }
        }

        // popup 样式
        popup.background: Rectangle {
            radius: Theme.radiusMedium
            color: Theme.bgCard
            border.color: Theme.border
            border.width: 1
        }
    }
}
