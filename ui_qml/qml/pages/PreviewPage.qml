import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls
import "../theme"
import "../components"

ColumnLayout {
    id: root
    spacing: Theme.spacingMedium

    // 顶部工具栏
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 20
        Layout.rightMargin: 20
        Layout.topMargin: 16
        spacing: 12

        ButtonStyled {
            text: engine.running ? "\u505C\u6B62\u63A8\u7406" : "\u5F00\u59CB\u63A8\u7406"
            type: engine.running ? "danger" : "primary"
            onClicked: engine.toggleRunning()
        }

        ButtonStyled {
            text: previewWindow.visible ? "\u6536\u56DE\u7A97\u53E3" : "\u5F39\u51FA\u7A97\u53E3"
            type: "secondary"
            onClicked: previewWindow.visible = !previewWindow.visible
        }

        Item { Layout.fillWidth: true }

        Label {
            text: "FPS: " + engine.fps.toFixed(1) + "  |  " + engine.inferMs.toFixed(1) + "ms"
            font.pixelSize: Theme.fontMedium
            font.family: "Consolas"
            color: engine.running ? Theme.accent : Theme.textMuted
        }
    }

    // 帧预览区 —— 固定显示尺寸（不随采集尺寸变化），仅弹出窗口跟随采集尺寸
    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: 320
        Layout.preferredHeight: 320
        radius: Theme.radiusLarge
        color: "#000000"
        clip: true

        Image {
            id: frameView
            anchors.fill: parent
            fillMode: Image.Stretch
            cache: false
            asynchronous: false
            source: "image://frames/current?" + engine.frameCounter
        }

        // FOV + 检测框叠加层
        Canvas {
            id: detectionCanvas
            anchors.fill: parent
            visible: engine.running

            onPaint: {
                var ctx = getContext("2d")
                var W = width
                var H = height
                ctx.clearRect(0, 0, W, H)
                if (!engine.running) return

                // ── FOV（圆形 or 矩形）──
                var aimRange = engine.config.aimRangeSize / 100
                var fovCX = W * 0.5
                var fovCY = H * 0.5
                var fovR = aimRange * W * 0.5

                ctx.strokeStyle = "#6C63FF80"
                ctx.lineWidth = 1.5
                ctx.setLineDash([6, 4])
                if (engine.config.aimRangeCircle) {
                    if (fovR > 2) { ctx.beginPath(); ctx.arc(fovCX, fovCY, fovR, 0, 2 * Math.PI); ctx.stroke() }
                } else {
                    var side = aimRange * W
                    if (side > 4) ctx.strokeRect(fovCX - side / 2, fovCY - side / 2, side, side)
                }
                ctx.setLineDash([])

                // 中心十字
                ctx.strokeStyle = "#FFFFFF60"; ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(fovCX - 10, fovCY); ctx.lineTo(fovCX + 10, fovCY)
                ctx.moveTo(fovCX, fovCY - 10); ctx.lineTo(fovCX, fovCY + 10)
                ctx.stroke()

                // ── 检测框 + 瞄准点 ──
                if (!engine.config.drawDetectionBoxes) return
                var dets = engine.detections
                if (!dets || dets.length === 0) return

                var targetClasses = engine.config.targetClassIds
                var fovRN = aimRange * 0.5
                var aimYRatio = engine.config.targetYRatio

                for (var i = 0; i < dets.length; i++) {
                    var d = dets[i]
                    var sx = d.x1 * W, sy = d.y1 * H
                    var sw = (d.x2 - d.x1) * W, sh = (d.y2 - d.y1) * H
                    var dcx = (d.x1 + d.x2) / 2, dcy = (d.y1 + d.y2) / 2
                    var dist = Math.sqrt((dcx - 0.5) * (dcx - 0.5) + (dcy - 0.5) * (dcy - 0.5))
                    var inFOV = dist <= fovRN
                    var isTarget = targetClasses.includes(d.classId)

                    ctx.strokeStyle = (isTarget && inFOV) ? "#00FF00" : (isTarget ? "#008000" : "#FF4444")
                    ctx.lineWidth = (isTarget && inFOV) ? 2 : (isTarget ? 1.5 : 1)
                    ctx.strokeRect(sx, sy, sw, sh)

                    if (isTarget) {
                        ctx.font = "11px Consolas"; ctx.fillStyle = ctx.strokeStyle
                        ctx.fillText("cls:" + d.classId + " " + Math.round(d.confidence * 100) + "%", sx, sy - 4)

                        // 黄色瞄准点十字（开关控制）
                        if (engine.config.drawAimOnPreview) {
                            var ax = sx + sw * 0.5
                            var ay = sy + sh * aimYRatio
                            ctx.strokeStyle = "#FFFF00"; ctx.lineWidth = 1.5
                            ctx.beginPath()
                            ctx.moveTo(ax - 6, ay); ctx.lineTo(ax + 6, ay)
                            ctx.moveTo(ax, ay - 6); ctx.lineTo(ax, ay + 6)
                            ctx.stroke()
                        }
                    }
                }
            }
        }

        Connections {
            target: engine
            function onDetectionsChanged() {
                detectionCanvas.requestPaint()
            }
        }

        // 采集尺寸标签
        Label {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 6
            font.pixelSize: 11
            font.family: "Consolas"
            color: "#FFFF00"
            text: "CS: " + engine.config.captureSize
        }

        // 占位提示
        Label {
            visible: !engine.running
            anchors.centerIn: parent
            text: "\u70B9\u51FB\u201C\u5F00\u59CB\u63A8\u7406\u201D\u542F\u52A8\u9884\u89C8"
            font.pixelSize: Theme.fontLarge
            color: Theme.textMuted
        }
    }

    // 底部设置区
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 20
        Layout.rightMargin: 20
        Layout.bottomMargin: 16
        spacing: 16

        CardBackground {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall
                Label {
                    text: "\u91C7\u96C6\u8BBE\u7F6E"
                    font.pixelSize: Theme.fontMedium
                    font.weight: Font.DemiBold
                    color: Theme.textPrimary
                }
                ComboBoxStyled {
                    id: captureSizeCombo
                    Layout.fillWidth: true
                    label: "\u91C7\u96C6\u5C3A\u5BF8"
                    model: ["256", "320", "480", "640"]
                    Component.onCompleted: {
                        var sizes = [256, 320, 480, 640]
                        var idx = sizes.indexOf(engine.config.captureSize)
                        captureSizeCombo.currentIndex = idx >= 0 ? idx : 3
                    }
                    onActivated: {
                        var sizes = [256, 320, 480, 640]
                        engine.config.captureSize = sizes[index]
                    }
                    Connections {
                        target: engine.config
                        function onCaptureSizeChanged() {
                            var sizes = [256, 320, 480, 640]
                            var idx = sizes.indexOf(engine.config.captureSize)
                            if (idx >= 0) captureSizeCombo.currentIndex = idx
                        }
                    }
                }
            }
        }

        CardBackground {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall
                Label {
                    text: "\u663E\u793A\u8BBE\u7F6E"
                    font.pixelSize: Theme.fontMedium
                    font.weight: Font.DemiBold
                    color: Theme.textPrimary
                }
                ToggleSwitch {
                    Layout.fillWidth: true
                    text: "\u663E\u793A\u63A8\u7406\u5EF6\u8FDF"
                    checked: engine.config.showInferLatency
                    onToggled: (v) => engine.config.showInferLatency = v
                }
                ToggleSwitch {
                    Layout.fillWidth: true
                    text: "\u7ED8\u5236\u68C0\u6D4B\u6846"
                    checked: engine.config.drawDetectionBoxes
                    onToggled: (v) => engine.config.drawDetectionBoxes = v
                }
                ToggleSwitch {
                    Layout.fillWidth: true
                    text: "\u8C03\u8BD5\u7A97\u53E3"
                    checked: engine.config.debugWindowEnabled
                    onToggled: (v) => engine.config.debugWindowEnabled = v
                }
            }
        }
    }

    PreviewWindow { id: previewWindow }
}
