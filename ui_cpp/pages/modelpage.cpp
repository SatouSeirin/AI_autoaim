#include "modelpage.h"
#include "../../src/engine.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QApplication>
#include <algorithm>

ModelPage::ModelPage(QWidget* parent)
    : QWidget(parent) {

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(24);

    // ── 1) 导入模型 ──
    auto* importGroup = new QGroupBox("📁 导入模型", this);
    auto* importLayout = new QVBoxLayout(importGroup);

    dragArea_ = new DragDropArea(importGroup);
    importLayout->addWidget(dragArea_);
    connect(dragArea_, &DragDropArea::fileDropped, this, &ModelPage::onFileDropped);

    mainLayout->addWidget(importGroup);

    // ── 2) 当前模型状态 ──
    auto* statusGroup = new QGroupBox("📦 当前模型", this);
    auto* statusLayout = new QVBoxLayout(statusGroup);

    modelStatus_ = new QLabel("未加载模型", statusGroup);
    modelStatus_->setAlignment(Qt::AlignCenter);
    modelStatus_->setObjectName("statusNoModel");
    modelStatus_->setMinimumHeight(40);
    modelStatus_->setStyleSheet(
        "font-size: 16px; padding: 10px; background: #F5F5F5; border-radius: 4px;");
    statusLayout->addWidget(modelStatus_);

    mainLayout->addWidget(statusGroup);

    // ── 3) 目标类别勾选 ──
    classGroup_ = new QGroupBox("🎯 目标类别", this);
    auto* classOuterLayout = new QVBoxLayout(classGroup_);
    classOuterLayout->setSpacing(0);
    classOuterLayout->setContentsMargins(0, 0, 0, 0);
    classContainer_ = new QWidget(classGroup_);
    classOuterLayout->addWidget(classContainer_);
    mainLayout->addWidget(classGroup_);
    // 初始化默认勾选（Class 0 默认选中）
    refreshClassCheckboxes();

    // ── 4) 推理参数 ──
    auto* paramGroup = new QGroupBox("⚙ 推理参数", this);
    auto* paramLayout = new QVBoxLayout(paramGroup);

    confSlider_ = new ParamSlider("置信度阈值", 0.01, 1.00, 0.50, 2, "", paramGroup);
    paramLayout->addWidget(confSlider_);
    connect(confSlider_, &ParamSlider::valueChanged, this, &ModelPage::onConfidenceChanged);

    nmsSlider_ = new ParamSlider("NMS 阈值", 0.01, 1.00, 0.40, 2, "", paramGroup);
    paramLayout->addWidget(nmsSlider_);
    connect(nmsSlider_, &ParamSlider::valueChanged, this, &ModelPage::onNMSChanged);

    // ── 5) FPS + 开始/停止按钮 ──
    auto* bottomRow = new QHBoxLayout();
    fpsLabel_ = new QLabel("FPS: --", this);
    fpsLabel_->setStyleSheet("font-size: 14px; color: #888888;");
    bottomRow->addWidget(fpsLabel_);
    bottomRow->addStretch();

    startStopBtn_ = new QPushButton("▶ 开始推理", this);
    startStopBtn_->setObjectName("btnPrimary");
    startStopBtn_->setCursor(Qt::PointingHandCursor);
    startStopBtn_->setEnabled(false);
    startStopBtn_->setMinimumWidth(140);
    startStopBtn_->setMinimumHeight(36);
    connect(startStopBtn_, &QPushButton::clicked, this, &ModelPage::onStartStop);
    bottomRow->addWidget(startStopBtn_);

    paramLayout->addLayout(bottomRow);
    mainLayout->addWidget(paramGroup);

    mainLayout->addStretch();

    timerId_ = startTimer(500);

    loadWatcher_ = new QFutureWatcher<bool>(this);
    connect(loadWatcher_, &QFutureWatcher<bool>::finished, this, &ModelPage::onModelLoadFinished);
}

void ModelPage::setEngine(AimEngine* engine) {
    engine_ = engine;
    if (engine_ && !classCheckboxes_.empty()) {
        // 同步当前 UI checkbox 勾选状态到引擎
        updateClassCheckboxesToEngine();
    }
}

// ── 拖拽/选择 ONNX 或 .engine ──
void ModelPage::onFileDropped(const QString& path) {
    if (!engine_) {
        QMessageBox::warning(this, "错误", "引擎未初始化");
        return;
    }

    if (loading_) {
        QMessageBox::information(this, "请稍候", "模型正在加载中，请等待完成后再试。");
        return;
    }

    // 中文路径检测
    for (const QChar& c : path) {
        if (c.unicode() > 127) {
            QMessageBox::warning(this, "路径错误",
                QString("模型路径包含中文字符，请将模型移动到不含中文的路径下再加载。\n\n当前路径：%1").arg(path));
            return;
        }
    }

    bool isEngine = path.endsWith(".engine", Qt::CaseInsensitive);
    bool isOnnx   = path.endsWith(".onnx", Qt::CaseInsensitive);

    if (!isEngine && !isOnnx) {
        QMessageBox::warning(this, "格式不支持",
            "仅支持 .onnx 和 .engine 格式的模型文件。");
        return;
    }

    startAsyncLoad(path, isEngine);
}

void ModelPage::startAsyncLoad(const QString& path, bool isEngine) {
    loading_ = true;
    pendingModelPath_ = path;
    dragArea_->setEnabled(false);
    startStopBtn_->setEnabled(false);

    if (isEngine) {
        modelStatus_->setText("⚡ 正在加载预编译引擎...");
    } else {
        modelStatus_->setText("⏳ 正在构建 TensorRT 引擎，预计 30-60 秒...");
    }
    modelStatus_->setStyleSheet(
        "font-size: 16px; padding: 10px; background: #FFF7E6;"
        " border: 1px solid #FFD591; border-radius: 4px; color: #FA8C16;");
    QApplication::processEvents();

    std::string modelPath = path.toStdString();
    QFuture<bool> future = QtConcurrent::run([this, modelPath, isEngine]() -> bool {
        if (isEngine) {
            return engine_->LoadEngineFile(modelPath);
        } else {
            return engine_->LoadModel(modelPath);
        }
    });
    loadWatcher_->setFuture(future);
}

void ModelPage::onModelLoadFinished() {
    loading_ = false;
    dragArea_->setEnabled(true);

    bool success = loadWatcher_->result();

    if (!success) {
        QString errMsg = QString::fromStdString(engine_->GetLastError());
        if (errMsg.isEmpty()) {
            errMsg = "模型加载失败，请检查文件是否有效。";
        }
        modelStatus_->setText("未加载模型");
        modelStatus_->setStyleSheet(
            "font-size: 16px; padding: 10px; background: #F5F5F5; border-radius: 4px;");
        QMessageBox::critical(this, "加载失败", errMsg);
        emit modelLoaded(pendingModelPath_, false);
        return;
    }

    modelLoaded_ = true;
    startStopBtn_->setEnabled(true);
    refreshClassCheckboxes();  // 刷新类别勾选

    // 同步引擎配置到 UI 滑块（config.json 可能有异常值）
    if (engine_) {
        auto cfg = engine_->GetConfig();
        confSlider_->blockSignals(true);
        confSlider_->setValue(cfg.confidence_threshold);
        confSlider_->blockSignals(false);
        nmsSlider_->blockSignals(true);
        nmsSlider_->setValue(cfg.nms_threshold);
        nmsSlider_->blockSignals(false);
    }

    updateStatusLabel();
    emit modelLoaded(pendingModelPath_, true);
}

void ModelPage::onStartStop() {
    if (!engine_ || !modelLoaded_) return;

    if (engineRunning_) {
        engine_->Stop();
        engineRunning_ = false;
        startStopBtn_->setText("▶ 开始推理");
        emit engineStopped();
    } else {
        engine_->Start();
        engineRunning_ = true;
        startStopBtn_->setText("⏹ 停止推理");
        emit engineStarted();
    }
}

void ModelPage::onConfidenceChanged(double val) {
    if (engine_) {
        auto cfg = engine_->GetConfig();
        cfg.confidence_threshold = static_cast<float>(val);
        engine_->UpdateConfig(cfg);
    }
}

void ModelPage::onNMSChanged(double val) {
    if (engine_) {
        auto cfg = engine_->GetConfig();
        cfg.nms_threshold = static_cast<float>(val);
        engine_->UpdateConfig(cfg);
    }
}

void ModelPage::updateStatusLabel() {
    if (!modelLoaded_) {
        modelStatus_->setText("未加载模型");
        modelStatus_->setObjectName("statusNoModel");
        modelStatus_->setStyleSheet(modelStatus_->styleSheet());
        return;
    }

    if (engine_) {
        auto cfg = engine_->GetConfig();
        QFileInfo fi(QString::fromStdString(cfg.model_path));
        bool isEngine = (cfg.model_path.rfind(".engine") != std::string::npos);
        QString icon = isEngine ? "⚡" : "✅";
        modelStatus_->setText(QString("%1  %2").arg(icon, fi.fileName()));
        modelStatus_->setObjectName("statusRunning");
        modelStatus_->setStyleSheet(
            "font-size: 16px; padding: 10px; background: #F0F9F0;"
            " border: 1px solid #B7EB8F; border-radius: 4px; color: #52C41A;");
    }

    startStopBtn_->setEnabled(true);
}

void ModelPage::timerEvent(QTimerEvent* event) {
    if (event->timerId() == timerId_ && engine_ && engineRunning_) {
        double fps = engine_->GetFPS();
        double ms  = engine_->GetInferMs();
        fpsLabel_->setText(QString("FPS: %1 | 推理: %2 ms").arg(fps, 0, 'f', 1).arg(ms, 0, 'f', 1));
    }
}

// ── 刷新类别勾选（模型加载后调用，或构造函数中初始化）──
void ModelPage::refreshClassCheckboxes() {
    if (!classContainer_) return;

    // 从引擎获取当前配置（如果引擎未初始化则用默认值）
    std::vector<int> curIds;
    if (engine_) {
        auto cfg = engine_->GetConfig();
        curIds = cfg.target_class_ids;
    }
    if (curIds.empty()) curIds = {0};  // 默认 Class 0
    std::sort(curIds.begin(), curIds.end());

    int numClasses = 10;  // 默认最大类别数

    // 完全重建 container 的 layout（旧 layout 及旧 widget 自动销毁）
    delete classContainer_->layout();
    classCheckboxes_.clear();

    auto* grid = new QGridLayout(classContainer_);
    grid->setSpacing(4);
    grid->setContentsMargins(4, 4, 4, 4);

    for (int i = 0; i < numClasses; i++) {
        bool checked = std::find(curIds.begin(), curIds.end(), i) != curIds.end();
        auto* cb = new QCheckBox(QString("Class %1").arg(i), classContainer_);
        cb->setChecked(checked);
        cb->setStyleSheet("QCheckBox::indicator { width: 18px; height: 18px; }"
                          "QCheckBox { font-size: 13px; padding: 2px 4px; }");

        int clsId = i;
        connect(cb, &QCheckBox::toggled, this, [this, clsId](bool checked) {
            onClassToggled(clsId, checked);
        });

        int row = i / 5;
        int col = i % 5;
        grid->addWidget(cb, row, col);
        classCheckboxes_.push_back(cb);
    }
}

void ModelPage::onClassToggled(int classId, bool checked) {
    if (!engine_) return;

    auto cfg = engine_->GetConfig();
    auto& ids = cfg.target_class_ids;

    if (checked) {
        if (std::find(ids.begin(), ids.end(), classId) == ids.end()) {
            ids.push_back(classId);
            std::sort(ids.begin(), ids.end());
        }
    } else {
        ids.erase(std::remove(ids.begin(), ids.end(), classId), ids.end());
    }

    // 至少保留一个
    if (ids.empty()) {
        ids.push_back(classId);
        // 把刚才取消的 checkbox 设回 checked（阻断信号避免递归）
        if (classId >= 0 && static_cast<size_t>(classId) < classCheckboxes_.size()) {
            classCheckboxes_[classId]->blockSignals(true);
            classCheckboxes_[classId]->setChecked(true);
            classCheckboxes_[classId]->blockSignals(false);
        }
        return;
    }

    engine_->UpdateConfig(cfg);
}

void ModelPage::updateClassCheckboxesToEngine() {
    // 同步当前勾选到引擎
    if (!engine_) return;

    auto cfg = engine_->GetConfig();
    cfg.target_class_ids.clear();
    for (size_t i = 0; i < classCheckboxes_.size(); i++) {
        if (classCheckboxes_[i]->isChecked()) {
            cfg.target_class_ids.push_back(static_cast<int>(i));
        }
    }
    if (cfg.target_class_ids.empty()) {
        cfg.target_class_ids = {0};
    }
    engine_->UpdateConfig(cfg);
}
