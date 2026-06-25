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

    auto* curveGroup = new QGroupBox("PID 控制", this);
    setupMouseCurve(curveGroup);
    mainLayout->addWidget(curveGroup);

    auto* aimGroup = new QGroupBox("瞄准设置", this);
    setupAimSettings(aimGroup);
    mainLayout->addWidget(aimGroup);

mainLayout->addStretch();
}

void AimPage::setEngine(AimEngine* engine) { engine_ = engine; }

void AimPage::setupMouseCurve(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);

    kpSlider_ = new ParamSlider("Kp", 0.001, 3.0, 0.25, 3, "", group);
    connect(kpSlider_, &ParamSlider::valueChanged, this, &AimPage::onKpChanged);
    layout->addWidget(kpSlider_);

    kiSlider_ = new ParamSlider("Ki", 0.0, 0.05, 0.0, 3, "", group);
    connect(kiSlider_, &ParamSlider::valueChanged, this, &AimPage::onKiChanged);
    layout->addWidget(kiSlider_);

    kdSlider_ = new ParamSlider("Kd", 0.001, 0.5, 0.08, 3, "", group);
    connect(kdSlider_, &ParamSlider::valueChanged, this, &AimPage::onKdChanged);
    layout->addWidget(kdSlider_);

    predictStepsSlider_ = new ParamSlider("提前预测", 0, 20, 3, 0, " 帧", group);
    connect(predictStepsSlider_, &ParamSlider::valueChanged, this, &AimPage::onPredictStepsChanged);
    layout->addWidget(predictStepsSlider_);
}

void AimPage::setupAimSettings(QGroupBox* group) {
    auto* hlayout = new QHBoxLayout(group);
    auto* leftLayout = new QVBoxLayout();

    rangeSizeSlider_ = new ParamSlider("范围大小", 1, 100, 21, 0, " %", group);
    connect(rangeSizeSlider_, &ParamSlider::valueChanged, this, &AimPage::onRangeSizeChanged);
    leftLayout->addWidget(rangeSizeSlider_);

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

    auto* partLabel = new QLabel("瞄准部位", group);
    partLabel->setStyleSheet("font-size: 14px; color: #333333; margin-top: 8px;");
    leftLayout->addWidget(partLabel);
    auto* partRow = new QHBoxLayout();
    btnHead_    = new QPushButton("头部", group);
    btnChest_   = new QPushButton("胸部", group);
    btnAbdomen_ = new QPushButton("腹部", group);
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

    sensitivitySlider_ = new ParamSlider("灵敏度", 0.01, 1.0, 0.15, 2, "", group);
    connect(sensitivitySlider_, &ParamSlider::valueChanged, this, &AimPage::onSensitivityChanged);
    leftLayout->addWidget(sensitivitySlider_);

    leftLayout->addStretch();
    hlayout->addLayout(leftLayout);

    auto* bodyLabel = new QLabel(group);
    bodyLabel->setFixedWidth(120);
    bodyLabel->setMinimumHeight(200);
    bodyLabel->setStyleSheet("background: #FAFAFA; border: 1px solid #E8E8E8; border-radius: 8px;");
    hlayout->addWidget(bodyLabel);
}


void AimPage::onKpChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig(); cfg.kp = val; engine_->UpdateConfig(cfg);
}
void AimPage::onKiChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig(); cfg.ki = val; engine_->UpdateConfig(cfg);
}
void AimPage::onKdChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig(); cfg.kd = val; engine_->UpdateConfig(cfg);
}
void AimPage::onPredictStepsChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig(); cfg.kalman_prediction_steps = static_cast<int>(val); engine_->UpdateConfig(cfg);
}
void AimPage::onSensitivityChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig(); cfg.sensitivity = val; engine_->UpdateConfig(cfg);
}
void AimPage::onRangeSizeChanged(double val) {
    if (!engine_) return;
    auto cfg = engine_->GetConfig(); cfg.aim_range_size = static_cast<int>(val); engine_->UpdateConfig(cfg);
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
void AimPage::onRangeLayoutChanged() {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    cfg.aim_range_circle = radioCircle_->isChecked();
    engine_->UpdateConfig(cfg);
}
