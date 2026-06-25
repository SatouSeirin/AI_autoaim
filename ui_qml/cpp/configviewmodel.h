#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include "../../src/config.hpp"

// ============================================================
// ConfigViewModel — AimConfig 的 QML 桥接
// 每个字段包装为 Q_PROPERTY，支持双向绑定
// ============================================================
class ConfigViewModel : public QObject {
    Q_OBJECT

    // 模型
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(bool enableFp16 READ enableFp16 WRITE setEnableFp16 NOTIFY enableFp16Changed)
    Q_PROPERTY(double confThreshold READ confThreshold WRITE setConfThreshold NOTIFY confThresholdChanged)
    Q_PROPERTY(double nmsThreshold READ nmsThreshold WRITE setNmsThreshold NOTIFY nmsThresholdChanged)

    // 画面
    Q_PROPERTY(int captureSize READ captureSize WRITE setCaptureSize NOTIFY captureSizeChanged)

    // 瞄准
    Q_PROPERTY(QVariantList targetClassIds READ targetClassIds WRITE setTargetClassIds NOTIFY targetClassIdsChanged)
    Q_PROPERTY(double targetYRatio READ targetYRatio WRITE setTargetYRatio NOTIFY targetYRatioChanged)
    Q_PROPERTY(int targetBodyPart READ targetBodyPart WRITE setTargetBodyPart NOTIFY targetBodyPartChanged)
    Q_PROPERTY(int aimRangeSize READ aimRangeSize WRITE setAimRangeSize NOTIFY aimRangeSizeChanged)
    Q_PROPERTY(bool aimRangeCircle READ aimRangeCircle WRITE setAimRangeCircle NOTIFY aimRangeCircleChanged)

    // PID
    Q_PROPERTY(double kp READ kp WRITE setKp NOTIFY kpChanged)
    Q_PROPERTY(double ki READ ki WRITE setKi NOTIFY kiChanged)
    Q_PROPERTY(double kd READ kd WRITE setKd NOTIFY kdChanged)

    // 卡尔曼
    Q_PROPERTY(int predictionSteps READ predictionSteps WRITE setPredictionSteps NOTIFY predictionStepsChanged)

    // 灵敏度
    Q_PROPERTY(double sensitivity READ sensitivity WRITE setSensitivity NOTIFY sensitivityChanged)

    // 热键
    Q_PROPERTY(int aimKey READ aimKey WRITE setAimKey NOTIFY aimKeyChanged)
    Q_PROPERTY(int exitKey READ exitKey WRITE setExitKey NOTIFY exitKeyChanged)
    Q_PROPERTY(int switchTargetKey READ switchTargetKey WRITE setSwitchTargetKey NOTIFY switchTargetKeyChanged)
    Q_PROPERTY(int startStopKey READ startStopKey WRITE setStartStopKey NOTIFY startStopKeyChanged)

    // 鼠标后端 (0=SendInput, 1=IbInputSimulator, 2=KMBoxNet, 3=MaKcu)
    Q_PROPERTY(int mouseBackend READ mouseBackend WRITE setMouseBackend NOTIFY mouseBackendChanged)
    Q_PROPERTY(QString kmboxIp READ kmboxIp WRITE setKmboxIp NOTIFY kmboxIpChanged)
    Q_PROPERTY(QString kmboxPort READ kmboxPort WRITE setKmboxPort NOTIFY kmboxPortChanged)
    Q_PROPERTY(QString kmboxMac READ kmboxMac WRITE setKmboxMac NOTIFY kmboxMacChanged)
    Q_PROPERTY(bool kmboxEncrypt READ kmboxEncrypt WRITE setKmboxEncrypt NOTIFY kmboxEncryptChanged)
    Q_PROPERTY(bool kmboxBezier READ kmboxBezier WRITE setKmboxBezier NOTIFY kmboxBezierChanged)
    Q_PROPERTY(QString makcuSerial READ makcuSerial WRITE setMakcuSerial NOTIFY makcuSerialChanged)
    Q_PROPERTY(QString makcuPort READ makcuPort WRITE setMakcuPort NOTIFY makcuPortChanged)

    // 可视化
    Q_PROPERTY(bool enableVisualization READ enableVisualization WRITE setEnableVisualization NOTIFY enableVisualizationChanged)
    Q_PROPERTY(bool showInferLatency READ showInferLatency WRITE setShowInferLatency NOTIFY showInferLatencyChanged)
    Q_PROPERTY(bool drawDetectionBoxes READ drawDetectionBoxes WRITE setDrawDetectionBoxes NOTIFY drawDetectionBoxesChanged)
    Q_PROPERTY(bool drawAimOnPreview READ drawAimOnPreview WRITE setDrawAimOnPreview NOTIFY drawAimOnPreviewChanged)
    Q_PROPERTY(bool debugWindowEnabled READ debugWindowEnabled WRITE setDebugWindowEnabled NOTIFY debugWindowEnabledChanged)

    // 通用
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray NOTIFY minimizeToTrayChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit ConfigViewModel(QObject* parent = nullptr);

    // 与 AimConfig 互转
    AimConfig toAimConfig() const;
    void fromAimConfig(const AimConfig& cfg);

    // Getters
    QString modelPath() const;
    bool enableFp16() const;
    double confThreshold() const;
    double nmsThreshold() const;
    int captureSize() const;
    QVariantList targetClassIds() const;
    double targetYRatio() const;
    int targetBodyPart() const;
    int aimRangeSize() const;
    bool aimRangeCircle() const;
    double kp() const;
    double ki() const;
    double kd() const;
    int predictionSteps() const;
    double sensitivity() const;
    int aimKey() const;
    int exitKey() const;
    int switchTargetKey() const;
    int startStopKey() const;
    int mouseBackend() const;
    QString kmboxIp() const;
    QString kmboxPort() const;
    QString kmboxMac() const;
    bool kmboxEncrypt() const;
    bool kmboxBezier() const;
    QString makcuSerial() const;
    QString makcuPort() const;
    bool enableVisualization() const;
    bool showInferLatency() const;
    bool drawDetectionBoxes() const;
    bool drawAimOnPreview() const;
    bool debugWindowEnabled() const;
    bool autoStart() const;
    bool minimizeToTray() const;
    QString language() const;

    // Setters
    void setModelPath(const QString& v);
    void setEnableFp16(bool v);
    void setConfThreshold(double v);
    void setNmsThreshold(double v);
    void setCaptureSize(int v);
    void setTargetClassIds(const QVariantList& v);
    void setTargetYRatio(double v);
    void setTargetBodyPart(int v);
    void setAimRangeSize(int v);
    void setAimRangeCircle(bool v);
    void setKp(double v);
    void setKi(double v);
    void setKd(double v);
    void setPredictionSteps(int v);
    void setSensitivity(double v);
    void setAimKey(int v);
    void setExitKey(int v);
    void setSwitchTargetKey(int v);
    void setStartStopKey(int v);
    void setMouseBackend(int v);
    void setKmboxIp(const QString& v);
    void setKmboxPort(const QString& v);
    void setKmboxMac(const QString& v);
    void setKmboxEncrypt(bool v);
    void setKmboxBezier(bool v);
    void setMakcuSerial(const QString& v);
    void setMakcuPort(const QString& v);
    void setEnableVisualization(bool v);
    void setShowInferLatency(bool v);
    void setDrawDetectionBoxes(bool v);
    void setDrawAimOnPreview(bool v);
    void setDebugWindowEnabled(bool v);
    void setAutoStart(bool v);
    void setMinimizeToTray(bool v);
    void setLanguage(const QString& v);

signals:
    void modelPathChanged();
    void enableFp16Changed();
    void confThresholdChanged();
    void nmsThresholdChanged();
    void captureSizeChanged();
    void targetClassIdsChanged();
    void targetYRatioChanged();
    void targetBodyPartChanged();
    void aimRangeSizeChanged();
    void aimRangeCircleChanged();
    void kpChanged();
    void kiChanged();
    void kdChanged();
    void predictionStepsChanged();
    void sensitivityChanged();
    void aimKeyChanged();
    void exitKeyChanged();
    void switchTargetKeyChanged();
    void startStopKeyChanged();
    void mouseBackendChanged();
    void kmboxIpChanged();
    void kmboxPortChanged();
    void kmboxMacChanged();
    void kmboxEncryptChanged();
    void kmboxBezierChanged();
    void makcuSerialChanged();
    void makcuPortChanged();
    void enableVisualizationChanged();
    void showInferLatencyChanged();
    void drawDetectionBoxesChanged();
    void drawAimOnPreviewChanged();
    void debugWindowEnabledChanged();
    void autoStartChanged();
    void minimizeToTrayChanged();
    void languageChanged();
    void configDirty();  // 任意配置变更时发出，通知引擎同步

private:
    QString m_modelPath;
    bool m_enableFp16 = true;
    double m_confThreshold = 0.5;
    double m_nmsThreshold = 0.4;
    int m_captureSize = 640;
    QVariantList m_targetClassIds;
    double m_targetYRatio = 0.3;
    int m_targetBodyPart = 0;
    int m_aimRangeSize = 21;
    bool m_aimRangeCircle = true;
    double m_kp = 0.25;
    double m_ki = 0.0;
    double m_kd = 0.08;
    int m_predictionSteps = 3;
    double m_sensitivity = 0.15;
    int m_aimKey = 0x02;
    int m_exitKey = 0x71;
    int m_switchTargetKey = 0x06;
    int m_startStopKey = 0x74;
    int m_mouseBackend = 0;
    QString m_kmboxIp = "192.168.2.188";
    QString m_kmboxPort = "8888";
    QString m_kmboxMac = "01FBC068";
    bool m_kmboxEncrypt = true;
    bool m_kmboxBezier = true;
    QString m_makcuSerial = "COM1";
    QString m_makcuPort = "8888";
    bool m_enableVisualization = true;
    bool m_showInferLatency = true;
    bool m_drawDetectionBoxes = true;
    bool m_drawAimOnPreview = true;
    bool m_debugWindowEnabled = false;
    bool m_autoStart = false;
    bool m_minimizeToTray = true;
    QString m_language = "zh-CN";
};
