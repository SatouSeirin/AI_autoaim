#include "engineviewmodel.h"
#include "../../src/config_manager.hpp"
#include <QFileInfo>
#include <QMetaObject>

EngineViewModel::EngineViewModel(FrameProvider* frameProvider, QObject* parent)
    : QObject(parent)
    , m_engine(std::make_unique<AimEngine>())
    , m_config(new ConfigViewModel(this))
    , m_frameProvider(frameProvider)
{
    // 从 JSON 加载配置
    AimConfig cfg = ConfigManager::Load("config.json");
    m_config->fromAimConfig(cfg);
    m_engine->SetConfig(cfg);

    // 配置变更防抖（200ms）
    m_configDebounceTimer = new QTimer(this);
    m_configDebounceTimer->setSingleShot(true);
    m_configDebounceTimer->setInterval(200);
    connect(m_configDebounceTimer, &QTimer::timeout, this, &EngineViewModel::onConfigDirty);
    connect(m_config, &ConfigViewModel::configDirty, this, [this]() {
        m_configDebounceTimer->start();
    });

    // FPS 定时器
    m_fpsTimer = new QTimer(this);
    connect(m_fpsTimer, &QTimer::timeout, this, &EngineViewModel::onFpsTimer);
    m_fpsTimer->start(500);

    // 帧回调
    setupFrameCallback();
}

EngineViewModel::~EngineViewModel() {
    if (m_engine && m_engine->IsRunning()) {
        m_engine->Stop();
    }
    // 保存配置
    ConfigManager::Save(m_config->toAimConfig(), "config.json");
}

// ── 状态读取 ──

bool EngineViewModel::isRunning() const {
    return m_engine && m_engine->IsRunning();
}

bool EngineViewModel::isModelLoaded() const {
    return m_engine && m_engine->IsModelLoaded();
}

QString EngineViewModel::currentModelName() const {
    return m_currentModelName;
}

QString EngineViewModel::statusText() const {
    if (isRunning()) return QStringLiteral("推理运行中");
    return QStringLiteral("系统就绪");
}

double EngineViewModel::fps() const {
    return m_engine ? m_engine->GetFPS() : 0.0;
}

double EngineViewModel::inferMs() const {
    return m_engine ? m_engine->GetInferMs() : 0.0;
}

qint64 EngineViewModel::frameCounter() const {
    return m_frameProvider ? m_frameProvider->frameCounter() : 0;
}

ConfigViewModel* EngineViewModel::config() const {
    return m_config;
}

QString EngineViewModel::lastError() const {
    if (m_engine) {
        return QString::fromStdString(m_engine->GetLastError());
    }
    return m_lastError;
}

// ── 控制方法 ──

void EngineViewModel::start() {
    if (!m_engine || m_engine->IsRunning()) return;
    syncConfigToEngine();
    m_engine->Start();
    emit runningChanged();
    emit statusTextChanged();
}

void EngineViewModel::stop() {
    if (!m_engine || !m_engine->IsRunning()) return;
    m_engine->Stop();
    emit runningChanged();
    emit statusTextChanged();
}

void EngineViewModel::toggleRunning() {
    if (isRunning()) stop();
    else start();
}

void EngineViewModel::loadModel(const QString& path) {
    if (!m_engine) return;

    emit modelLoadStarted();
    m_currentModelName = QFileInfo(path).fileName();

    bool ok = false;
    if (path.endsWith(".engine", Qt::CaseInsensitive)) {
        ok = m_engine->LoadEngineFile(path.toStdString());
    } else {
        ok = m_engine->LoadModel(path.toStdString());
    }

    if (ok) {
        m_config->setModelPath(path);
        emit currentModelNameChanged();
        emit modelLoadedChanged();
    } else {
        m_lastError = QString::fromStdString(m_engine->GetLastError());
        emit lastErrorChanged();
        emit errorOccurred(m_lastError);
    }
    emit modelLoadFinished(ok);
}

void EngineViewModel::applyConfig() {
    syncConfigToEngine();
}

bool EngineViewModel::saveProfile(const QString& filepath) {
    AimConfig cfg = m_config->toAimConfig();
    return ConfigManager::Save(cfg, filepath.toStdString());
}

bool EngineViewModel::loadProfile(const QString& filepath) {
    AimConfig cfg = ConfigManager::Load(filepath.toStdString());
    if (cfg.model_path.empty() && m_config->modelPath().isEmpty()) {
        // 可能是空配置或加载失败
    }
    m_config->fromAimConfig(cfg);
    syncConfigToEngine();
    return true;
}

void EngineViewModel::setTargetClass(int classId, bool enabled) {
    QVariantList classes = m_config->targetClassIds();
    if (enabled) {
        if (!classes.contains(classId)) {
            classes.append(classId);
        }
    } else {
        classes.removeAll(classId);
    }
    m_config->setTargetClassIds(classes);
    emit targetClassesChanged();
}

bool EngineViewModel::isTargetClassEnabled(int classId) const {
    return m_config->targetClassIds().contains(classId);
}

QVariantList EngineViewModel::targetClassIds() const {
    return m_config->targetClassIds();
}

QVariantList EngineViewModel::detections() const {
    if (!m_engine) return {};
    auto dets = m_engine->GetLastDetections();
    QVariantList result;
    for (const auto& d : dets) {
        QVariantMap m;
        m["x1"] = d.x1;
        m["y1"] = d.y1;
        m["x2"] = d.x2;
        m["y2"] = d.y2;
        m["confidence"] = d.confidence;
        m["classId"] = d.class_id;
        result.append(m);
    }
    return result;
}

void EngineViewModel::testMove(const QString& direction) {
    if (!m_engine) return;
    double dx = 0, dy = 0;
    if (direction == "up")        dy = -20;
    else if (direction == "down")  dy = 20;
    else if (direction == "left")  dx = -20;
    else if (direction == "right") dx = 20;
    m_engine->MoveMouseRelative(dx, dy);
}

// ── 内部槽 ──

void EngineViewModel::onConfigDirty() {
    syncConfigToEngine();
}

void EngineViewModel::onFpsTimer() {
    emit fpsChanged();
    emit inferMsChanged();
}

// ── 帧回调 ──

void EngineViewModel::setupFrameCallback() {
    m_engine->SetFrameCallback([this](const cv::Mat& frame) {
        if (frame.empty()) return;

        // 更新帧提供者（线程安全）
        m_frameProvider->updateFrame(frame);

        // 通知 QML 刷新
        QMetaObject::invokeMethod(this, [this]() {
            emit frameReady();
            emit detectionsChanged();
        }, Qt::QueuedConnection);
    });
}

void EngineViewModel::syncConfigToEngine() {
    if (!m_engine) return;
    AimConfig cfg = m_config->toAimConfig();

    // 检测采集尺寸变更，需要单独调用 SetCaptureSize
    AimConfig currentCfg = m_engine->GetConfig();
    if (cfg.capture_size != currentCfg.capture_size) {
        m_engine->SetCaptureSize(cfg.capture_size);
    }

    m_engine->UpdateConfig(cfg);
}

void EngineViewModel::setHotkeyCapturing(bool v) {
    if (m_hotkeyCapturing == v) return;
    m_hotkeyCapturing = v;
    emit hotkeyCapturingChanged();
}
