#pragma once

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>

#include "../widgets/hotkeycapture.h"
#include "../../src/engine.hpp"
#include "../../src/config_manager.hpp"

// ============================================================
// SettingsPage — v2.1 系统设置页
// 通用设置 + 热键绑定 + 检查更新 + 关于
// ============================================================

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);
    void syncFromConfig();

    // 获取/设置热键值（供 MainWindow 同步）
    int startStopKey() const;
    int aimKey()       const;
    int exitKey()      const;
    int switchKey()    const;
    void setStartStopKey(int vk);
    void setAimKey(int vk);
    void setExitKey(int vk);
    void setSwitchKey(int vk);

signals:
    void saveProfileRequested(const QString& path);
    void loadProfileRequested(const QString& path);

public slots:
    void setEngine(AimEngine* engine) { engine_ = engine; }
    void onSaveProfile();
    void onLoadProfile();
    void startStopKeyChanged(int vk);
    void aimKeyChanged(int vk);
    void exitKeyChanged(int vk);
    void switchKeyChanged(int vk);

private:
    void setupGeneral(QGroupBox* group);
    void setupHotkeys(QGroupBox* group);
    void setupProfiles(QGroupBox* group);
    void setupUpdate(QGroupBox* group);
    void setupAbout(QGroupBox* group);

    QCheckBox*    chkAutoStart_    = nullptr;
    QCheckBox*    chkMinimizeTray_ = nullptr;
    QComboBox*    comboLanguage_   = nullptr;
    QLabel*       versionLabel_    = nullptr;

    HotkeyCapture* hkStartStop_    = nullptr;
    HotkeyCapture* hkAim_          = nullptr;
    HotkeyCapture* hkExit_         = nullptr;
    HotkeyCapture* hkSwitch_       = nullptr;

    QPushButton* btnSaveProfile_   = nullptr;
    QPushButton* btnLoadProfile_   = nullptr;

    AimEngine* engine_ = nullptr;
};
