#include "mainwindow.h"
#include "../src/engine.hpp"
#include "../src/config.hpp"
#include "../src/config_manager.hpp"
#include "../src/hotkey_manager.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QFile>
#include <QStyle>
#include <QTimer>
#include <QFileInfo>
#include <QMetaObject>
#include <QAbstractNativeEventFilter>
#include <windows.h>

MainWindow::MainWindow(const QString& modelArg, QWidget* parent)
    : QMainWindow(parent) {

    initEngine(modelArg);
    initUI();

    // QSS
    QFile qssFile(":/style.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    }

    // 引擎 → 页面关联
    modelPage_->setEngine(engine_.get());
    previewPage_->setEngine(engine_.get());
    aimPage_->setEngine(engine_.get());
    settingsPage_->setEngine(engine_.get());

    // 帧回调：推理线程 → UI 线程
    setupFrameCallback();

    // FPS timer (updates preview page)
    QTimer* fpsTimer = new QTimer(this);
    connect(fpsTimer, &QTimer::timeout, this, [this]() {
        if (engine_ && previewPage_) {
            double fps = engine_->GetFPS();
            double ms = engine_->GetInferMs();
            previewPage_->setFpsInfo(fps, ms);
        }
    });
    fpsTimer->start(500);

    // 全局热键
    setupHotkeys();

    setWindowTitle("MiniSense v2.1");
    setMinimumSize(900, 600);
    resize(1100, 750);;
}

MainWindow::~MainWindow() {
    if (engine_ && engine_->IsRunning()) {
        engine_->Stop();
    }
    hotkeyManager_->UnregisterAll();
    saveConfigOnExit();
}

// ============================================================
// Windows 消息处理 — WM_HOTKEY
// ============================================================
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (hotkeyManager_ && hotkeyManager_->HandleMessage(msg)) {
            *result = 0;
            return true;
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

// ============================================================
// 初始化引擎（加载配置持久化 + 可选命令行模型）
// ============================================================
void MainWindow::initEngine(const QString& modelArg) {
    engine_ = std::make_unique<AimEngine>();

    // 从 JSON 恢复配置
    AimConfig cfg = ConfigManager::Load("config.json");
    engine_->SetConfig(cfg);

    // 命令行模型参数覆盖 JSON
    if (!modelArg.isEmpty()) {
        cfg.model_path = modelArg.toStdString();
        engine_->SetConfig(cfg);
    }
}

// ============================================================
// 全局热键设置
// ============================================================
void MainWindow::setupHotkeys() {
    hotkeyManager_ = std::make_unique<HotkeyManager>();

    auto cfg = engine_->GetConfig();

    // F5: 开始/停止推理（全局热键，无需切回窗口）
    int startStopId = 1;
    hotkeyManager_->Register(startStopId, MOD_NOREPEAT, cfg.start_stop_key, [this](int) {
        // UI 线程安全调用
        QMetaObject::invokeMethod(this, [this]() {
            if (modelPage_) {
                // 模拟点击开始/停止按钮
                modelPage_->onStartStop();
            }
        }, Qt::QueuedConnection);
    });
    // F2: exit key (global)
    int exitId = 2;
    hotkeyManager_->Register(exitId, MOD_NOREPEAT, cfg.exit_key, [this](int) {
        QMetaObject::invokeMethod(this, [this]() {
            if (engine_) {
                std::cout << "[Hotkey] Exit key pressed, stopping engine..." << std::endl;
                engine_->Stop();
            }
        }, Qt::QueuedConnection);
    });

    // 从 SettingsPage 同步初始热键值
    auto* sp = settingsPage_;
    if (sp) {
        sp->setStartStopKey(cfg.start_stop_key);
        sp->setAimKey(cfg.aim_key);
        sp->setExitKey(cfg.exit_key);
        sp->setSwitchKey(cfg.switch_target_key);
    }

    // 热键变更 → 引擎
    connect(sp, &SettingsPage::startStopKeyChanged, this, [this](int vk) {
        hotkeyManager_->Unregister(1);
        hotkeyManager_->Register(1, MOD_NOREPEAT, vk, [this](int) {
            QMetaObject::invokeMethod(this, [this]() {
                if (modelPage_) modelPage_->onStartStop();
            }, Qt::QueuedConnection);
        });
    });


    connect(sp, &SettingsPage::aimKeyChanged, this, [this](int) {
        syncConfigFromUI();
    });

    connect(sp, &SettingsPage::exitKeyChanged, this, [this](int vk) {
        hotkeyManager_->Unregister(2);
        hotkeyManager_->Register(2, MOD_NOREPEAT, vk, [this](int) {
            QMetaObject::invokeMethod(this, [this]() {
                if (engine_) engine_->Stop();
            }, Qt::QueuedConnection);
        });
        syncConfigFromUI();
    });

    connect(sp, &SettingsPage::switchKeyChanged, this, [this](int) {
        syncConfigFromUI();
    });
}

// ============================================================
// UI 更改 → 引擎配置
// ============================================================
void MainWindow::syncConfigFromUI() {
    if (!engine_ || !settingsPage_) return;
    auto cfg = engine_->GetConfig();
    cfg.aim_key           = settingsPage_->aimKey();
    cfg.exit_key          = settingsPage_->exitKey();
    cfg.switch_target_key = settingsPage_->switchKey();
    cfg.start_stop_key    = settingsPage_->startStopKey();
    engine_->UpdateConfig(cfg);
}

// ============================================================
// 帧回调：推理线程安全通知 UI
// ============================================================
void MainWindow::setupFrameCallback() {
    engine_->SetFrameCallback([this](const cv::Mat& frame) {
        if (frame.empty()) return;
        // BGRA → BGR → QImage（修复 4通道/3通道 不匹配）
        cv::Mat bgr;
        if (frame.channels() == 4) {
            cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
        } else {
            bgr = frame;
        }
        QImage qimg(bgr.data, bgr.cols, bgr.rows,
                    static_cast<int>(bgr.step), QImage::Format_BGR888);
        QImage copy = qimg.copy();  // 深拷贝，脱离 cv::Mat 生命周期
        QMetaObject::invokeMethod(this, [this, copy]() {
            previewPage_->updateFrame(copy);
        }, Qt::QueuedConnection);
    });
}

// ============================================================
// 保存配置到 JSON
// ============================================================
void MainWindow::saveConfigOnExit() {
    if (engine_) {
        // 确保热键配置已同步
        syncConfigFromUI();
        ConfigManager::Save(engine_->GetConfig(), "config.json");
    }
}

// ============================================================
// UI 布局
// ============================================================
void MainWindow::initUI() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainHBox = new QHBoxLayout(centralWidget);
    mainHBox->setContentsMargins(0, 0, 0, 0);
    mainHBox->setSpacing(0);

    mainHBox->addWidget(createNavBar());

    // 右侧
    auto* rightArea = new QWidget(centralWidget);
    auto* rightLayout = new QVBoxLayout(rightArea);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // 顶部信息栏
    auto* topBar = new QWidget(rightArea);
    topBar->setObjectName("topInfoBar");
    topBar->setFixedHeight(36);
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(20, 0, 20, 0);

    topSysStatus_ = new QLabel("🔵 系统就绪", topBar);
    topSysStatus_->setObjectName("statusRunning");
    topLayout->addWidget(topSysStatus_);

    topModelStatus_ = new QLabel("⬜ 尚未导入模型", topBar);
    topModelStatus_->setObjectName("statusNoModel");
    topLayout->addWidget(topModelStatus_);

    topLayout->addStretch();
    rightLayout->addWidget(topBar);

    // QStackedWidget
    stackedWidget_ = new QStackedWidget(rightArea);

    modelPage_    = new ModelPage(stackedWidget_);
    previewPage_  = new PreviewPage(stackedWidget_);
    aimPage_      = new AimPage(stackedWidget_);
    settingsPage_ = new SettingsPage(stackedWidget_);

    stackedWidget_->addWidget(modelPage_);     // 0
    stackedWidget_->addWidget(previewPage_);   // 1
    stackedWidget_->addWidget(aimPage_);       // 2
    stackedWidget_->addWidget(settingsPage_);  // 3

    rightLayout->addWidget(stackedWidget_, 1);
    mainHBox->addWidget(rightArea, 1);

    stackedWidget_->setCurrentIndex(0);

    // 模型加载事件 → 顶部状态栏
    connect(modelPage_, &ModelPage::modelLoaded, this, [this](const QString& path, bool ok) {
        if (ok) {
            QFileInfo fi(path);
            topModelStatus_->setText(QString("✅ %1").arg(fi.fileName()));
            topModelStatus_->setObjectName("statusRunning");
            topModelStatus_->setStyleSheet(topModelStatus_->styleSheet());
        }
    });

    connect(modelPage_, &ModelPage::engineStarted, this, [this]() {
        topSysStatus_->setText("🔵 推理运行中");
    });

    connect(modelPage_, &ModelPage::engineStopped, this, [this]() {
        topSysStatus_->setText("🔵 系统就绪");
    });
}

// ============================================================
// 左侧导航栏
// ============================================================
QWidget* MainWindow::createNavBar() {
    auto* navBar = new QWidget(this);
    navBar->setObjectName("navBar");
    auto* navLayout = new QVBoxLayout(navBar);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    auto* brandLabel = new QLabel("MiniSense", navBar);
    brandLabel->setObjectName("navBrand");
    brandLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    navLayout->addWidget(brandLabel);
    navLayout->addSpacing(20);

    navGroup_ = new QButtonGroup(this);
    navGroup_->setExclusive(true);

    navButtons_ << createNavItem("⚙  系统设置",  "settings", 3);
    navButtons_ << createNavItem("🖥  画面源预览", "preview", 1);
    navButtons_ << createNavItem("🎯  瞄准参数",  "aim",     2);
    navButtons_ << createNavItem("☁  模型导入",  "model",   0);

    for (auto* btn : navButtons_) {
        navLayout->addWidget(btn);
    }
    navLayout->addStretch();

    auto* versionLabel = new QLabel("v2.1", navBar);
    // QSS styled
    versionLabel->setAlignment(Qt::AlignCenter);
    navLayout->addWidget(versionLabel);



#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(navGroup_, &QButtonGroup::idClicked, this, &MainWindow::onNavChanged);
#else
    connect(navGroup_, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::onNavChanged);
#endif

    QTimer::singleShot(0, this, [this]() {
        navButtons_[3]->setChecked(true);
        stackedWidget_->setCurrentIndex(0);
        for (int i = 0; i < navButtons_.size(); i++) {
            bool active = (i == 3);
            navButtons_[i]->setProperty("active", active);
            navButtons_[i]->style()->unpolish(navButtons_[i]);
            navButtons_[i]->style()->polish(navButtons_[i]);
        }
    });

    return navBar;
}

QPushButton* MainWindow::createNavItem(const QString& text, const QString&, int index) {
    auto* btn = new QPushButton(text, this);
    btn->setObjectName("navItem");
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(44);
    navGroup_->addButton(btn, index);
    return btn;
}

void MainWindow::onNavChanged(int index) {
    for (auto* btn : navButtons_) {
        bool active = (navGroup_->id(btn) == index);
        btn->setProperty("active", active);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
    stackedWidget_->setCurrentIndex(index);
}
