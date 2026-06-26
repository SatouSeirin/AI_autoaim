#include "configviewmodel.h"
#include <iostream>

ConfigViewModel::ConfigViewModel(QObject* parent)
    : QObject(parent) {
    m_targetClassIds.append(0);  // 默认目标类别 0
}

// ── AimConfig 互转 ──

AimConfig ConfigViewModel::toAimConfig() const {
    AimConfig cfg;
    cfg.model_path = m_modelPath.toStdString();
    cfg.enable_fp16 = m_enableFp16;
    cfg.confidence_threshold = static_cast<float>(m_confThreshold);
    cfg.nms_threshold = static_cast<float>(m_nmsThreshold);
    cfg.capture_size = m_captureSize;

    cfg.target_class_ids.clear();
    for (const auto& v : m_targetClassIds) {
        cfg.target_class_ids.push_back(v.toInt());
    }

    cfg.target_y_ratio = static_cast<float>(m_targetYRatio);
    cfg.target_body_part = m_targetBodyPart;
    cfg.aim_range_size = m_aimRangeSize;
    cfg.aim_range_circle = m_aimRangeCircle;
    cfg.kp = m_kp;
    cfg.ki = m_ki;
    cfg.kd = m_kd;
    cfg.kalman_prediction_steps = m_predictionSteps;
    cfg.sensitivity = m_sensitivity;
    cfg.aim_key = m_aimKey;
    cfg.exit_key = m_exitKey;
    cfg.switch_target_key = m_switchTargetKey;
    cfg.start_stop_key = m_startStopKey;
    cfg.mouse_backend = static_cast<MouseBackend>(m_mouseBackend);
    cfg.kmbox_ip = m_kmboxIp.toStdString();
    cfg.kmbox_port = m_kmboxPort.toStdString();
    cfg.kmbox_mac = m_kmboxMac.toStdString();
    cfg.kmbox_encrypt = m_kmboxEncrypt;
    cfg.kmbox_bezier = m_kmboxBezier;
    cfg.makcu_serial = m_makcuSerial.toStdString();
    cfg.makcu_port = m_makcuPort.toStdString();
    cfg.enable_visualization = m_enableVisualization;
    cfg.show_infer_latency = m_showInferLatency;
    cfg.draw_detection_boxes = m_drawDetectionBoxes;
    cfg.draw_aim_on_preview = m_drawAimOnPreview;
    cfg.debug_window_enabled = m_debugWindowEnabled;
    cfg.auto_start = m_autoStart;
    cfg.minimize_to_tray = m_minimizeToTray;
    cfg.language = m_language.toStdString();
    return cfg;
}

void ConfigViewModel::fromAimConfig(const AimConfig& cfg) {
    std::cout << "[ConfigVM] fromAimConfig loading:" << std::endl;
    std::cout << "  captureSize=" << cfg.capture_size << std::endl;
    std::cout << "  kp=" << cfg.kp << " ki=" << cfg.ki << " kd=" << cfg.kd << std::endl;
    std::cout << "  sensitivity=" << cfg.sensitivity << " predictionSteps=" << cfg.kalman_prediction_steps << std::endl;
    std::cout << "  aimRangeSize=" << cfg.aim_range_size << " aimRangeCircle=" << cfg.aim_range_circle << std::endl;
    std::cout << "  targetYRatio=" << cfg.target_y_ratio << " targetBodyPart=" << cfg.target_body_part << std::endl;
    std::cout << "  aimKey=0x" << std::hex << cfg.aim_key << " exitKey=0x" << cfg.exit_key
              << " switchTargetKey=0x" << cfg.switch_target_key << " startStopKey=0x" << cfg.start_stop_key << std::dec << std::endl;
    std::cout << "  targetClassIds=[";
    for (size_t i = 0; i < cfg.target_class_ids.size(); i++) {
        if (i > 0) std::cout << ",";
        std::cout << cfg.target_class_ids[i];
    }
    std::cout << "]" << std::endl;

    m_modelPath = QString::fromStdString(cfg.model_path);
    m_enableFp16 = cfg.enable_fp16;
    m_confThreshold = cfg.confidence_threshold;
    m_nmsThreshold = cfg.nms_threshold;
    m_captureSize = cfg.capture_size;

    m_targetClassIds.clear();
    for (int id : cfg.target_class_ids) {
        m_targetClassIds.append(id);
    }

    m_targetYRatio = cfg.target_y_ratio;
    m_targetBodyPart = cfg.target_body_part;
    m_aimRangeSize = cfg.aim_range_size;
    m_aimRangeCircle = cfg.aim_range_circle;
    m_kp = cfg.kp;
    m_ki = cfg.ki;
    m_kd = cfg.kd;
    m_predictionSteps = cfg.kalman_prediction_steps;
    m_sensitivity = cfg.sensitivity;
    m_aimKey = cfg.aim_key;
    m_exitKey = cfg.exit_key;
    m_switchTargetKey = cfg.switch_target_key;
    m_startStopKey = cfg.start_stop_key;
    m_mouseBackend = static_cast<int>(cfg.mouse_backend);
    m_kmboxIp = QString::fromStdString(cfg.kmbox_ip);
    m_kmboxPort = QString::fromStdString(cfg.kmbox_port);
    m_kmboxMac = QString::fromStdString(cfg.kmbox_mac);
    m_kmboxEncrypt = cfg.kmbox_encrypt;
    m_kmboxBezier = cfg.kmbox_bezier;
    m_makcuSerial = QString::fromStdString(cfg.makcu_serial);
    m_makcuPort = QString::fromStdString(cfg.makcu_port);
    m_enableVisualization = cfg.enable_visualization;
    m_showInferLatency = cfg.show_infer_latency;
    m_drawDetectionBoxes = cfg.draw_detection_boxes;
    m_drawAimOnPreview = cfg.draw_aim_on_preview;
    m_debugWindowEnabled = cfg.debug_window_enabled;
    m_autoStart = cfg.auto_start;
    m_minimizeToTray = cfg.minimize_to_tray;
    m_language = QString::fromStdString(cfg.language);

    // 发射所有变更信号
    emit modelPathChanged();
    emit enableFp16Changed();
    emit confThresholdChanged();
    emit nmsThresholdChanged();
    emit captureSizeChanged();
    emit targetClassIdsChanged();
    emit targetYRatioChanged();
    emit targetBodyPartChanged();
    emit aimRangeSizeChanged();
    emit aimRangeCircleChanged();
    emit kpChanged();
    emit kiChanged();
    emit kdChanged();
    emit predictionStepsChanged();
    emit sensitivityChanged();
    emit aimKeyChanged();
    emit exitKeyChanged();
    emit switchTargetKeyChanged();
    emit startStopKeyChanged();
    emit mouseBackendChanged();
    emit kmboxIpChanged();
    emit kmboxPortChanged();
    emit kmboxMacChanged();
    emit kmboxEncryptChanged();
    emit kmboxBezierChanged();
    emit makcuSerialChanged();
    emit makcuPortChanged();
    emit enableVisualizationChanged();
    emit showInferLatencyChanged();
    emit drawDetectionBoxesChanged();
    emit drawAimOnPreviewChanged();
    emit debugWindowEnabledChanged();
    emit autoStartChanged();
    emit minimizeToTrayChanged();
    emit languageChanged();
}

// ── Getters ──
QString ConfigViewModel::modelPath() const { return m_modelPath; }
bool ConfigViewModel::enableFp16() const { return m_enableFp16; }
double ConfigViewModel::confThreshold() const { return m_confThreshold; }
double ConfigViewModel::nmsThreshold() const { return m_nmsThreshold; }
int ConfigViewModel::captureSize() const { return m_captureSize; }
QVariantList ConfigViewModel::targetClassIds() const { return m_targetClassIds; }
double ConfigViewModel::targetYRatio() const { return m_targetYRatio; }
int ConfigViewModel::targetBodyPart() const { return m_targetBodyPart; }
int ConfigViewModel::aimRangeSize() const { return m_aimRangeSize; }
bool ConfigViewModel::aimRangeCircle() const { return m_aimRangeCircle; }
double ConfigViewModel::kp() const { return m_kp; }
double ConfigViewModel::ki() const { return m_ki; }
double ConfigViewModel::kd() const { return m_kd; }
int ConfigViewModel::predictionSteps() const { return m_predictionSteps; }
double ConfigViewModel::sensitivity() const { return m_sensitivity; }
int ConfigViewModel::aimKey() const { return m_aimKey; }
int ConfigViewModel::exitKey() const { return m_exitKey; }
int ConfigViewModel::switchTargetKey() const { return m_switchTargetKey; }
int ConfigViewModel::startStopKey() const { return m_startStopKey; }
int ConfigViewModel::mouseBackend() const { return m_mouseBackend; }
QString ConfigViewModel::kmboxIp() const { return m_kmboxIp; }
QString ConfigViewModel::kmboxPort() const { return m_kmboxPort; }
QString ConfigViewModel::kmboxMac() const { return m_kmboxMac; }
bool ConfigViewModel::kmboxEncrypt() const { return m_kmboxEncrypt; }
bool ConfigViewModel::kmboxBezier() const { return m_kmboxBezier; }
QString ConfigViewModel::makcuSerial() const { return m_makcuSerial; }
QString ConfigViewModel::makcuPort() const { return m_makcuPort; }
bool ConfigViewModel::enableVisualization() const { return m_enableVisualization; }
bool ConfigViewModel::showInferLatency() const { return m_showInferLatency; }
bool ConfigViewModel::drawDetectionBoxes() const { return m_drawDetectionBoxes; }
bool ConfigViewModel::drawAimOnPreview() const { return m_drawAimOnPreview; }
bool ConfigViewModel::debugWindowEnabled() const { return m_debugWindowEnabled; }
bool ConfigViewModel::autoStart() const { return m_autoStart; }
bool ConfigViewModel::minimizeToTray() const { return m_minimizeToTray; }
QString ConfigViewModel::language() const { return m_language; }

// ── Setters (宏定义减少重复代码) ──

#define IMPL_SETTER(Type, Name, name, signal) \
    void ConfigViewModel::set##Name(Type v) { \
        if (m_##name == v) return; \
        m_##name = v; \
        emit signal(); \
        emit configDirty(); \
    }

IMPL_SETTER(const QString&, ModelPath, modelPath, modelPathChanged)
IMPL_SETTER(bool, EnableFp16, enableFp16, enableFp16Changed)
IMPL_SETTER(double, ConfThreshold, confThreshold, confThresholdChanged)
IMPL_SETTER(double, NmsThreshold, nmsThreshold, nmsThresholdChanged)
IMPL_SETTER(int, CaptureSize, captureSize, captureSizeChanged)
IMPL_SETTER(double, TargetYRatio, targetYRatio, targetYRatioChanged)
IMPL_SETTER(int, TargetBodyPart, targetBodyPart, targetBodyPartChanged)
IMPL_SETTER(int, AimRangeSize, aimRangeSize, aimRangeSizeChanged)
IMPL_SETTER(bool, AimRangeCircle, aimRangeCircle, aimRangeCircleChanged)
IMPL_SETTER(double, Kp, kp, kpChanged)
IMPL_SETTER(double, Ki, ki, kiChanged)
IMPL_SETTER(double, Kd, kd, kdChanged)
IMPL_SETTER(int, PredictionSteps, predictionSteps, predictionStepsChanged)
IMPL_SETTER(double, Sensitivity, sensitivity, sensitivityChanged)
IMPL_SETTER(int, AimKey, aimKey, aimKeyChanged)
IMPL_SETTER(int, ExitKey, exitKey, exitKeyChanged)
IMPL_SETTER(int, SwitchTargetKey, switchTargetKey, switchTargetKeyChanged)
IMPL_SETTER(int, StartStopKey, startStopKey, startStopKeyChanged)
IMPL_SETTER(int, MouseBackend, mouseBackend, mouseBackendChanged)
IMPL_SETTER(const QString&, KmboxIp, kmboxIp, kmboxIpChanged)
IMPL_SETTER(const QString&, KmboxPort, kmboxPort, kmboxPortChanged)
IMPL_SETTER(const QString&, KmboxMac, kmboxMac, kmboxMacChanged)
IMPL_SETTER(bool, KmboxEncrypt, kmboxEncrypt, kmboxEncryptChanged)
IMPL_SETTER(bool, KmboxBezier, kmboxBezier, kmboxBezierChanged)
IMPL_SETTER(const QString&, MakcuSerial, makcuSerial, makcuSerialChanged)
IMPL_SETTER(const QString&, MakcuPort, makcuPort, makcuPortChanged)
IMPL_SETTER(bool, EnableVisualization, enableVisualization, enableVisualizationChanged)
IMPL_SETTER(bool, ShowInferLatency, showInferLatency, showInferLatencyChanged)
IMPL_SETTER(bool, DrawDetectionBoxes, drawDetectionBoxes, drawDetectionBoxesChanged)
IMPL_SETTER(bool, DrawAimOnPreview, drawAimOnPreview, drawAimOnPreviewChanged)
IMPL_SETTER(bool, DebugWindowEnabled, debugWindowEnabled, debugWindowEnabledChanged)
IMPL_SETTER(bool, AutoStart, autoStart, autoStartChanged)
IMPL_SETTER(bool, MinimizeToTray, minimizeToTray, minimizeToTrayChanged)
IMPL_SETTER(const QString&, Language, language, languageChanged)

#undef IMPL_SETTER

// 特殊处理: setTargetClassIds
void ConfigViewModel::setTargetClassIds(const QVariantList& v) {
    if (m_targetClassIds == v) return;
    m_targetClassIds = v;
    emit targetClassIdsChanged();
    emit configDirty();
}
