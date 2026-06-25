#pragma once
#include <QObject>
#include <QAbstractNativeEventFilter>
#include <windows.h>

// 全局热键捕获调度器
// 使用 C++ 事件过滤器拦截按键，完全绕过 QML 焦点系统
class HotkeyCaptureDispatcher : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
    Q_PROPERTY(bool capturing READ isCapturing NOTIFY capturingChanged)

public:
    explicit HotkeyCaptureDispatcher(QObject* parent = nullptr)
        : QObject(parent), m_target(nullptr) {}

    bool isCapturing() const { return m_target != nullptr; }

    // QML 调用：注册/取消捕获目标
    Q_INVOKABLE void beginCapture(QObject* target) {
        m_target = target;
        emit capturingChanged();
    }

    Q_INVOKABLE void cancelCapture() {
        if (m_target) {
            m_target = nullptr;
            emit capturingChanged();
        }
    }

    // QAbstractNativeEventFilter
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override {
        if (!m_target) return false;

        if (eventType == "windows_generic_MSG") {
            auto msg = static_cast<MSG*>(message);
            if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN) {
                UINT vk = static_cast<UINT>(msg->wParam);
                // 忽略纯修饰键（Shift/Ctrl/Alt 单独按下时不绑定）
                if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU)
                    return false;
                emit keyCaptured(static_cast<int>(vk));
                m_target = nullptr;
                emit capturingChanged();
                return true;  // 吞掉事件
            }
            // 鼠标按键
            if (msg->message == WM_LBUTTONDOWN || msg->message == WM_RBUTTONDOWN ||
                msg->message == WM_MBUTTONDOWN || msg->message == WM_XBUTTONDOWN) {
                UINT vk = VK_LBUTTON;
                if (msg->message == WM_RBUTTONDOWN) vk = VK_RBUTTON;
                else if (msg->message == WM_MBUTTONDOWN) vk = VK_MBUTTON;
                else if (msg->message == WM_XBUTTONDOWN) {
                    auto xmk = GET_XBUTTON_WPARAM(msg->wParam);
                    vk = (xmk == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
                }
                emit keyCaptured(static_cast<int>(vk));
                m_target = nullptr;
                emit capturingChanged();
                return true;
            }
        }
        return false;
    }

signals:
    void capturingChanged();
    void keyCaptured(int vkCode);

private:
    QObject* m_target;
};
