#pragma once

#include <QPushButton>
#include <QKeyEvent>
#include <QMouseEvent>

// ============================================================
// HotkeyCapture — v2.1 热键捕获控件
// 点击进入"监听模式"，按下任意键/鼠标键即绑定
// ============================================================
class HotkeyCapture : public QPushButton {
    Q_OBJECT

public:
    explicit HotkeyCapture(int defaultVk = 0, QWidget* parent = nullptr);

    // 获取/设置当前绑定的虚拟键码
    int  vkCode()    const { return vkCode_; }
    void setVkCode(int vk);

signals:
    void keyChanged(int newVk);

protected:
    void keyPressEvent(QKeyEvent* event)   override;
    void mousePressEvent(QMouseEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void enterCaptureMode();
    void exitCaptureMode();

    int     vkCode_      = 0;
    bool    listening_   = false;
    QString origText_;
    QString origStyle_;
    QString captureStyle_ =
        "QPushButton {"
        "  background: #FFF7E6;"
        "  border: 2px solid #FFA940;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  color: #FA8C16;"
        "  min-height: 32px;"
        "}";
};
