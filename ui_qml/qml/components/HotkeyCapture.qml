import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Rectangle {
    id: root
    implicitWidth: 140
    implicitHeight: 34
    radius: Theme.radiusMedium
    color: capturing ? Theme.accentDim : Theme.bgInput
    border.color: capturing ? Theme.accent : Theme.border
    border.width: capturing ? 2 : 1

    property int keyCode: 0
    property string keyName: "\u672A\u8BBE\u7F6E"
    property bool capturing: false

    signal keyCaptured(int code, string name)

    Component.onCompleted: {
        if (keyCode > 0) keyName = formatVkKey(keyCode)
    }

    Label {
        anchors.centerIn: parent
        text: root.capturing ? "\u6309\u4E0B\u4EFB\u610F\u952E..." : root.keyName
        font.pixelSize: Theme.fontMedium
        color: root.capturing ? Theme.accent : Theme.textPrimary
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.capturing = true
            hotkeyDispatcher.beginCapture(root)
        }
    }

    Connections {
        target: hotkeyDispatcher

        function onKeyCaptured(vkCode) {
            if (root.capturing) {
                root.keyCode = vkCode
                root.keyName = formatVkKey(vkCode)
                root.capturing = false
                root.keyCaptured(vkCode, root.keyName)
            }
        }

        function onCapturingChanged() {
            // 如果调度器不再捕获（被取消），退出捕获状态
            if (!hotkeyDispatcher.capturing && root.capturing) {
                root.capturing = false
            }
        }
    }

    // VK 码 → 显示名称
    function formatVkKey(vk) {
        var map = {
            0x01: "\u9F20\u6807\u5DE6\u952E", 0x02: "\u9F20\u6807\u53F3\u952E", 0x04: "\u9F20\u6807\u4E2D\u952E",
            0x05: "\u9F20\u6807\u4FA71", 0x06: "\u9F20\u6807\u4FA72",
            0x08: "Backspace", 0x09: "Tab", 0x0D: "Enter", 0x20: "Space",
            0x10: "Shift", 0x11: "Ctrl", 0x12: "Alt", 0x14: "CapsLock",
            0x1B: "Esc", 0x2E: "Delete", 0x2D: "Insert",
            0x24: "Home", 0x23: "End", 0x21: "PageUp", 0x22: "PageDown",
            0x25: "\u2190", 0x26: "\u2191", 0x27: "\u2192", 0x28: "\u2193",
            0x70: "F1", 0x71: "F2", 0x72: "F3", 0x73: "F4",
            0x74: "F5", 0x75: "F6", 0x76: "F7", 0x77: "F8",
            0x78: "F9", 0x79: "F10", 0x7A: "F11", 0x7B: "F12"
        }
        if (map[vk]) return map[vk]
        if (vk >= 0x30 && vk <= 0x39) return String.fromCharCode(vk)
        if (vk >= 0x41 && vk <= 0x5A) return String.fromCharCode(vk)
        return "VK_" + vk
    }

    Behavior on border.color { ColorAnimation { duration: 150 } }
    Behavior on color { ColorAnimation { duration: 150 } }
}
