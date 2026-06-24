#pragma once

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>

// ============================================================
// ParamSlider — v2.0 MVP
// 标签 (120px) + 滑块 (弹性) + 数字框 (70px)，双向绑定
// ============================================================
class ParamSlider : public QWidget {
    Q_OBJECT

public:
    // label: 参数名称
    // min/max/value: 范围与默认值
    // decimals: 小数位数 (0=整数步进)
    // suffix: 数字框后缀（如 " ms", " %"）
    explicit ParamSlider(const QString& label, double min, double max,
                         double value, int decimals = 2,
                         const QString& suffix = "",
                         QWidget* parent = nullptr);

    double value() const;
    void   setValue(double v);

signals:
    void valueChanged(double newValue);

private slots:
    void onSliderChanged(int intVal);
    void onSpinBoxChanged(double doubleVal);

private:
    QLabel*          label_;
    QSlider*         slider_;
    QDoubleSpinBox*  spinBox_;
    double           min_;
    double           max_;
    int              decimals_;
    int              precision_;    // 滑块精度 (100~10000)
    bool             updating_ = false;  // 防止递归更新
};
