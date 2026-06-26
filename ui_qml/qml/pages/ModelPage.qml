import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls
import "../theme"
import "../components"

Flickable {
    id: root
    contentHeight: contentCol.implicitHeight + 40
    clip: true
    boundsMovement: Flickable.StopAtBounds

    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
    }

    ColumnLayout {
        id: contentCol
        anchors.fill: parent
        anchors.margins: 20
        spacing: Theme.spacingMedium

        // ── 模型导入 ──
        SectionHeader { text: "\u6A21\u578B\u5BFC\u5165" }

        CardBackground {
            Layout.fillWidth: true

            ColumnLayout {
                id: contentCol2
                Layout.fillWidth: true
                spacing: Theme.spacingMedium

                DragDropArea {
                    Layout.fillWidth: true
                    onFileDropped: (filePath) => {
                        engine.loadModel(filePath)
                    }
                    onBrowseClicked: fileDialog.open()
                }

                // 当前模型状态
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: engine.modelLoading ? Theme.warning
                             : engine.modelLoaded ? Theme.success : Theme.textMuted
                        SequentialAnimation on opacity {
                            running: engine.modelLoading
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 500 }
                            NumberAnimation { to: 1.0; duration: 500 }
                        }
                    }
                    Label {
                        text: engine.modelLoading ? "\u6A21\u578B\u52A0\u8F7D\u4E2D..."
                              : engine.modelLoaded ? engine.currentModelName : "\u672A\u5BFC\u5165\u6A21\u578B"
                        font.pixelSize: Theme.fontMedium
                        color: engine.modelLoading ? Theme.warning
                              : engine.modelLoaded ? Theme.textPrimary : Theme.textMuted
                        Layout.fillWidth: true
                    }
                    ButtonStyled {
                        text: engine.running ? "\u505C\u6B62" : "\u5F00\u59CB"
                        type: engine.running ? "danger" : "primary"
                        onClicked: engine.toggleRunning()
                        enabled: engine.modelLoaded && !engine.modelLoading
                    }
                }
            }
        }

        // ── 目标类别 ──
        SectionHeader { text: "\u76EE\u6807\u7C7B\u522B" }

        CardBackground {
            Layout.fillWidth: true

            GridLayout {
                columns: 5
                columnSpacing: 8
                rowSpacing: 8

                Repeater {
                    model: 10
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 32
                        radius: Theme.radiusSmall
                        color: engine.config.targetClassIds.includes(index) ? Theme.accentDim : Theme.bgInput
                        border.color: engine.config.targetClassIds.includes(index) ? Theme.accent : Theme.border
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: "Class " + index
                            font.pixelSize: Theme.fontSmall
                            color: engine.config.targetClassIds.includes(index) ? Theme.accent : Theme.textSecondary
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: engine.setTargetClass(index, !engine.config.targetClassIds.includes(index))
                        }
                    }
                }
            }
        }

        // ── 推理参数 ──
        SectionHeader { text: "\u63A8\u7406\u53C2\u6570" }

        CardBackground {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMedium

                ParamSlider {
                    Layout.fillWidth: true
                    label: "\u7F6E\u4FE1\u5EA6\u9608\u503C"
                    value: engine.config.confThreshold
                    from: 0.01; to: 1.0; stepSize: 0.01; decimals: 2
                    onValueModified: (v) => engine.config.confThreshold = v
                }

                ParamSlider {
                    Layout.fillWidth: true
                    label: "NMS \u9608\u503C"
                    value: engine.config.nmsThreshold
                    from: 0.01; to: 1.0; stepSize: 0.01; decimals: 2
                    onValueModified: (v) => engine.config.nmsThreshold = v
                }
            }
        }

        // 底部间距
        Item { Layout.preferredHeight: 20 }
    }

    // 文件选择对话框
    FileDialog {
        id: fileDialog
        title: "\u9009\u62E9\u6A21\u578B\u6587\u4EF6"
        nameFilters: ["\u6A21\u578B\u6587\u4EF6 (*.onnx *.engine)", "\u6240\u6709\u6587\u4EF6 (*)"]
        onAccepted: {
            var path = selectedFile.toString()
            if (path.indexOf("file:///") === 0) path = path.substring(8)
            if (Qt.platform.os === "windows") path = path.replace(/\//g, "\\")
            engine.loadModel(path)
        }
    }

    // 错误弹窗
    MessageDialog {
        id: errorDialog
        title: "\u6A21\u578B\u52A0\u8F7D\u5931\u8D25"
        buttons: MessageDialog.Ok
    }

    Connections {
        target: engine
        function onErrorOccurred(message) {
            errorDialog.text = message
            errorDialog.open()
        }
    }
}
