pragma Singleton
import QtQuick

QtObject {
    // ── 背景色 ──
    readonly property color bgPrimary:    "#1A1A2E"
    readonly property color bgSecondary:  "#16213E"
    readonly property color bgSidebar:    "#0F0F1A"
    readonly property color bgCard:       "#1E1E32"
    readonly property color bgInput:      "#252540"
    readonly property color bgHover:      "#2A2A45"
    readonly property color bgActive:     "#6C63FF20"

    // ── 文字色 ──
    readonly property color textPrimary:   "#E8E8F0"
    readonly property color textSecondary: "#A0A0B8"
    readonly property color textMuted:     "#606078"

    // ── 强调色 ──
    readonly property color accent:      "#6C63FF"
    readonly property color accentHover: "#7B73FF"
    readonly property color accentDim:   "#6C63FF30"
    readonly property color success:     "#4CAF50"
    readonly property color warning:     "#FF9800"
    readonly property color error:       "#F44336"

    // ── 边框 ──
    readonly property color border:    "#2A2A45"
    readonly property color separator: "#2A2A45"

    // ── 圆角 ──
    readonly property int radiusSmall:  4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge:  12

    // ── 字号 ──
    readonly property int fontSmall:  11
    readonly property int fontMedium: 13
    readonly property int fontLarge:  16
    readonly property int fontTitle:  20

    // ── 间距 ──
    readonly property int spacingSmall:  6
    readonly property int spacingMedium: 12
    readonly property int spacingLarge:  20

    // ── 侧边栏 ──
    readonly property int sidebarWidth: 200

    // ── 阴影 ──
    readonly property color shadow: "#00000040"

    // ── 顶栏 ──
    readonly property int topBarHeight: 40
}
