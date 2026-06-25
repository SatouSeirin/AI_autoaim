#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <memory>

#include "pages/modelpage.h"
#include "pages/previewpage.h"
#include "pages/aimpage.h"
#include "pages/settingspage.h"
#include "../src/hotkey_manager.hpp"

class AimEngine;
struct AimConfig;

// ============================================================
// MainWindow — v2.1
// 左侧导航栏 + 右侧 QStackedWidget (4 页) + 顶部信息栏
// 配置持久化 + 帧回调 + 全局热键
// ============================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& modelArg = "", QWidget* parent = nullptr);
    ~MainWindow();

protected:
    // 接收 Windows WM_HOTKEY 消息
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void initUI();
    void initEngine(const QString& modelArg);
    void setupFrameCallback();
    void setupHotkeys();
    void saveConfigOnExit();
    void syncConfigFromUI();
    QWidget* createNavBar();
    QPushButton* createNavItem(const QString& text, const QString& icon, int index);
    void onNavChanged(int index);

    // 引擎
    std::unique_ptr<AimEngine> engine_;

    // 全局热键管理器
    std::unique_ptr<HotkeyManager> hotkeyManager_;

    // UI
    QStackedWidget* stackedWidget_ = nullptr;
    QButtonGroup*   navGroup_      = nullptr;

    ModelPage*    modelPage_    = nullptr;
    PreviewPage*  previewPage_  = nullptr;
    AimPage*      aimPage_      = nullptr;
    SettingsPage* settingsPage_ = nullptr;

    QLabel* topSysStatus_   = nullptr;
    QLabel* topModelStatus_ = nullptr;
    QLabel* topCardInfo_    = nullptr;

    QList<QPushButton*> navButtons_;
};
