import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Label {
    id: root
    font.pixelSize: Theme.fontLarge
    font.weight: Font.DemiBold
    color: Theme.textPrimary

    Rectangle {
        anchors.bottom: parent.bottom
        width: 24
        height: 2
        radius: 1
        color: Theme.accent
    }

    Layout.topMargin: 8
    Layout.bottomMargin: 4
}
