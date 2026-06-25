#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QTimer>
#include <QSpinBox>

// ============================================================
// PreviewPage — v2.1 画面源预览页
// 实时预览 + 输入模式 + 显示设置 + 独立预览窗口
// ============================================================
class AimEngine;
class PreviewWindow;

class PreviewPage : public QWidget {
    Q_OBJECT

public:
    explicit PreviewPage(QWidget* parent = nullptr);
    ~PreviewPage() override;
    void setEngine(AimEngine* engine);
    void setFpsInfo(double fps, double ms);

public slots:
    void updateFrame(const QImage& frame);

private slots:
    void onInputModeChanged();
    void onShowLatencyToggled(bool on);
    void onDebugWindowToggled(bool on);
    void onDrawBoxesToggled(bool on);
    void onPopOutToggled(bool on);
    void onCaptureSizeChanged(int idx);

private:
    void setupPreviewArea(QGroupBox* group);
    void setupInputMode(QGroupBox* group);
    void setupDisplaySettings(QGroupBox* group);

    AimEngine* engine_ = nullptr;

    // 内嵌预览
    QLabel*       previewLabel_ = nullptr;
    QLabel*       fpsLabel_       = nullptr;
    QPushButton*  debugBtn_     = nullptr;
    QPushButton*  popOutBtn_    = nullptr;

    // 独立预览窗口
    PreviewWindow* previewWin_  = nullptr;

    // 输入模式
    QRadioButton* radioSendInput_ = nullptr;
    QRadioButton* radioIbInput_   = nullptr;

    // 显示设置
    QCheckBox* chkShowLatency_ = nullptr;
    QCheckBox* chkDebugWindow_ = nullptr;
    QCheckBox* chkDrawBoxes_   = nullptr;
    QComboBox* sizeCombo_      = nullptr;
    QSpinBox*  customSizeSpin_ = nullptr;

    int captureSize_ = 640;  // 截图尺寸 (capture_size)

    bool popOutActive_ = false;
};
