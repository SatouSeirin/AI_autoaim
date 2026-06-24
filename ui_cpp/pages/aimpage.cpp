#include "aimpage.h"
#include "../../src/engine.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPainter>
#include <QPen>

AimPage::AimPage(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(24);

    // ── 鼠标曲线输入方案 ──
    auto* curveGroup = new QGroupBox("🖱 鼠标曲线输入方案", this);
    setupMouseCurve(curveGroup);
    mainLayout->addWidget(curveGroup);

    // ── 瞄准设置 ──
    auto* aimGroup = new QGroupBox("🎯 瞄准设置", this);
    setupAimSettings(aimGroup);
    mainLayout->addWidget(aimGroup);

    // ── 热键绑定 ──
    auto* hotkeyGroup = new QGroupBox("⌨ 热键绑定", this);
    setupHotkeys(hotkeyGroup);
    mainLayout->addWidget(hotkeyGroup);

    mainLayout->addStretch();
}

void AimPage::setEngine(AimEngine* engine) {
    engine_ = engine;
}

// ── 鼠标曲线 ──
void AimPage::setupMouseCurve(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);

    kpSlider_ = new ParamSlider("PD 比例增益 (Kp)", 0.0, 1.0, 0.25, 3, "", group);
    connect(kpSlider_, &ParamSlider::valueChanged, this, &AimPage::onKpChanged);
    layout->addWidget(kpSlider_);

    kiSlider_ = new ParamSlider("PID 积分增益 (Ki)", 0.0, 0.01, 0.0008, 4, "", group);
    connect(kiSlider_, &ParamSlider::valueChanged, this, &AimPage::onKiChanged);
    layout->addWidget(kiSlider_);

    kalmanSlider_ = new ParamSlider("卡尔曼预测", 0, 200, 0, 0, " ms", group);
    connect(kalmanSlider_, &ParamSlider::valueChanged, this, &AimPage::onKalmanChanged);
    layout->addWidget(kalmanSlider_);

    delayCompSlider_ = new ParamSlider("延迟矫正补偿", 0, 50, 5, 0, " ms", group);
    connect(delayCompSlider_, &ParamSlider::valueChanged, this, &AimPage::onDelayCompChanged);
    layout->addWidget(delayCompSlider_);
}

// ── 瞄准设置（左侧参数 + 右侧人体示意图）──
void AimPage::setupAimSettings(QGroupBox* group) {
    auto* hlayout = new QHBoxLayout(group);

    // 左侧参数
    auto* leftLayout = new QVBoxLayout();

    rangeSizeSlider_ = new ParamSlider("范围大小", 1, 100, 21, 0, " %", group);
    connect(rangeSizeSlider_, &ParamSlider::valueChanged, this, &AimPage::onRangeSizeChanged);
    leftLayout->addWidget(rangeSizeSlider_);

    // 范围布局
    auto* layoutLabel = new QLabel("范围布局", group);
    layoutLabel->setStyleSheet("font-size: 14px; color: #333333; margin-top: 8px;");
    leftLayout->addWidget(layoutLabel);
    auto* layoutRow = new QHBoxLayout();
    radioCircle_ = new QRadioButton("圆形", group);
    radioCircle_->setChecked(true);
    radioRect_   = new QRadioButton("矩形", group);
    radioCircle_->setStyleSheet("font-size: 14px; color: #333333;");
    radioRect_->setStyleSheet("font-size: 14px; color: #333333;");
    connect(radioCircle_, &QRadioButton::toggled, this, &AimPage::onRangeLayoutChanged);
    layoutRow->addWidget(radioCircle_);
    layoutRow->addWidget(radioRect_);
    layoutRow->addStretch();
    leftLayout->addLayout(layoutRow);

    // 对准部位
    auto* partLabel = new QLabel("对准部位", group);
    partLabel->setStyleSheet("font-size: 14px; color: #333333; margin-top: 8px;");
    leftLayout->addWidget(partLabel);
    auto* partRow = new QHBoxLayout();
    btnHead_    = new QPushButton("对准头部", group);
    btnChest_   = new QPushButton("对准胸部", group);
    btnAbdomen_ = new QPushButton("对准腹部", group);
    for (auto* b : {btnHead_, btnChest_, btnAbdomen_}) {
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(32);
        b->setStyleSheet(
            "QPushButton { font-size: 13px; padding: 4px 12px; border: 1px solid #D9D9D9;"
            " border-radius: 4px; background: #FFFFFF; color: #333333; }"
            "QPushButton:checked { background: #1890FF; color: white; border-color: #1890FF; }"
            "QPushButton:hover { border-color: #1890FF; }");
        partRow->addWidget(b);
    }
    partRow->addStretch();
    leftLayout->addLayout(partRow);
    btnHead_->setChecked(true);
    connect(btnHead_,    &QPushButton::clicked, [this]() { onBodyPartClicked(0); });
    connect(btnChest_,   &QPushButton::clicked, [this]() { onBodyPartClicked(1); });
    connect(btnAbdomen_, &QPushButton::clicked, [this]() { onBodyPartClicked(2); });

    // PID 平滑
    auto* smoothSlider = new ParamSlider("平滑系数", 0.1, 1.0, 1.0, 1, "", group);
    connect(smoothSlider, &ParamSlider::valueChanged, this, &AimPage::onSmoothingChanged);
    leftLayout->addWidget(smoothSlider);

    leftLayout->addStretch();
    hlayout->addLayout(leftLayout);

    // 右侧人体示意图
    auto* bodyWidget = new QWidget(group);
    bodyWidget->setFixedWidth(120);
    bodyWidget->setMinimumHeight(200);
    connect(bodyWidget, &QWidget::destroyed, [](){});
    // 使用 paintEvent 绘制
    auto* bodyLabel = new QLabel(group);
    bodyLabel->setFixedWidth(120);
    bodyLabel->setMinimumHeight(200);
    bodyLabel->setStyleSheet("background: #FAFAFA; border: 1px solid #E8E8E8; border-radius: 8px;");
    hlayout->addWidget(bodyLabel);
}

// ── 热键 ──
void AimPage::setupHotkeys(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);

    auto* row1 = new QHBoxLayout();
    auto* lbl1 = new QLabel("自瞄开关", group);
    lbl1->setStyleSheet("font-size: 14px; color: #333333;");
    hotkeyAim_ = new QPushButton("鼠标右键", group);
    hotkeyAim_->setCursor(Qt::PointingHandCursor);
    hotkeyAim_->setMinimumWidth(120);
    hotkeyAim_->setStyleSheet(
        "QPushButton { font-size: 13px; padding: 4px 12px; border: 1px solid #D9D9D9;"
        " border-radius: 4px; background: #FFFFFF; color: #333333; }"
        "QPushButton:hover { border-color: #1890FF; }");
    row1->addWidget(lbl1);
    row1->addStretch();
    row1->addWidget(hotkeyAim_);
    layout->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    auto* lbl2 = new QLabel("切换目标", group);
    lbl2->setStyleSheet("font-size: 14px; color: #333333;");
    hotkeySwitch_ = new QPushButton("鼠标侧键1", group);
    hotkeySwitch_->setCursor(Qt::PointingHandCursor);
    hotkeySwitch_->setMinimumWidth(120);
    hotkeySwitch_->setStyleSheet(
        "QPushButton { font-size: 13px; padding: 4px 12px; border: 1px solid #D9D9D9;"
        " border-radius: 4px; background: #FFFFFF; color: #333333; }"
        "QPushButton:hover { border-color: #1890FF; }");
    row2->addWidget(lbl2);
    row2->addStretch();
    row2->addWidget(hotkeySwitch_);
    layout->addLayout(row2);
}

// ── 槽函数 ──
void AimPage::onKpChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.kp = val;
    engine_->UpdateConfig(cfg);
}

void AimPage::onKiChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.ki = val;
    engine_->UpdateConfig(cfg);
}

void AimPage::onKalmanChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.kalman_ms = static_cast<int>(val);
    engine_->UpdateConfig(cfg);
}

void AimPage::onDelayCompChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.delay_comp_ms = static_cast<int>(val);
    engine_->UpdateConfig(cfg);
}

void AimPage::onRangeSizeChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.aim_range_size = static_cast<int>(val);
    engine_->UpdateConfig(cfg);
}

void AimPage::onBodyPartClicked(int part) {
    activeBodyPart_ = part;
    btnHead_->setChecked(part == 0);
    btnChest_->setChecked(part == 1);
    btnAbdomen_->setChecked(part == 2);
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.target_body_part = part;
    cfg.target_y_ratio = (part == 0 ? 0.15f : part == 1 ? 0.35f : 0.55f);
    engine_->UpdateConfig(cfg);
}

void AimPage::onSmoothingChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.aim_smoothing = val;
    engine_->UpdateConfig(cfg);
}

void AimPage::onRangeLayoutChanged() {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.aim_range_circle = radioCircle_->isChecked();
    engine_->UpdateConfig(cfg);
}
