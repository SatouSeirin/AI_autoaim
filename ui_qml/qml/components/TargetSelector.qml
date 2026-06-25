import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

// 瞄准位置选择器 — 人体矩形 + 蓝色指示点（连续滑块）
// 嵌入 AimPage 瞄准范围卡片的右半部分
Item {
    id: root
    implicitHeight: 220

    // 连续比例 0.0（头部）→ 1.0（脚部）
    property double ratio: 0.35
    // 是否在组件上绘制瞄准十字
    property bool showAimPoint: false

    // ── 人体矩形框 ──
    Rectangle {
        id: bodyRect
        anchors.horizontalCenter: parent.horizontalCenter
        y: 8
        width: Math.min(60, parent.width * 0.4)
        height: parent.height - 48
        radius: Theme.radiusSmall
        color: "transparent"
        border.color: Theme.textMuted
        border.width: 1

        // 头部标记
        Rectangle {
            width: 8; height: 8; radius: 4
            anchors.horizontalCenter: parent.horizontalCenter
            y: 4
            color: Theme.textMuted
        }

        // 胸部标记线
        Rectangle {
            width: parent.width * 0.5; height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height * 0.35
            color: Theme.textMuted
            opacity: 0.5
        }

        // 脚部标记线
        Rectangle {
            width: parent.width * 0.4; height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height * 0.7
            color: Theme.textMuted
            opacity: 0.5
        }
    }

    // ── 垂直轨道 ──
    Rectangle {
        id: track
        x: bodyRect.x + bodyRect.width + 16
        width: 3
        y: bodyRect.y + 4
        height: bodyRect.height - 8
        radius: 1.5
        color: Theme.bgInput
    }

    // ── 蓝色指示点（可拖拽，连续）──
    Rectangle {
        id: dot
        width: 16; height: 16; radius: 8
        x: track.x + track.width / 2 - width / 2
        y: ratioToY(root.ratio)
        color: Theme.accent
        border.color: Theme.accentHover
        border.width: 2
        z: 2

        // 光晕
        Rectangle {
            anchors.centerIn: parent
            width: 26; height: 26; radius: 13
            color: Theme.accentDim
        }

        MouseArea {
            anchors.fill: parent
            anchors.margins: -12
            cursorShape: Qt.PointingHandCursor
            drag.target: dot
            drag.axis: Drag.YAxis
            drag.minimumY: 0
            drag.maximumY: track.height - dot.height

            onPositionChanged: {
                root.ratio = yToRatio(dot.y)
            }
        }
    }

    // ── 黄色瞄准十字（绘制在人体矩形上）──
    Canvas {
        id: aimCanvas
        anchors.fill: bodyRect
        visible: root.showAimPoint
        z: 3

        onPaint: {
            var ctx = getContext("2d")
            var w = width
            var h = height
            ctx.clearRect(0, 0, w, h)
            if (!root.showAimPoint) return

            var cx = w * 0.5
            var cy = root.ratio * h
            ctx.strokeStyle = "#FFFF00"
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.moveTo(cx - 8, cy); ctx.lineTo(cx + 8, cy)
            ctx.moveTo(cx, cy - 8); ctx.lineTo(cx, cy + 8)
            ctx.stroke()
        }
    }

    // ratio 变化时重绘十字 + 移动标签
    onRatioChanged: {
        aimCanvas.requestPaint()
    }

    // ── 部位标签 ──
    Label {
        anchors.left: track.right
        anchors.leftMargin: 10
        y: dot.y + dot.height / 2 - height / 2
        text: Math.round(root.ratio * 100) + "%"
        font.pixelSize: Theme.fontSmall
        font.family: "Consolas"
        color: Theme.accent
    }

    // ── 位置映射（连续）──
    function ratioToY(r) {
        return r * (track.height - dot.height)
    }

    function yToRatio(yPos) {
        var max_ = track.height - dot.height
        if (max_ <= 0) return 0.35
        return Math.max(0, Math.min(1, yPos / max_))
    }
}
