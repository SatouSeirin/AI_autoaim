#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include "../../src/engine.hpp"
#include "configviewmodel.h"
#include "frameprovider.h"

// ============================================================
// EngineViewModel — AimEngine 的 QML 桥接
// 暴露引擎状态和控制方法给 QML
// ============================================================
class EngineViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool modelLoaded READ isModelLoaded NOTIFY modelLoadedChanged)
    Q_PROPERTY(QString currentModelName READ currentModelName NOTIFY currentModelNameChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(double fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(double inferMs READ inferMs NOTIFY inferMsChanged)
    Q_PROPERTY(qint64 frameCounter READ frameCounter NOTIFY frameReady)
    Q_PROPERTY(ConfigViewModel* config READ config CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QVariantList targetClassIds READ targetClassIds NOTIFY targetClassesChanged)
    Q_PROPERTY(QVariantList detections READ detections NOTIFY detectionsChanged)
    Q_PROPERTY(bool hotkeyCapturing READ isHotkeyCapturing WRITE setHotkeyCapturing NOTIFY hotkeyCapturingChanged)
    Q_PROPERTY(bool kmboxConnected READ isKmboxConnected NOTIFY kmboxConnectedChanged)
    Q_PROPERTY(bool modelLoading READ isModelLoading NOTIFY modelLoadingChanged)

public:
    explicit EngineViewModel(FrameProvider* frameProvider, QObject* parent = nullptr);
    ~EngineViewModel();

    // 状态读取
    bool isRunning() const;
    bool isModelLoaded() const;
    QString currentModelName() const;
    QString statusText() const;
    double fps() const;
    double inferMs() const;
    qint64 frameCounter() const;
    ConfigViewModel* config() const;
    QString lastError() const;

    // QML 可调用的方法
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggleRunning();
    Q_INVOKABLE void loadModel(const QString& path);
    Q_INVOKABLE void applyConfig();
    Q_INVOKABLE void resetConfig();
    Q_INVOKABLE bool saveProfile(const QString& filepath);
    Q_INVOKABLE bool loadProfile(const QString& filepath);
    Q_INVOKABLE void setTargetClass(int classId, bool enabled);
    Q_INVOKABLE bool isTargetClassEnabled(int classId) const;
    Q_INVOKABLE void testMove(const QString& direction);
    Q_INVOKABLE bool connectKMBox();
    Q_INVOKABLE void disconnectKMBox();
    bool isKmboxConnected() const;
    bool isModelLoading() const { return m_modelLoading; }

    // 新增属性读取
    QVariantList targetClassIds() const;
    QVariantList detections() const;

    // 热键捕获状态
    bool isHotkeyCapturing() const { return m_hotkeyCapturing; }
    void setHotkeyCapturing(bool v);

    // 引擎访问（供 HotkeyController 等使用）
    AimEngine* engine() { return m_engine.get(); }

signals:
    void runningChanged();
    void modelLoadedChanged();
    void currentModelNameChanged();
    void statusTextChanged();
    void fpsChanged();
    void inferMsChanged();
    void frameReady();
    void lastErrorChanged();
    void targetClassesChanged();
    void detectionsChanged();
    void hotkeyCapturingChanged();
    void kmboxConnectedChanged();
    void errorOccurred(const QString& message);
    void modelLoadStarted();
    void modelLoadFinished(bool success);
    void profileLoaded(bool success, const QString& message);
    void kmboxConnectionResult(bool success, const QString& message);
    void modelLoadingChanged();

private slots:
    void onConfigDirty();
    void onFpsTimer();

private:
    void setupFrameCallback();
    void syncConfigToEngine();

    std::unique_ptr<AimEngine> m_engine;
    ConfigViewModel* m_config;
    FrameProvider* m_frameProvider;
    QTimer* m_configDebounceTimer;
    QTimer* m_fpsTimer;
    QString m_currentModelName;
    QString m_lastError;
    bool m_hotkeyCapturing = false;
    bool m_modelLoading = false;
};
