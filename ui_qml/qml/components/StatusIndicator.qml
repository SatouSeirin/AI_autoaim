import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

RowLayout {
    id: root
    spacing: 6

    property bool running: false

    Rectangle {
        width: 8
        height: 8
        radius: 4
        color: root.running ? Theme.success : Theme.textMuted

        // 运行时呼吸灯效果
        SequentialAnimation on opacity {
            running: root.running
            loops: Animation.Infinite
            NumberAnimation { to: 0.4; duration: 1000; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 1.0; duration: 1000; easing.type: Easing.InOutQuad }
        }
    }

    Label {
        text: root.running ? "\u8FD0\u884C\u4E2D" : "\u5F85\u547D"
        font.pixelSize: Theme.fontSmall
        color: root.running ? Theme.success : Theme.textMuted
    }
}
