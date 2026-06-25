#include "hotkeycapture.h"
#include "../../src/hotkey_manager.hpp"

#include <QApplication>

HotkeyCapture::HotkeyCapture(int defaultVk, QWidget* parent)
    : QPushButton(parent), vkCode_(defaultVk) {
    setText(QString::fromStdString(VkToString(defaultVk)));
    setCursor(Qt::PointingHandCursor);
    setMinimumWidth(120);
    setMaximumWidth(200);
    setStyleSheet(
        "QPushButton {"
        "  background: #F5F5F5;"
        "  border: 1px solid #D9D9D9;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "  font-size: 14px;"
        "  color: #333333;"
        "  min-height: 32px;"
        "}"
        "QPushButton:hover {"
        "  border: 1px solid #1890FF;"
        "  color: #1890FF;"
        "}");
}

void HotkeyCapture::setVkCode(int vk) {
    vkCode_ = vk;
    setText(QString::fromStdString(VkToString(vk)));
    emit keyChanged(vk);
}

void HotkeyCapture::mousePressEvent(QMouseEvent* event) {
    if (listening_) {
        if (event->button() == Qt::LeftButton) {
            exitCaptureMode();
            setVkCode(VK_LBUTTON);
        } else if (event->button() == Qt::RightButton) {
            exitCaptureMode();
            setVkCode(VK_RBUTTON);
        } else if (event->button() == Qt::MiddleButton) {
            exitCaptureMode();
            setVkCode(VK_MBUTTON);
        } else if (event->button() == Qt::XButton1) {
            exitCaptureMode();
            setVkCode(VK_XBUTTON1);
        } else if (event->button() == Qt::XButton2) {
            exitCaptureMode();
            setVkCode(VK_XBUTTON2);
        }
        return;
    }
    enterCaptureMode();
}

void HotkeyCapture::keyPressEvent(QKeyEvent* event) {
    if (!listening_) {
        QPushButton::keyPressEvent(event);
        return;
    }

    int qtKey = event->key();

    // Qt key → Windows VK
    int vk = 0;
    switch (qtKey) {
    case Qt::Key_F1:  vk = VK_F1;  break;
    case Qt::Key_F2:  vk = VK_F2;  break;
    case Qt::Key_F3:  vk = VK_F3;  break;
    case Qt::Key_F4:  vk = VK_F4;  break;
    case Qt::Key_F5:  vk = VK_F5;  break;
    case Qt::Key_F6:  vk = VK_F6;  break;
    case Qt::Key_F7:  vk = VK_F7;  break;
    case Qt::Key_F8:  vk = VK_F8;  break;
    case Qt::Key_F9:  vk = VK_F9;  break;
    case Qt::Key_F10: vk = VK_F10; break;
    case Qt::Key_F11: vk = VK_F11; break;
    case Qt::Key_F12: vk = VK_F12; break;
    case Qt::Key_Shift:   vk = VK_SHIFT;   break;
    case Qt::Key_Control: vk = VK_CONTROL; break;
    case Qt::Key_Alt:     vk = VK_MENU;    break;
    case Qt::Key_Tab:     vk = VK_TAB;     break;
    case Qt::Key_CapsLock: vk = VK_CAPITAL; break;
    case Qt::Key_Space:   vk = VK_SPACE;   break;
    case Qt::Key_Return:
    case Qt::Key_Enter:   vk = VK_RETURN;  break;
    case Qt::Key_Backspace: vk = VK_BACK;  break;
    case Qt::Key_Escape:   vk = VK_ESCAPE; break;
    case Qt::Key_Delete:   vk = VK_DELETE; break;
    case Qt::Key_Insert:   vk = VK_INSERT; break;
    case Qt::Key_Home:     vk = VK_HOME;   break;
    case Qt::Key_End:      vk = VK_END;    break;
    case Qt::Key_PageUp:   vk = VK_PRIOR;  break;
    case Qt::Key_PageDown: vk = VK_NEXT;   break;
    case Qt::Key_Up:       vk = VK_UP;     break;
    case Qt::Key_Down:     vk = VK_DOWN;   break;
    case Qt::Key_Left:     vk = VK_LEFT;   break;
    case Qt::Key_Right:    vk = VK_RIGHT;  break;
    default:
        // 字母数字键直接用 ASCII
        if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
            vk = 'A' + (qtKey - Qt::Key_A);
        } else if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) {
            vk = '0' + (qtKey - Qt::Key_0);
        }
        break;
    }

    if (vk != 0) {
        exitCaptureMode();
        setVkCode(vk);
    }
}

void HotkeyCapture::focusOutEvent(QFocusEvent* event) {
    if (listening_) {
        exitCaptureMode();
    }
    QPushButton::focusOutEvent(event);
}

void HotkeyCapture::enterCaptureMode() {
    listening_ = true;
    origText_  = text();
    origStyle_ = styleSheet();
    setText("按任意键绑定...");
    setStyleSheet(captureStyle_);
    setFocus();
    grabKeyboard();
    grabMouse();
}

void HotkeyCapture::exitCaptureMode() {
    if (!listening_) return;
    listening_ = false;
    releaseKeyboard();
    releaseMouse();
    clearFocus();
}
