import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../theme"
import "../components"

Flickable {
    id: root
    contentHeight: contentCol.implicitHeight + 40
    clip: true
    boundsMovement: Flickable.StopAtBounds

    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    ColumnLayout {
        id: contentCol
        anchors.fill: parent
        anchors.margins: 20
        spacing: Theme.spacingMedium

        // ── 热键设置 ──
        SectionHeader { text: "\u5FEB\u6377\u952E" }

        CardBackground {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                RowLayout {
                    Label { text: "\u5F00\u59CB/\u505C\u6B62"; Layout.fillWidth: true; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                    HotkeyCapture {
                        keyCode: engine.config.startStopKey
                        onKeyCaptured: (code, name) => engine.config.startStopKey = code
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.separator }

                RowLayout {
                    Label { text: "\u7784\u51C6\u952E"; Layout.fillWidth: true; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                    HotkeyCapture {
                        keyCode: engine.config.aimKey
                        onKeyCaptured: (code, name) => engine.config.aimKey = code
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.separator }

                RowLayout {
                    Label { text: "\u9000\u51FA"; Layout.fillWidth: true; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                    HotkeyCapture {
                        keyCode: engine.config.exitKey
                        onKeyCaptured: (code, name) => engine.config.exitKey = code
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.separator }

                RowLayout {
                    Label { text: "\u5207\u6362\u76EE\u6807"; Layout.fillWidth: true; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                    HotkeyCapture {
                        keyCode: engine.config.switchTargetKey
                        onKeyCaptured: (code, name) => engine.config.switchTargetKey = code
                    }
                }
            }
        }

        // ── 鼠标后端 ──
        SectionHeader { text: "\u8F93\u5165\u6A21\u5F0F" }

        CardBackground {
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                // 左侧：方向测试按钮
                ColumnLayout {
                    spacing: 6
                    Layout.preferredWidth: 80

                    Label {
                        text: "\u6D4B\u8BD5\u79FB\u52A8"
                        font.pixelSize: Theme.fontSmall
                        color: Theme.textMuted
                    }

                    // 上
                    ButtonStyled {
                        Layout.fillWidth: true
                        text: "\u2191 \u4E0A"
                        type: "secondary"
                        onClicked: engine.testMove("up")
                    }

                    RowLayout {
                        spacing: 4
                        ButtonStyled {
                            Layout.fillWidth: true
                            text: "\u2190"
                            type: "secondary"
                            onClicked: engine.testMove("left")
                        }
                        ButtonStyled {
                            Layout.fillWidth: true
                            text: "\u2192"
                            type: "secondary"
                            onClicked: engine.testMove("right")
                        }
                    }

                    // 下
                    ButtonStyled {
                        Layout.fillWidth: true
                        text: "\u2193 \u4E0B"
                        type: "secondary"
                        onClicked: engine.testMove("down")
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: Theme.separator
                }

                // 右侧：后端设置
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMedium

                    ComboBoxStyled {
                        id: backendCombo
                        Layout.fillWidth: true
                        label: "\u9F20\u6807\u540E\u7AEF"
                        model: ["SendInput", "IbInputSimulator", "KMBoxNet", "MaKcu"]
                        currentIndex: engine.config.mouseBackend
                        onActivated: engine.config.mouseBackend = index
                    }

                    // KMBoxNet 设置
                    ColumnLayout {
                        visible: engine.config.mouseBackend === 2
                        Layout.fillWidth: true
                        spacing: Theme.spacingSmall

                        Label {
                            text: "KMBoxNet \u8BBE\u7F6E"
                            font.pixelSize: Theme.fontSmall
                            color: Theme.textMuted
                        }

                        RowLayout {
                            Label { text: "IP"; Layout.preferredWidth: 50; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                            TextField {
                                Layout.fillWidth: true
                                leftPadding: 10; rightPadding: 10
                                text: engine.config.kmboxIp
                                onEditingFinished: engine.config.kmboxIp = text
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontMedium
                                background: Rectangle { radius: Theme.radiusMedium; color: Theme.bgInput; border.color: parent.activeFocus ? Theme.accent : "#3A3A55"; border.width: 1; implicitHeight: 34 }
                            }
                        }

                        RowLayout {
                            Label { text: "Port"; Layout.preferredWidth: 50; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                            TextField {
                                Layout.fillWidth: true
                                leftPadding: 10; rightPadding: 10
                                text: engine.config.kmboxPort
                                onEditingFinished: engine.config.kmboxPort = text
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontMedium
                                background: Rectangle { radius: Theme.radiusMedium; color: Theme.bgInput; border.color: parent.activeFocus ? Theme.accent : "#3A3A55"; border.width: 1; implicitHeight: 34 }
                            }
                        }

                        RowLayout {
                            Label { text: "UUID"; Layout.preferredWidth: 50; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                            TextField {
                                Layout.fillWidth: true
                                leftPadding: 10; rightPadding: 10
                                text: engine.config.kmboxMac
                                onEditingFinished: engine.config.kmboxMac = text
                                placeholderText: "MAC \u5730\u5740"
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontMedium
                                background: Rectangle { radius: Theme.radiusMedium; color: Theme.bgInput; border.color: parent.activeFocus ? Theme.accent : "#3A3A55"; border.width: 1; implicitHeight: 34 }
                            }
                        }

                        ButtonStyled {
                            Layout.fillWidth: true
                            text: "\u8FDE\u63A5 KMBoxNet"
                            type: "primary"
                            onClicked: {
                                // 触发配置同步
                                engine.applyConfig()
                            }
                        }
                    }

                    // MaKcu 设置
                    ColumnLayout {
                        visible: engine.config.mouseBackend === 3
                        Layout.fillWidth: true
                        spacing: Theme.spacingSmall

                        Label {
                            text: "MaKcu \u8BBE\u7F6E"
                            font.pixelSize: Theme.fontSmall
                            color: Theme.textMuted
                        }

                        RowLayout {
                            Label { text: "\u4E32\u53E3"; Layout.preferredWidth: 50; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                            TextField {
                                Layout.fillWidth: true
                                leftPadding: 10; rightPadding: 10
                                text: engine.config.makcuSerial
                                onEditingFinished: engine.config.makcuSerial = text
                                placeholderText: "COM\u7AEF\u53E3"
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontMedium
                                background: Rectangle { radius: Theme.radiusMedium; color: Theme.bgInput; border.color: parent.activeFocus ? Theme.accent : "#3A3A55"; border.width: 1; implicitHeight: 34 }
                            }
                        }

                        RowLayout {
                            Label { text: "\u6CE2\u7279\u7387"; Layout.preferredWidth: 50; color: Theme.textSecondary; font.pixelSize: Theme.fontMedium }
                            ComboBoxStyled {
                                Layout.fillWidth: true
                                model: ["9600", "19200", "38400", "57600", "115200"]
                                currentIndex: 4
                            }
                        }

                        ButtonStyled {
                            Layout.fillWidth: true
                            text: "\u8FDE\u63A5 MaKcu"
                            type: "primary"
                            onClicked: {
                                engine.applyConfig()
                            }
                        }
                    }
                }
            }
        }

        // ── 配置档案 ──
        SectionHeader { text: "\u914D\u7F6E\u6863\u6848" }

        CardBackground {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                RowLayout {
                    spacing: 8

                    ButtonStyled {
                        text: "\u4FDD\u5B58\u914D\u7F6E"
                        type: "primary"
                        onClicked: {
                            saveFileDialog.open()
                        }
                    }

                    ButtonStyled {
                        text: "\u52A0\u8F7D\u914D\u7F6E"
                        type: "secondary"
                        onClicked: {
                            loadFileDialog.open()
                        }
                    }

                    ButtonStyled {
                        text: "\u6062\u590D\u9ED8\u8BA4"
                        type: "danger"
                        onClicked: {
                            resetConfirmDialog.open()
                        }
                    }
                }
            }
        }

        // ── 通用设置 ──
        SectionHeader { text: "\u901A\u7528\u8BBE\u7F6E" }

        CardBackground {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                ToggleSwitch {
                    Layout.fillWidth: true
                    text: "\u81EA\u52A8\u542F\u52A8\u63A8\u7406"
                    checked: engine.config.autoStart
                    onToggled: (v) => engine.config.autoStart = v
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.separator }

                ToggleSwitch {
                    Layout.fillWidth: true
                    text: "\u6700\u5C0F\u5316\u5230\u7CFB\u7EDF\u6258\u76D8"
                    checked: engine.config.minimizeToTray
                    onToggled: (v) => engine.config.minimizeToTray = v
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.separator }

                ComboBoxStyled {
                    id: langCombo
                    Layout.fillWidth: true
                    label: "\u8BED\u8A00"
                    model: ["\u4E2D\u6587", "English"]
                    currentIndex: engine.config.language === "zh-CN" ? 0 : 1
                    onActivated: engine.config.language = (index === 0) ? "zh-CN" : "en-US"
                }
            }
        }

        // ── 关于 ──
        SectionHeader { text: "\u5173\u4E8E" }

        CardBackground {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Label {
                    text: "MiniSense v2.1"
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.Bold
                    color: Theme.accent
                }

                Label {
                    text: "TensorRT 10.x + YOLO \u5B9E\u65F6\u81EA\u7784"
                    font.pixelSize: Theme.fontMedium
                    color: Theme.textSecondary
                }

                Label {
                    text: "Qt 6.8.2 QML | C++17 | CUDA 13.x"
                    font.pixelSize: Theme.fontSmall
                    color: Theme.textMuted
                }
            }
        }

        // 底部间距
        Item { Layout.preferredHeight: 20 }
    }

    // 保存文件对话框
    FileDialog {
        id: saveFileDialog
        fileMode: FileDialog.SaveFile
        title: "\u4FDD\u5B58\u914D\u7F6E"
        nameFilters: ["JSON \u6587\u4EF6 (*.json)"]
        onAccepted: {
            var path = selectedFile.toString()
            if (path.indexOf("file:///") === 0) path = path.substring(8)
            if (Qt.platform.os === "windows") path = path.replace(/\//g, "\\")
            engine.saveProfile(path)
        }
    }

    // 加载文件对话框
    FileDialog {
        id: loadFileDialog
        fileMode: FileDialog.OpenFile
        title: "\u52A0\u8F7D\u914D\u7F6E"
        nameFilters: ["JSON \u6587\u4EF6 (*.json)"]
        onAccepted: {
            var path = selectedFile.toString()
            if (path.indexOf("file:///") === 0) path = path.substring(8)
            if (Qt.platform.os === "windows") path = path.replace(/\//g, "\\")
            engine.loadProfile(path)
        }
    }

    // 恢复默认确认对话框
    MessageDialog {
        id: resetConfirmDialog
        title: "\u6062\u590D\u9ED8\u8BA4\u914D\u7F6E"
        text: "\u786E\u5B9A\u8981\u6062\u590D\u6240\u6709\u914D\u7F6E\u4E3A\u9ED8\u8BA4\u503C\u5417\uFF1F\u6B64\u64CD\u4F5C\u4E0D\u53EF\u64A4\u9500\u3002"
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function(button, role, source) {
            if (button === MessageDialog.Yes) {
                engine.resetConfig()
            }
        }
    }

}
