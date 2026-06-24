#include "paramslider.h"
#include <QHBoxLayout>
#include <cmath>

ParamSlider::ParamSlider(const QString& label, double min, double max,
                         double value, int decimals,
                         const QString& suffix, QWidget* parent)
    : QWidget(parent), min_(min), max_(max), decimals_(decimals) {

    // 整数参数(decimals=0)也至少给 100 级精度，否则滑块只能选最小/最大值
    precision_ = std::max(static_cast<int>(std::pow(10, decimals_)), 100);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 4);

    // 1) 标签
    label_ = new QLabel(label, this);
    label_->setFixedWidth(130);
    label_->setStyleSheet("font-size: 14px; color: #333333;");
    layout->addWidget(label_);

    // 2) 滑块（内部用整数 0~precision_ 映射到 min~max）
    slider_ = new QSlider(Qt::Horizontal, this);
    slider_->setRange(0, precision_);
    slider_->setValue(static_cast<int>((value - min) / (max - min) * precision_));
    layout->addWidget(slider_, 1);  // 弹性占用

    // 3) 数字框
    spinBox_ = new QDoubleSpinBox(this);
    spinBox_->setRange(min, max);
    spinBox_->setDecimals(decimals_);
    spinBox_->setValue(value);
    spinBox_->setFixedWidth(80);
    spinBox_->setSuffix(suffix);
    spinBox_->setStyleSheet(
        "QDoubleSpinBox { border: 1px solid #D9D9D9; border-radius: 4px;"
        " padding: 3px 6px; font-size: 14px; color: #333333; }"
        "QDoubleSpinBox:focus { border-color: #1890FF; }");
    layout->addWidget(spinBox_);

    // 4) 信号连接（双向绑定，防递归）
    connect(slider_, &QSlider::valueChanged, this, &ParamSlider::onSliderChanged);
    connect(spinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamSlider::onSpinBoxChanged);
}

double ParamSlider::value() const {
    return spinBox_->value();
}

void ParamSlider::setValue(double v) {
    updating_ = true;
    spinBox_->setValue(v);
    slider_->setValue(static_cast<int>((v - min_) / (max_ - min_) * precision_));
    updating_ = false;
}

void ParamSlider::onSliderChanged(int intVal) {
    if (updating_) return;
    updating_ = true;
    double v = min_ + static_cast<double>(intVal) / precision_ * (max_ - min_);
    spinBox_->setValue(v);
    updating_ = false;
    emit valueChanged(v);
}

void ParamSlider::onSpinBoxChanged(double doubleVal) {
    if (updating_) return;
    updating_ = true;
    slider_->setValue(static_cast<int>((doubleVal - min_) / (max_ - min_) * precision_));
    updating_ = false;
    emit valueChanged(doubleVal);
}
