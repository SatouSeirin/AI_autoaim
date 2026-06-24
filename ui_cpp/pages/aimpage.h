#pragma once

#include <QWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QGroupBox>
#include "../widgets/paramslider.h"

// ============================================================
// AimPage — v2.1 瞄准参数页
// 鼠标曲线 + 瞄准设置 + 人体示意图 + 热键绑定
// ============================================================
class AimEngine;

class AimPage : public QWidget {
    Q_OBJECT

public:
    explicit AimPage(QWidget* parent = nullptr);
    void setEngine(AimEngine* engine);

private slots:
    void onKpChanged(double val);
    void onKiChanged(double val);
    void onKalmanChanged(double val);
    void onDelayCompChanged(double val);
    void onRangeSizeChanged(double val);
    void onBodyPartClicked(int part);
    void onSmoothingChanged(double val);
    void onRangeLayoutChanged();

private:
    void setupMouseCurve(QGroupBox* group);
    void setupAimSettings(QGroupBox* group);
    void setupHotkeys(QGroupBox* group);
    void paintBodyDiagram(QPainter& painter, const QRect& rect);

    AimEngine* engine_ = nullptr;

    // 鼠标曲线
    ParamSlider* kpSlider_       = nullptr;
    ParamSlider* kiSlider_       = nullptr;
    ParamSlider* kalmanSlider_   = nullptr;
    ParamSlider* delayCompSlider_ = nullptr;

    // 瞄准设置
    ParamSlider* rangeSizeSlider_ = nullptr;
    QRadioButton* radioCircle_    = nullptr;
    QRadioButton* radioRect_      = nullptr;
    QPushButton*  btnHead_        = nullptr;
    QPushButton*  btnChest_       = nullptr;
    QPushButton*  btnAbdomen_     = nullptr;
    int           activeBodyPart_ = 0;

    // 热键
    QPushButton* hotkeyAim_    = nullptr;
    QPushButton* hotkeySwitch_ = nullptr;
};
