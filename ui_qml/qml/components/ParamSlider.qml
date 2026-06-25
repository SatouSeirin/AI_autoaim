import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

ColumnLayout {
    id: root
    property string label: ""
    property real value: 0
    property real from: 0
    property real to: 1
    property real stepSize: 0.01
    property string suffix: ""
    property int decimals: 2
    property string unit: ""

    signal valueModified(real newValue)

    spacing: 4

    RowLayout {
        Layout.fillWidth: true

        Label {
            text: root.label
            font.pixelSize: Theme.fontMedium
            color: Theme.textSecondary
            Layout.fillWidth: true
        }

        // 数值显示
        Rectangle {
            implicitWidth: 70
            implicitHeight: 26
            radius: Theme.radiusSmall
            color: Theme.bgInput
            border.color: Theme.border
            border.width: 1

            Label {
                anchors.centerIn: parent
                text: root.value.toFixed(root.decimals) + root.suffix
                font.pixelSize: Theme.fontSmall
                color: Theme.textPrimary
            }
        }
    }

    // 使用标准 Slider 控件 + 深色主题定制
    Slider {
        id: slider
        Layout.fillWidth: true
        from: root.from
        to: root.to
        value: root.value
        stepSize: root.stepSize

        onValueChanged: {
            root.value = value
            root.valueModified(value)
        }

        // 轨道样式
        background: Rectangle {
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitHeight: 4
            width: slider.availableWidth
            height: implicitHeight
            radius: 2
            color: Theme.bgInput

            // 已填充部分
            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: parent.radius
                color: Theme.accent
            }
        }

        // 手柄样式
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 16
            height: 16
            radius: 8
            color: Theme.accent
            border.color: Theme.accentHover
            border.width: 2
        }
    }
}
