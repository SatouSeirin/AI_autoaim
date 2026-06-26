import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Window {
    id: popoutWindow
    width: 656
    height: 704
    title: "MiniSense - \u9884\u89C8\u7A97\u53E3"

    function syncSize() {
        var cs = engine.config.captureSize
        popoutWindow.minimumWidth = cs + 16
        popoutWindow.minimumHeight = cs + 48
        popoutWindow.maximumWidth = cs + 16
        popoutWindow.maximumHeight = cs + 48
        popoutWindow.width = cs + 16
        popoutWindow.height = cs + 48
    }

    onVisibleChanged: {
        if (visible) syncSize()
    }

    Connections {
        target: engine.config
        function onCaptureSizeChanged() {
            resizeTimer.restart()
        }
    }

    Timer {
        id: resizeTimer
        interval: 50
        onTriggered: popoutWindow.syncSize()
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 8
        color: "#000000"

        Image {
            id: frameView
            anchors.fill: parent
            fillMode: Image.Stretch
            cache: false
            asynchronous: false
            source: "image://frames/current?" + engine.frameCounter
        }

        // FOV + 检测框
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
            function onDetectionsChanged() { detectionCanvas.requestPaint() }
        }

        // FPS 角标
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 4
            width: fpsLabel.implicitWidth + 12
            height: 22
            radius: 4
            color: "#00000080"
            Label {
                id: fpsLabel
                anchors.centerIn: parent
                text: "FPS: " + engine.fps.toFixed(1) + " | " + engine.inferMs.toFixed(1) + "ms"
                font.pixelSize: 11
                color: "white"
            }
        }
    }

    // 调试标签（最后声明 → 渲染在最上层）
    Label {
        id: debugLabel
        x: 12
        y: 4
        z: 999
        font.pixelSize: 12
        font.family: "Consolas"
        color: "#FFFF00"
        text: {
            var cs = engine.config.captureSize
            return "cs:" + cs + "  win:" + popoutWindow.width.toFixed(0) + "x" + popoutWindow.height.toFixed(0)
        }
    }
}
