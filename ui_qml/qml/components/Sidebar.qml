import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Rectangle {
    id: root
    Layout.preferredWidth: Theme.sidebarWidth
    Layout.fillHeight: true
    color: Theme.bgSidebar

    property int currentIndex: 0
    signal pageChanged(int index)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Logo 区域
        Item {
            Layout.preferredHeight: 64
            Layout.fillWidth: true
            Layout.leftMargin: 20

            RowLayout {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                Rectangle {
                    width: 32; height: 32; radius: 8
                    color: Theme.accent
                    Label {
                        anchors.centerIn: parent
                        text: "M"
                        font.pixelSize: 18
                        font.bold: true
                        color: "white"
                    }
                }
                Label {
                    text: "MiniSense"
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    color: Theme.textPrimary
                }
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            height: 1
            color: Theme.separator
        }

        // 导航按钮
        Repeater {
            model: ListModel {
                ListElement { label: "\u2601  \u6A21\u578B\u5BFC\u5165";   page: 0 }
                ListElement { label: "\u{1F5A5}  \u753B\u9762\u6E90\u9884\u89C8"; page: 1 }
                ListElement { label: "\u{1F3AF}  \u7784\u51C6\u53C2\u6570";   page: 2 }
                ListElement { label: "\u2699  \u7CFB\u7EDF\u8BBE\u7F6E";   page: 3 }
            }
            delegate: SidebarButton {
                text: label
                checked: root.currentIndex === page
                Layout.fillWidth: true
                onClicked: {
                    root.currentIndex = page
                    root.pageChanged(page)
                }
            }
        }

        Item { Layout.fillHeight: true }

        // ── 信息面板 ──
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            radius: Theme.radiusMedium
            color: Theme.bgCard
            border.color: Theme.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                // 到期信息
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "\u5230\u671F"
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: "2027-6-30"
                        font.pixelSize: 10
                        font.family: "Consolas"
                        color: Theme.textSecondary
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "\u5269\u4F59"
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: "365 \u5929"
                        font.pixelSize: 10
                        font.family: "Consolas"
                        color: Theme.success
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.separator }

                // 输入模式
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "\u8F93\u5165"
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: {
                            var modes = ["SendInput", "IbInput", "KMBoxNet", "MaKcu"]
                            return modes[engine.config.mouseBackend] || "SendInput"
                        }
                        font.pixelSize: 10
                        font.family: "Consolas"
                        color: Theme.accent
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.separator }

                // FPS / 推理延迟
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "FPS"
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: engine.fps.toFixed(0)
                        font.pixelSize: 10
                        font.family: "Consolas"
                        color: engine.running ? Theme.success : Theme.textMuted
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "\u63A8\u7406"
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: engine.inferMs.toFixed(1) + "ms"
                        font.pixelSize: 10
                        font.family: "Consolas"
                        color: engine.running ? Theme.accent : Theme.textMuted
                    }
                }
            }
        }

        // 底部状态 + 版本
        ColumnLayout {
            Layout.margins: 16
            spacing: 8

            StatusIndicator {
                running: engine.running
            }

            Label {
                text: "v2.1"
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
