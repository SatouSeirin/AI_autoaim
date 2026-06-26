import QtQuick
import QtQuick.Controls
import "../theme"

// 轻量 Toast 提示 —— 底部居中，自动消失
Rectangle {
    id: toast
    z: 9999
    width: toastLabel.implicitWidth + 40
    height: 40
    radius: Theme.radiusMedium
    color: Theme.success
    opacity: 0
    visible: false

    property alias text: toastLabel.text
    property alias duration: hideTimer.interval

    Label {
        id: toastLabel
        anchors.centerIn: parent
        font.pixelSize: Theme.fontMedium
        font.weight: Font.DemiBold
        color: "white"
    }

    function show(msg, type) {
        text = msg
        if (type === "error")      color = Theme.error
        else if (type === "info")  color = Theme.accent
        else                       color = Theme.success
        visible = true
        appearAnim.start()
        hideTimer.restart()
    }

    NumberAnimation {
        id: appearAnim
        target: toast
        property: "opacity"
        from: 0; to: 1
        duration: 200
        easing.type: Easing.OutCubic
    }

    NumberAnimation {
        id: disappearAnim
        target: toast
        property: "opacity"
        from: 1; to: 0
        duration: 300
        easing.type: Easing.InCubic
        onFinished: toast.visible = false
    }

    Timer {
        id: hideTimer
        interval: 2000
        onTriggered: disappearAnim.start()
    }
}
