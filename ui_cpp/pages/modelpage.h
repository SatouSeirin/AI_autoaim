#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QFutureWatcher>
#include <vector>
#include "../widgets/dragdroparea.h"
#include "../widgets/paramslider.h"

// ============================================================
// ModelPage — v2.1（.onnx + .engine 双格式支持 + 类别勾选）
// ============================================================
class AimEngine;

class ModelPage : public QWidget {
    Q_OBJECT

public:
    explicit ModelPage(QWidget* parent = nullptr);
    void setEngine(AimEngine* engine);
    void syncSlidersFromConfig();
    void refreshClassCheckboxes();
    void loadModelFromPath(const QString& path);  // 模型加载后刷新类别勾选

signals:
    void modelLoaded(const QString& path, bool success);
    void engineStarted();
    void engineStopped();

public slots:
    void onStartStop();

private slots:
    void onFileDropped(const QString& path);
    void onModelLoadFinished();
    void onConfidenceChanged(double val);
    void onNMSChanged(double val);
    void onClassToggled(int classId, bool checked);
    void updateStatusLabel();

private:
    void startAsyncLoad(const QString& path, bool isEngine);
    void updateClassCheckboxesToEngine();

    DragDropArea*  dragArea_       = nullptr;
    QLabel*        modelStatus_    = nullptr;
    ParamSlider*   confSlider_     = nullptr;
    ParamSlider*   nmsSlider_      = nullptr;
    QPushButton*   startStopBtn_   = nullptr;
    QLabel*        fpsLabel_       = nullptr;

    // 类别勾选
    QGroupBox*     classGroup_     = nullptr;
    QWidget*       classContainer_ = nullptr;  // 承载 checkbox grid 的容器
    std::vector<QCheckBox*> classCheckboxes_;

    AimEngine*     engine_         = nullptr;
    bool           modelLoaded_    = false;
    bool           engineRunning_  = false;
    bool           loading_        = false;
    int            timerId_        = 0;

    QFutureWatcher<bool>* loadWatcher_ = nullptr;
    QString               pendingModelPath_;

protected:
    void timerEvent(QTimerEvent* event) override;
};
