#pragma once

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QImage>

// ============================================================
// PreviewWindow — v2.1 可拖拽独立推理预览窗口
// 显示 1:1 原始帧，WA_DeleteOnClose 避免状态泄漏
// 尺寸由 PreviewPage 的 capture_size 决定
// ============================================================
class PreviewWindow : public QWidget {
    Q_OBJECT

public:
    explicit PreviewWindow(QWidget* parent = nullptr);
    void updateFrame(const QImage& frame);

private slots:
    void setAlwaysOnTop(bool on);

private:
    void setupUI();

    QLabel*    frameLabel_      = nullptr;
    QCheckBox* chkAlwaysOnTop_  = nullptr;
    QImage     lastFrame_;
};
