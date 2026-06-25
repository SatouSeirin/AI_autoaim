import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Rectangle {
    id: root
    implicitHeight: 120
    radius: Theme.radiusLarge
    color: dragArea.containsDrag ? Theme.accentDim : Theme.bgCard
    border.color: dragArea.containsDrag ? Theme.accent : Theme.border
    border.width: dragArea.containsDrag ? 2 : 1

    property bool hovered: false

    signal fileDropped(string filePath)
    signal browseClicked()

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 8

        // 上传图标（用矩形+箭头模拟）
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 48; height: 48; radius: 24
            color: Theme.bgInput
            border.color: Theme.border
            border.width: 1

            Label {
                anchors.centerIn: parent
                text: "\u2B06"
                font.pixelSize: 20
                color: Theme.accent
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "\u62D6\u62FD .onnx \u6216 .engine \u6587\u4EF6\u5230\u8FD9\u91CC"
            font.pixelSize: Theme.fontMedium
            color: Theme.textSecondary
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "\u6216\u70B9\u51FB\u6D4F\u89C8\u6587\u4EF6"
            font.pixelSize: Theme.fontSmall
            color: Theme.textMuted
        }
    }

    DropArea {
        id: dragArea
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (drop) => {
            if (drop.hasUrls) {
                var path = drop.urls[0].toString()
                // 处理 Windows 文件路径
                if (path.indexOf("file:///") === 0) {
                    path = path.substring(8)
                }
                // Windows 路径转换
                if (Qt.platform.os === "windows") {
                    path = path.replace(/\//g, "\\")
                }
                root.fileDropped(path)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onEntered: root.hovered = true
        onExited: root.hovered = false
        onClicked: root.browseClicked()
    }

    Behavior on border.color { ColorAnimation { duration: 150 } }
    Behavior on color { ColorAnimation { duration: 150 } }
}
