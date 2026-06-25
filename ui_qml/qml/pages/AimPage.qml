import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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

        // ── PID 控制器 ──
        SectionHeader { text: "PID \u63A7\u5236\u5668" }

        CardBackground {
            Layout.fillWidth: true

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 16
                rowSpacing: Theme.spacingMedium

                ParamSlider {
                    Layout.fillWidth: true
                    label: "Kp"
                    value: engine.config.kp
                    from: 0.001; to: 3.0; stepSize: 0.001; decimals: 3
                    onValueModified: (v) => engine.config.kp = v
                }

                ParamSlider {
                    Layout.fillWidth: true
                    label: "Ki"
                    value: engine.config.ki
                    from: 0; to: 0.05; stepSize: 0.001; decimals: 3
                    onValueModified: (v) => engine.config.ki = v
                }

                ParamSlider {
                    Layout.fillWidth: true
                    label: "Kd"
                    value: engine.config.kd
                    from: 0.001; to: 0.5; stepSize: 0.001; decimals: 3
                    onValueModified: (v) => engine.config.kd = v
                }

                ParamSlider {
                    Layout.fillWidth: true
                    label: "\u9884\u6D4B\u6B65\u6570"
                    value: engine.config.predictionSteps
                    from: 0; to: 20; stepSize: 1; decimals: 0
                    onValueModified: (v) => engine.config.predictionSteps = Math.round(v)
                }
            }
        }

        // ── 瞄准范围 ──
        SectionHeader { text: "\u7784\u51C6\u8303\u56F4" }

        CardBackground {
            Layout.fillWidth: true

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 16
                rowSpacing: Theme.spacingMedium

                // ── 左列：参数控制 ──
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1  // 与右列等宽
                    spacing: Theme.spacingMedium

                    ParamSlider {
                        Layout.fillWidth: true
                        label: "\u8303\u56F4\u5927\u5C0F (%)"
                        value: engine.config.aimRangeSize
                        from: 1; to: 100; stepSize: 1; decimals: 0
                        onValueModified: (v) => engine.config.aimRangeSize = Math.round(v)
                    }

                    ParamSlider {
                        Layout.fillWidth: true
                        label: "\u7075\u654F\u5EA6"
                        value: engine.config.sensitivity
                        from: 0.01; to: 1.0; stepSize: 0.01; decimals: 2
                        onValueModified: (v) => engine.config.sensitivity = v
                    }

                    // 范围形状
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "\u8303\u56F4\u5F62\u72B6"
                            font.pixelSize: Theme.fontMedium
                            color: Theme.textSecondary
                        }

                        Rectangle {
                            implicitWidth: 60
                            implicitHeight: 28
                            radius: Theme.radiusSmall
                            color: engine.config.aimRangeCircle ? Theme.accentDim : Theme.bgInput
                            border.color: engine.config.aimRangeCircle ? Theme.accent : Theme.border
                            border.width: 1
                            Label {
                                anchors.centerIn: parent
                                text: "\u5706\u5F62"
                                font.pixelSize: Theme.fontSmall
                                color: engine.config.aimRangeCircle ? Theme.accent : Theme.textSecondary
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: engine.config.aimRangeCircle = true
                            }
                        }

                        Rectangle {
                            implicitWidth: 60
                            implicitHeight: 28
                            radius: Theme.radiusSmall
                            color: !engine.config.aimRangeCircle ? Theme.accentDim : Theme.bgInput
                            border.color: !engine.config.aimRangeCircle ? Theme.accent : Theme.border
                            border.width: 1
                            Label {
                                anchors.centerIn: parent
                                text: "\u77E9\u5F62"
                                font.pixelSize: Theme.fontSmall
                                color: !engine.config.aimRangeCircle ? Theme.accent : Theme.textSecondary
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: engine.config.aimRangeCircle = false
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // ── 右列：瞄准位置选择器 ──
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    spacing: Theme.spacingSmall

                    TargetSelector {
                        id: targetSelector
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        showAimPoint: !engine.config.drawAimOnPreview

                        Component.onCompleted: {
                            ratio = engine.config.targetYRatio
                        }

                        onRatioChanged: {
                            engine.config.targetYRatio = ratio
                        }
                    }

                    ToggleSwitch {
                        Layout.fillWidth: true
                        text: "\u7ED8\u5236\u5728\u9884\u89C8\u7A97\u53E3\u4E0A"
                        checked: engine.config.drawAimOnPreview
                        onToggled: (v) => engine.config.drawAimOnPreview = v
                    }
                }
            }
        }

        // 底部间距
        Item { Layout.preferredHeight: 20 }
    }
}
