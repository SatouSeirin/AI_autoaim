#pragma once

#include <QWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QGroupBox>
#include "../widgets/paramslider.h"

class AimEngine;

class AimPage : public QWidget {
    Q_OBJECT

public:
    explicit AimPage(QWidget* parent = nullptr);
    void setEngine(AimEngine* engine);
     void syncFromConfig();

private slots:
    void onKpChanged(double val);
    void onKiChanged(double val);
    void onKdChanged(double val);
    void onPredictStepsChanged(double val);
    void onSensitivityChanged(double val);
    void onRangeSizeChanged(double val);
    void onBodyPartClicked(int part);
    void onRangeLayoutChanged();

private:
    void setupMouseCurve(QGroupBox* group);
    void setupAimSettings(QGroupBox* group);

    AimEngine* engine_ = nullptr;

    ParamSlider* kpSlider_           = nullptr;
    ParamSlider* kiSlider_           = nullptr;
    ParamSlider* kdSlider_           = nullptr;
    ParamSlider* predictStepsSlider_ = nullptr;
    ParamSlider* sensitivitySlider_  = nullptr;
    ParamSlider* rangeSizeSlider_    = nullptr;

    QRadioButton* radioCircle_    = nullptr;
    QRadioButton* radioRect_      = nullptr;
    QPushButton*  btnHead_        = nullptr;
    QPushButton*  btnChest_       = nullptr;
    QPushButton*  btnAbdomen_     = nullptr;
    int           activeBodyPart_ = 0;


};
