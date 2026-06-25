#include "previewpage.h"
#include "../../src/engine.hpp"
#include "../widgets/previewwindow.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QImage>
#include <QPixmap>

PreviewPage::PreviewPage(QWidget* parent) : QWidget(parent) {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    outerLayout->addWidget(scrollArea);

    auto* container = new QWidget();
    scrollArea->setWidget(container);

    auto* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(24);

    auto* previewGroup = new QGroupBox(QString::fromUtf8("\xf0\x9f\x96\xa5 \xe7\x94\xbb\xe9\x9d\xa2\xe6\xba\x90\xe9\xa2\x84\xe8\xa7\x88"), this);
    setupPreviewArea(previewGroup);
    mainLayout->addWidget(previewGroup);

    
    auto* displayGroup = new QGroupBox(QString::fromUtf8("\xf0\x9f\x96\xa5 \xe6\x98\xbe\xe7\xa4\xba\xe8\xae\xbe\xe7\xbd\xae"), this);
    setupDisplaySettings(displayGroup);
    mainLayout->addWidget(displayGroup);

    mainLayout->addStretch();

    fpsLabel_ = new QLabel("FPS: -- | \u63a8\u7406: -- ms", this);
    fpsLabel_->setStyleSheet("font-size: 13px; color: #64748B; padding: 8px 0;");
    fpsLabel_->setAlignment(Qt::AlignRight);
    mainLayout->addWidget(fpsLabel_);
}

PreviewPage::~PreviewPage() {
    if (previewWin_) {
        previewWin_->close();
        delete previewWin_;
        previewWin_ = nullptr;
    }
}

void PreviewPage::setEngine(AimEngine* engine) {
    engine_ = engine;
    if (engine_) {
        auto cfg = engine_->GetConfig();
        captureSize_ = cfg.capture_size;

        // 同步 UI checkbox 到引擎配置的 enable_visualization
        if (chkDrawBoxes_) {
            chkDrawBoxes_->blockSignals(true);
            chkDrawBoxes_->setChecked(cfg.enable_visualization);
            chkDrawBoxes_->blockSignals(false);
        }

        // 同步 UI 到当前截图尺寸
        static const int presets[] = {256, 320, 480, 640};
        int match = -1;
        for (int i = 0; i < 4; i++) {
            if (presets[i] == captureSize_) { match = i; break; }
        }
        if (match >= 0) {
            sizeCombo_->blockSignals(true);
            sizeCombo_->setCurrentIndex(match);
            sizeCombo_->blockSignals(false);
            customSizeSpin_->setVisible(false);
        } else {
            sizeCombo_->blockSignals(true);
            sizeCombo_->setCurrentIndex(4);  // 自定义
            sizeCombo_->blockSignals(false);
            customSizeSpin_->setVisible(true);
            customSizeSpin_->blockSignals(true);
            customSizeSpin_->setValue(captureSize_);
            customSizeSpin_->blockSignals(false);
        }
    }
}

// ============================================================
// 帧回调入口
// ============================================================
void PreviewPage::updateFrame(const QImage& frame) {
    if (frame.isNull()) return;

    bool embedded = chkDebugWindow_ && chkDebugWindow_->isChecked();
    bool popped   = popOutActive_ && previewWin_ && previewWin_->isVisible();

    if (!embedded && !popped) return;

    // → 内嵌预览（缩放到 label 大小）
    if (embedded) {
        int target = qMin(previewLabel_->width(), previewLabel_->height());
        if (target < 64) target = 256;
        QImage scaled = frame.scaled(target, target, Qt::KeepAspectRatio, Qt::FastTransformation);
        previewLabel_->setPixmap(QPixmap::fromImage(scaled));
    }

    // → 独立预览窗口（原始尺寸）
    if (popped) {
        previewWin_->updateFrame(frame);
    }
}

// ============================================================
// 预览区域 UI
// ============================================================
void PreviewPage::setupPreviewArea(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);
    layout->setSpacing(10);

    previewLabel_ = new QLabel(group);
    previewLabel_->setMinimumHeight(240);
    previewLabel_->setAlignment(Qt::AlignCenter);
    previewLabel_->setStyleSheet(
        "background: #FAFAFA; border: 1px solid #E8E8E8; border-radius: 8px;"
        " font-size: 14px; color: #888888;");
    previewLabel_->setText(QString::fromUtf8(
        "\xf0\x9f\x91\x81\xe2\x80\x8d\xf0\x9f\x97\xa8 \xe9\xa2\x84\xe8\xa7\x88\xe7\xaa\x97\xe5\x8f\xa3\xe5\xb7\xb2\xe5\x85\xb3\xe9\x97\xad\n"
        "\xe7\x82\xb9\xe5\x87\xbb\xe3\x80\x8c\xe8\xb0\x83\xe8\xaf\x95\xe7\xaa\x97\xe5\x8f\xa3\xe3\x80\x8d\xe6\x88\x96\xe3\x80\x8c\xe5\xbc\xb9\xe5\x87\xba\xe7\xaa\x97\xe5\x8f\xa3\xe3\x80\x8d\xe6\x8c\x89\xe9\x92\xae\xe5\xbc\x80\xe5\x90\xaf\xe5\xae\x9e\xe6\x97\xb6\xe9\xa2\x84\xe8\xa7\x88"));
    layout->addWidget(previewLabel_);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    debugBtn_ = new QPushButton(QString::fromUtf8("\xe8\xb0\x83\xe8\xaf\x95\xe7\xaa\x97\xe5\x8f\xa3"), group);
    debugBtn_->setCursor(Qt::PointingHandCursor);
    debugBtn_->setFixedWidth(100);
    debugBtn_->setMinimumHeight(32);
    connect(debugBtn_, &QPushButton::clicked, [this]() {
        if (engine_) {
            auto cfg = engine_->GetConfig();
            cfg.debug_window_enabled = !cfg.debug_window_enabled;
            engine_->UpdateConfig(cfg);
            chkDebugWindow_->setChecked(cfg.debug_window_enabled);
            if (!cfg.debug_window_enabled) {
                previewLabel_->setText(QString::fromUtf8(
                    "\xf0\x9f\x91\x81\xe2\x80\x8d\xf0\x9f\x97\xa8 \xe9\xa2\x84\xe8\xa7\x88\xe7\xaa\x97\xe5\x8f\xa3\xe5\xb7\xb2\xe5\x85\xb3\xe9\x97\xad\n"
                    "\xe7\x82\xb9\xe5\x87\xbb\xe3\x80\x8c\xe8\xb0\x83\xe8\xaf\x95\xe7\xaa\x97\xe5\x8f\xa3\xe3\x80\x8d\xe6\x88\x96\xe3\x80\x8c\xe5\xbc\xb9\xe5\x87\xba\xe7\xaa\x97\xe5\x8f\xa3\xe3\x80\x8d\xe6\x8c\x89\xe9\x92\xae\xe5\xbc\x80\xe5\x90\xaf\xe5\xae\x9e\xe6\x97\xb6\xe9\xa2\x84\xe8\xa7\x88"));
            }
        }
    });
    btnRow->addWidget(debugBtn_);

    popOutBtn_ = new QPushButton(QString::fromUtf8("\xf0\x9f\x93\xa4 \xe5\xbc\xb9\xe5\x87\xba\xe7\xaa\x97\xe5\x8f\xa3"), group);
    popOutBtn_->setCursor(Qt::PointingHandCursor);
    popOutBtn_->setMinimumHeight(32);
    popOutBtn_->setStyleSheet(
        "QPushButton { background: #1890FF; color: white; border-radius: 4px;"
        " font-weight: bold; padding: 0 16px; }"
        "QPushButton:hover { background: #40A9FF; }");
    connect(popOutBtn_, &QPushButton::clicked, this, &PreviewPage::onPopOutToggled);
    btnRow->addWidget(popOutBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);
}

// ============================================================
// 弹出/收回独立预览窗口（修复 WA_DeleteOnClose 导致状态错乱）
// ============================================================
void PreviewPage::onPopOutToggled(bool /*on*/) {
    if (!popOutActive_) {
        // 创建并显示独立窗口
        if (!previewWin_) {
            previewWin_ = new PreviewWindow(nullptr);
            previewWin_->setAttribute(Qt::WA_DeleteOnClose);  // X 按钮直接销毁
            connect(previewWin_, &QWidget::destroyed, this, [this]() {
                previewWin_ = nullptr;
                popOutActive_ = false;
                popOutBtn_->setText(QString::fromUtf8("\xf0\x9f\x93\xa4 \xe5\xbc\xb9\xe5\x87\xba\xe7\xaa\x97\xe5\x8f\xa3"));
                popOutBtn_->setStyleSheet(
                    "QPushButton { background: #1890FF; color: white; border-radius: 4px;"
                    " font-weight: bold; padding: 0 16px; }"
                    "QPushButton:hover { background: #40A9FF; }");
            });
        }
        previewWin_->show();
        previewWin_->raise();
        popOutActive_ = true;
        popOutBtn_->setText(QString::fromUtf8("\xe2\x9c\x95 \xe6\x94\xb6\xe5\x9b\x9e\xe7\xaa\x97\xe5\x8f\xa3"));
        popOutBtn_->setStyleSheet(
            "QPushButton { background: #FF4D4F; color: white; border-radius: 4px;"
            " font-weight: bold; padding: 0 16px; }"
            "QPushButton:hover { background: #FF7875; }");
    } else {
        // 关闭独立窗口（WA_DeleteOnClose 会触发 destroyed → 自动清理）
        if (previewWin_) {
            previewWin_->close();
        }
        // destroyed 信号会自动重置 popOutActive_ 和按钮
    }
}

// ============================================================
// 输入模式
// ============================================================
// 显示设置（截图尺寸 + 选项）
// ============================================================
void PreviewPage::setupDisplaySettings(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);
    layout->setSpacing(12);

    // 截图尺寸（修改 capture_size，即实际送入模型的画面区域大小）
    auto* sizeRow = new QHBoxLayout();
    auto* sizeLabel = new QLabel(QString::fromUtf8("\xe6\x88\xaa\xe5\x9b\xbe\xe5\xb0\xba\xe5\xaf\xb8"), group);
    sizeLabel->setStyleSheet("font-size: 14px; color: #333333;");
    sizeCombo_ = new QComboBox(group);
    sizeCombo_->addItems({QStringLiteral("256\u00d7256"), QStringLiteral("320\u00d7320"),
                          QStringLiteral("480\u00d7480"), QStringLiteral("640\u00d7640"),
                          QString::fromUtf8("\xe8\x87\xaa\xe5\xae\x9a\xe4\xb9\x89")});
    sizeCombo_->setCurrentIndex(3);  // 默认 640
    sizeCombo_->setFixedWidth(110);
    sizeCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #D9D9D9; border-radius: 4px;"
        " padding: 4px 8px; font-size: 13px; color: #333333; }");
    connect(sizeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreviewPage::onCaptureSizeChanged);

    customSizeSpin_ = new QSpinBox(group);
    customSizeSpin_->setRange(64, 1920);
    customSizeSpin_->setValue(640);
    customSizeSpin_->setFixedWidth(80);
    customSizeSpin_->setSuffix(" px");
    customSizeSpin_->setVisible(false);
    customSizeSpin_->setStyleSheet(
        "QSpinBox { border: 1px solid #D9D9D9; border-radius: 4px;"
        " padding: 4px 8px; font-size: 13px; }");
    connect(customSizeSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        captureSize_ = val;
        if (engine_) {
            engine_->SetCaptureSize(val);
        }
    });

    sizeRow->addWidget(sizeLabel);
    sizeRow->addStretch();
    sizeRow->addWidget(sizeCombo_);
    sizeRow->addWidget(customSizeSpin_);
    layout->addLayout(sizeRow);

    // 显示推理延迟
    auto* row1 = new QHBoxLayout();
    auto* lbl1 = new QLabel(QString::fromUtf8("\xe6\x98\xbe\xe7\xa4\xba\xe6\x8e\xa8\xe7\x90\x86\xe5\xbb\xb6\xe8\xbf\x9f"), group);
    lbl1->setStyleSheet("font-size: 14px; color: #333333;");
    chkShowLatency_ = new QCheckBox(group);
    chkShowLatency_->setChecked(true);
    chkShowLatency_->style()->polish(chkShowLatency_);
    connect(chkShowLatency_, &QCheckBox::toggled, this, &PreviewPage::onShowLatencyToggled);
    row1->addWidget(lbl1);
    row1->addStretch();
    row1->addWidget(chkShowLatency_);
    layout->addLayout(row1);

    // 内嵌显示调试窗口
    auto* row2 = new QHBoxLayout();
    auto* lbl2 = new QLabel(QString::fromUtf8("\xe5\x86\x85\xe5\xb5\x8c\xe6\x98\xbe\xe7\xa4\xba\xe8\xb0\x83\xe8\xaf\x95\xe7\xaa\x97\xe5\x8f\xa3"), group);
    lbl2->setStyleSheet("font-size: 14px; color: #333333;");
    chkDebugWindow_ = new QCheckBox(group);
    chkDebugWindow_->setChecked(false);
    chkDebugWindow_->style()->polish(chkDebugWindow_);
    connect(chkDebugWindow_, &QCheckBox::toggled, this, &PreviewPage::onDebugWindowToggled);
    row2->addWidget(lbl2);
    row2->addStretch();
    row2->addWidget(chkDebugWindow_);
    layout->addLayout(row2);

    // 绘制检测框
    auto* row3 = new QHBoxLayout();
    auto* lbl3 = new QLabel(QString::fromUtf8("\xe7\xbb\x98\xe5\x88\xb6\xe6\xa3\x80\xe6\xb5\x8b\xe6\xa1\x86"), group);
    lbl3->setStyleSheet("font-size: 14px; color: #333333;");
    chkDrawBoxes_ = new QCheckBox(group);
    chkDrawBoxes_->setChecked(true);
    chkDrawBoxes_->style()->polish(chkDrawBoxes_);
    connect(chkDrawBoxes_, &QCheckBox::toggled, this, &PreviewPage::onDrawBoxesToggled);
    row3->addWidget(lbl3);
    row3->addStretch();
    row3->addWidget(chkDrawBoxes_);
    layout->addLayout(row3);
}

void PreviewPage::onCaptureSizeChanged(int idx) {
    static const int sizes[] = {256, 320, 480, 640};
    if (idx >= 0 && idx < 4) {
        captureSize_ = sizes[idx];
        customSizeSpin_->setVisible(false);
    } else if (idx == 4) {
        customSizeSpin_->setVisible(true);
        customSizeSpin_->setValue(captureSize_);
        captureSize_ = customSizeSpin_->value();
    }
    if (engine_) {
        engine_->SetCaptureSize(captureSize_);
    }
}


void PreviewPage::onShowLatencyToggled(bool on) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.show_infer_latency = on;
    engine_->UpdateConfig(cfg);
}

void PreviewPage::onDebugWindowToggled(bool on) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.debug_window_enabled = on;
    engine_->UpdateConfig(cfg);
    if (!on) {
        previewLabel_->setText(QString::fromUtf8(
            "\xf0\x9f\x91\x81\xe2\x80\x8d\xf0\x9f\x97\xa8 \xe9\xa2\x84\xe8\xa7\x88\xe7\xaa\x97\xe5\x8f\xa3\xe5\xb7\xb2\xe5\x85\xb3\xe9\x97\xad\n"
            "\xe7\x82\xb9\xe5\x87\xbb\xe3\x80\x8c\xe8\xb0\x83\xe8\xaf\x95\xe7\xaa\x97\xe5\x8f\xa3\xe3\x80\x8d\xe6\x88\x96\xe3\x80\x8c\xe5\xbc\xb9\xe5\x87\xba\xe7\xaa\x97\xe5\x8f\xa3\xe3\x80\x8d\xe6\x8c\x89\xe9\x92\xae\xe5\xbc\x80\xe5\x90\xaf\xe5\xae\x9e\xe6\x97\xb6\xe9\xa2\x84\xe8\xa7\x88"));
    } else {
        previewLabel_->setText(QString::fromUtf8("\xe7\xad\x89\xe5\xbe\x85\xe6\x8e\xa8\xe7\x90\x86\xe5\xb8\xa7..."));
    }
}

void PreviewPage::onDrawBoxesToggled(bool on) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.enable_visualization = on;
    cfg.draw_detection_boxes = on;
    engine_->UpdateConfig(cfg);



}

void PreviewPage::setFpsInfo(double fps, double ms) {
    if (fpsLabel_) {
        fpsLabel_->setText(QString("FPS: %1 | ??: %2 ms").arg(fps, 0, 'f', 1).arg(ms, 0, 'f', 1));
    }
}
