import QtQuick
import QtQuick.Layouts
import "../theme"

// CardBackground — 圆角卡片容器
// 自动适应内容高度，无需外部指定 implicitHeight
Rectangle {
    id: root
    color: Theme.bgCard
    radius: Theme.radiusLarge
    border.color: Theme.border
    border.width: 1

    default property alias content: contentArea.children
    property int cardPadding: 16

    ColumnLayout {
        id: contentArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.cardPadding
        anchors.rightMargin: root.cardPadding
        anchors.topMargin: root.cardPadding
        spacing: Theme.spacingMedium
    }

    // 内容高度 + 上下 padding
    implicitHeight: contentArea.implicitHeight + cardPadding * 2
}
