import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "theme"
import "components"
import "pages"

ApplicationWindow {
    id: root
    visible: true
    width: 1100
    height: 750
    minimumWidth: 900
    minimumHeight: 600
    title: "MiniSense v2.1"
    color: Theme.bgPrimary

    // 主布局：侧边栏 + 分隔线 + 内容区
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧导航
        Sidebar {
            id: sidebar
            Layout.fillHeight: true
        }

        // 侧边栏右侧分隔线
        Rectangle {
            Layout.fillHeight: true
            width: 1
            color: Theme.separator
        }

        // 右侧内容区
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // 顶部信息栏
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: Theme.bgSecondary

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20

                    StatusIndicator {
                        running: engine.running
                    }

                    Label {
                        text: engine.modelLoading ? "模型加载中..." :
                              engine.modelLoaded ? engine.currentModelName :
                                                   "尚未导入模型"
                        font.pixelSize: Theme.fontSmall
                        color: engine.modelLoading ? Theme.warning :
                              engine.modelLoaded ? Theme.success : Theme.textMuted
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: "FPS: " + engine.fps.toFixed(1)
                        font.pixelSize: Theme.fontSmall
                        font.family: "Consolas"
                        color: engine.running ? Theme.accent : Theme.textMuted
                    }
                }

                // 底部分隔线
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.separator
                }
            }

            // 页面堆叠区
            StackLayout {
                id: stackLayout
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: sidebar.currentIndex

                ModelPage {}
                PreviewPage {}
                AimPage {}
                SettingsPage {}
            }
        }
    }

    // 全局 Toast 提示（覆盖在最上层）
    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
    }

    Connections {
        target: engine
        function onProfileLoaded(success, message) {
            toast.show(message, success ? "success" : "error")
        }
        function onKmboxConnectionResult(success, message) {
            toast.show(message, success ? "success" : "error")
        }
    }
}
