#include "settingspage.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QFrame>
 #include <QLineEdit>
 #include <QStackedWidget>
#include <windows.h>
struct WheelBlocker : QObject {
    using QObject::QObject;
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Wheel) return true;
        return QObject::eventFilter(obj, event);
    }
};

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent) {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    outerLayout->addWidget(scrollArea);

    auto* container = new QWidget();
    scrollArea->setWidget(container);

    auto* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(24);

    // ── 热键绑定 ──
    auto* hotkeyGroup = new QGroupBox("⌨ 热键绑定", this);
    setupHotkeys(hotkeyGroup);
    mainLayout->addWidget(hotkeyGroup);
 
     // ── 输入模式 ──
     auto* inputGroup = new QGroupBox("🖥 输入模式 / 鼠标后端", this);
     setupInputMode(inputGroup);
     mainLayout->addWidget(inputGroup);

    // 配置文件
    auto* profileGroup = new QGroupBox("配置文件", this);
    setupProfiles(profileGroup);
    mainLayout->addWidget(profileGroup);

    // ── 通用设置 ──
    auto* generalGroup = new QGroupBox("⚙ 通用设置", this);
    setupGeneral(generalGroup);
    mainLayout->addWidget(generalGroup);

    // ── 检查更新 ──
    auto* updateGroup = new QGroupBox("🔄 检查更新", this);
    setupUpdate(updateGroup);
    mainLayout->addWidget(updateGroup);

    // ── 关于 ──
    auto* aboutGroup = new QGroupBox("ℹ 关于", this);
    setupAbout(aboutGroup);
    mainLayout->addWidget(aboutGroup);

    mainLayout->addStretch();
}

void SettingsPage::setupHotkeys(QGroupBox* group) {
    auto* layout = new QFormLayout(group);
    layout->setSpacing(14);
    layout->setContentsMargins(16, 20, 16, 16);

    auto makeLabel = [](const QString& text) {
        auto* lbl = new QLabel(text);
        lbl->setStyleSheet("font-size: 14px; color: #333333; font-weight: bold;");
        return lbl;
    };

    auto makeDesc = [](const QString& text) {
        auto* lbl = new QLabel(text);
        lbl->setStyleSheet("font-size: 12px; color: #999999; margin-left: 8px;");
        return lbl;
    };

    // 开始/停止推理
    auto* row1 = new QHBoxLayout();
    hkStartStop_ = new HotkeyCapture(VK_F5, group);
    row1->addWidget(hkStartStop_);
    row1->addWidget(makeDesc("全局开始 / 停止推理，无需切回窗口"));
    row1->addStretch();
    layout->addRow(makeLabel("开始/停止"), row1);
    connect(hkStartStop_, &HotkeyCapture::keyChanged, this, &SettingsPage::startStopKeyChanged);

    // 瞄准键
    auto* row2 = new QHBoxLayout();
    hkAim_ = new HotkeyCapture(VK_RBUTTON, group);
    row2->addWidget(hkAim_);
    row2->addWidget(makeDesc("按住此键时自动瞄准目标"));
    row2->addStretch();
    layout->addRow(makeLabel("瞄准键"), row2);
    connect(hkAim_, &HotkeyCapture::keyChanged, this, &SettingsPage::aimKeyChanged);

    // 退出键
    auto* row3 = new QHBoxLayout();
    hkExit_ = new HotkeyCapture(VK_F2, group);
    row3->addWidget(hkExit_);
    row3->addWidget(makeDesc("推理循环中按下此键退出"));
    row3->addStretch();
    layout->addRow(makeLabel("退出键"), row3);
    connect(hkExit_, &HotkeyCapture::keyChanged, this, &SettingsPage::exitKeyChanged);

    // 切换目标键
    auto* row4 = new QHBoxLayout();
    hkSwitch_ = new HotkeyCapture(VK_XBUTTON1, group);
    row4->addWidget(hkSwitch_);
    row4->addWidget(makeDesc("按下切换瞄准目标"));
    row4->addStretch();
    layout->addRow(makeLabel("切换目标"), row4);
    connect(hkSwitch_, &HotkeyCapture::keyChanged, this, &SettingsPage::switchKeyChanged);
}

void SettingsPage::setupProfiles(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);
    layout->setSpacing(10);
    layout->setContentsMargins(16, 16, 16, 16);
    auto* desc = new QLabel("保存或加载配置文件 (.json)", group);
    desc->setStyleSheet("font-size: 13px; color: #888888;");
    layout->addWidget(desc);
    auto* btnRow = new QHBoxLayout();
    btnSaveProfile_ = new QPushButton("保存配置文件...", group);
    btnSaveProfile_->setCursor(Qt::PointingHandCursor);
    btnSaveProfile_->setMinimumHeight(32);
    connect(btnSaveProfile_, &QPushButton::clicked, this, &SettingsPage::onSaveProfile);
    btnRow->addWidget(btnSaveProfile_);
    btnLoadProfile_ = new QPushButton("加载配置文件...", group);
    btnLoadProfile_->setCursor(Qt::PointingHandCursor);
    btnLoadProfile_->setMinimumHeight(32);
    connect(btnLoadProfile_, &QPushButton::clicked, this, &SettingsPage::onLoadProfile);
    btnRow->addWidget(btnLoadProfile_);
    btnRow->addStretch();
    layout->addLayout(btnRow);
}

void SettingsPage::setupGeneral(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);

    auto* row1 = new QHBoxLayout();
    auto* lbl1 = new QLabel("开机自启", group);
    lbl1->setStyleSheet("font-size: 14px; color: #333333;");
    chkAutoStart_ = new QCheckBox(group);
    chkAutoStart_->setChecked(false);
    chkAutoStart_->style()->polish(chkAutoStart_);
    row1->addWidget(lbl1);
    row1->addStretch();
    row1->addWidget(chkAutoStart_);
    layout->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    auto* lbl2 = new QLabel("最小化到托盘", group);
    lbl2->setStyleSheet("font-size: 14px; color: #333333;");
    chkMinimizeTray_ = new QCheckBox(group);
    chkMinimizeTray_->setChecked(true);
    chkMinimizeTray_->style()->polish(chkMinimizeTray_);
    row2->addWidget(lbl2);
    row2->addStretch();
    row2->addWidget(chkMinimizeTray_);
    layout->addLayout(row2);

    auto* row3 = new QHBoxLayout();
    auto* lbl3 = new QLabel("语言", group);
    lbl3->setStyleSheet("font-size: 14px; color: #333333;");
    comboLanguage_ = new QComboBox(group);
    comboLanguage_->addItem("简体中文");
    comboLanguage_->addItem("English");
    comboLanguage_->setCurrentIndex(0);
    comboLanguage_->installEventFilter(new WheelBlocker(comboLanguage_));
    comboLanguage_->setFixedWidth(140);
    comboLanguage_->setStyleSheet(
        "QComboBox { border: 1px solid #D9D9D9; border-radius: 4px;"
        " padding: 4px 8px; font-size: 14px; color: #333333; }");
    row3->addWidget(lbl3);
    row3->addStretch();
    row3->addWidget(comboLanguage_);
    layout->addLayout(row3);
}

void SettingsPage::setupUpdate(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);

    versionLabel_ = new QLabel("当前版本: v2.1.0", group);
    versionLabel_->setStyleSheet("font-size: 14px; color: #888888;");
    layout->addWidget(versionLabel_);

    auto* checkBtn = new QPushButton(" 检查更新 ", group);
    checkBtn->setObjectName("btnPrimary");
    checkBtn->setCursor(Qt::PointingHandCursor);
    checkBtn->setFixedWidth(120);
    checkBtn->setMinimumHeight(32);
    layout->addWidget(checkBtn);
}

void SettingsPage::setupAbout(QGroupBox* group) {
    auto* layout = new QVBoxLayout(group);

    auto* title = new QLabel("Chimera AI v2.1", group);
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #1890FF;");
    layout->addWidget(title);

    auto* desc = new QLabel("Built with Qt6 + TensorRT C++ API + CUDA\n"
                            "支持 .onnx / .engine 双格式\n"
                            "YOLOv8+ ~ v26 通用目标检测", group);
    desc->setStyleSheet("font-size: 13px; color: #888888; line-height: 1.6;");
    layout->addWidget(desc);
}


void SettingsPage::syncFromConfig() {
    if (!engine_) return;
    auto cfg = engine_->GetConfig();
    setStartStopKey(cfg.start_stop_key);
    setAimKey(cfg.aim_key);
    setExitKey(cfg.exit_key);
    setSwitchKey(cfg.switch_target_key);
    // 同步输入模式
    if (comboInputMode_) {
        int idx = static_cast<int>(cfg.mouse_backend);
        if (idx < 0 || idx >= comboInputMode_->count()) idx = 0;
        comboInputMode_->setCurrentIndex(idx);
        if (idx == 2) inputConfigStack_->setCurrentIndex(1);
        else if (idx == 3) inputConfigStack_->setCurrentIndex(2);
        else inputConfigStack_->setCurrentIndex(0);
        if (kmboxIpEdit_) kmboxIpEdit_->setText(QString::fromStdString(cfg.kmbox_ip));
        if (kmboxPortEdit_) kmboxPortEdit_->setText(QString::fromStdString(cfg.kmbox_port));
        if (kmboxUuidEdit_) kmboxUuidEdit_->setText(QString::fromStdString(cfg.kmbox_mac));
        if (makcuSerialEdit_) makcuSerialEdit_->setText(QString::fromStdString(cfg.makcu_serial));
        if (makcuPortEdit_) makcuPortEdit_->setText(QString::fromStdString(cfg.makcu_port));
    }
}

void SettingsPage::onSaveProfile() {
    QString path = QFileDialog::getSaveFileName(this, "保存配置", "profile.json", "配置文件 (*.json)");
    if (path.isEmpty()) return;
    if (engine_) {
        ConfigManager::Save(engine_->GetConfig(), path.toStdString());
    }
}

void SettingsPage::onLoadProfile() {
    QString path = QFileDialog::getOpenFileName(this, "加载配置", "", "配置文件 (*.json)");
    if (path.isEmpty()) return;
    if (engine_) {
        AimConfig cfg = ConfigManager::Load(path.toStdString());
        engine_->UpdateConfig(cfg);
        emit loadProfileRequested(path);
    }
}

// ── 热键值访问器 ──
int SettingsPage::startStopKey() const { return hkStartStop_ ? hkStartStop_->vkCode() : VK_F5; }
int SettingsPage::aimKey()       const { return hkAim_       ? hkAim_->vkCode()       : VK_RBUTTON; }
int SettingsPage::exitKey()      const { return hkExit_      ? hkExit_->vkCode()      : VK_F2; }
int SettingsPage::switchKey()    const { return hkSwitch_    ? hkSwitch_->vkCode()    : VK_XBUTTON1; }

void SettingsPage::setStartStopKey(int vk) { if (hkStartStop_) hkStartStop_->setVkCode(vk); }
void SettingsPage::setAimKey(int vk)       { if (hkAim_)       hkAim_->setVkCode(vk); }
void SettingsPage::setExitKey(int vk)      { if (hkExit_)      hkExit_->setVkCode(vk); }

// Manual signal implementations (Qt6 MOC workaround)

void SettingsPage::setSwitchKey(int vk)    { if (hkSwitch_)    hkSwitch_->setVkCode(vk); }
 
 // ============================================================
 // 输入模式 / 鼠标后端
 // ============================================================
 void SettingsPage::setupInputMode(QGroupBox* group) {
     auto* layout = new QVBoxLayout(group);
     layout->setSpacing(10);
     layout->setContentsMargins(16, 16, 16, 16);
 
     auto* topRow = new QHBoxLayout();
     auto* lbl = new QLabel("后端类型", group);
     lbl->setStyleSheet("font-size: 14px; color: #333333; font-weight: bold;");
     topRow->addWidget(lbl);
 
     comboInputMode_ = new QComboBox(group);
     comboInputMode_->addItems({"SendInput", "IbInputSimulator", "KMBoxNet", "MaKcu"});
     comboInputMode_->setMinimumWidth(180);
    comboInputMode_->installEventFilter(new WheelBlocker(comboInputMode_));
     topRow->addWidget(comboInputMode_);
     topRow->addStretch();
     layout->addLayout(topRow);
 
     // Stacked widget: 0=empty, 1=KMBoxNet, 2=MaKcu
     inputConfigStack_ = new QStackedWidget(group);
 
     // Page 0: 空面板（SendInput / IbInputSimulator）
     auto* emptyPage = new QWidget(group);
     auto* emptyLabel = new QLabel("无需额外配置，选择后直接生效", emptyPage);
     emptyLabel->setStyleSheet("font-size: 13px; color: #999999; padding: 16px 0;");
     auto* emptyLayout = new QVBoxLayout(emptyPage);
     emptyLayout->addWidget(emptyLabel);
     emptyLayout->addStretch();
         auto* backendStatusLabel = new QLabel("当前后端: 未连接", emptyPage);
    backendStatusLabel->setObjectName("backendStatusLabel");
    backendStatusLabel->setStyleSheet("font-size: 13px; color: #64748B; padding: 8px 0;");
    emptyLayout->addWidget(backendStatusLabel);
inputConfigStack_->addWidget(emptyPage);  // index 0
 
     // Page 1: KMBoxNet 配置
     auto* kmboxPage = new QWidget(group);
     auto* kmboxLayout = new QVBoxLayout(kmboxPage);
     kmboxLayout->setSpacing(8);
 
     auto makeRow = [](const QString& label, QLineEdit*& edit, const QString& placeholder, QWidget* parent) -> QHBoxLayout* {
         auto* row = new QHBoxLayout();
         auto* lbl = new QLabel(label, parent);
         lbl->setFixedWidth(60);
         lbl->setStyleSheet("font-size: 13px; color: #555555;");
         edit = new QLineEdit(parent);
         edit->setPlaceholderText(placeholder);
         edit->setMinimumWidth(200);
         edit->setStyleSheet(
             "QLineEdit { border: 1px solid #D9D9D9; border-radius: 4px;"
             " padding: 4px 8px; font-size: 13px; color: #333333; background: #FFFFFF; }"
             "QLineEdit:focus { border-color: #3B82F6; }");
         row->addWidget(lbl);
         row->addWidget(edit);
         row->addStretch();
         return row;
     };
 
     kmboxLayout->addLayout(makeRow("IP:", kmboxIpEdit_, "192.168.2.188", kmboxPage));
     kmboxLayout->addLayout(makeRow("Port:", kmboxPortEdit_, "8888", kmboxPage));
     kmboxLayout->addLayout(makeRow("UUID:", kmboxUuidEdit_, "01FBC068", kmboxPage));
 
     auto* kmboxBtnRow = new QHBoxLayout();
     btnKmboxConnect_ = new QPushButton("连接 KMBoxNet", kmboxPage);
     btnKmboxConnect_->setObjectName("btnPrimary");
     btnKmboxConnect_->setCursor(Qt::PointingHandCursor);
     btnKmboxConnect_->setFixedWidth(140);
     connect(btnKmboxConnect_, &QPushButton::clicked, this, &SettingsPage::onKmboxConnect);
     kmboxBtnRow->addWidget(btnKmboxConnect_);
     kmboxBtnRow->addStretch();
     kmboxLayout->addLayout(kmboxBtnRow);
     inputConfigStack_->addWidget(kmboxPage);  // index 1
 
     // Page 2: MaKcu 配置
     auto* makcuPage = new QWidget(group);
     auto* makcuLayout = new QVBoxLayout(makcuPage);
     makcuLayout->setSpacing(8);
 
     makcuLayout->addLayout(makeRow("串口:", makcuSerialEdit_, "COM1", makcuPage));
     makcuLayout->addLayout(makeRow("比特率:", makcuPortEdit_, "115200", makcuPage));
 
     auto* makcuBtnRow = new QHBoxLayout();
     btnMakcuConnect_ = new QPushButton("连接 MaKcu", makcuPage);
     btnMakcuConnect_->setObjectName("btnPrimary");
     btnMakcuConnect_->setCursor(Qt::PointingHandCursor);
     btnMakcuConnect_->setFixedWidth(140);
     connect(btnMakcuConnect_, &QPushButton::clicked, this, &SettingsPage::onMakcuConnect);
     makcuBtnRow->addWidget(btnMakcuConnect_);
     makcuBtnRow->addStretch();
     makcuLayout->addLayout(makcuBtnRow);
     inputConfigStack_->addWidget(makcuPage);  // index 2
 
     layout->addWidget(inputConfigStack_);
 
     connect(comboInputMode_, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &SettingsPage::onInputModeChanged);
 }
 
 void SettingsPage::onInputModeChanged(int index) {
     if (!engine_) return;
     auto cfg = engine_->GetConfig();
     cfg.mouse_backend = static_cast<MouseBackend>(index);
 
     // 保存当前填写的连接参数
     cfg.kmbox_ip   = kmboxIpEdit_->text().toStdString();
     cfg.kmbox_port = kmboxPortEdit_->text().toStdString();
     cfg.kmbox_mac  = kmboxUuidEdit_->text().toStdString();
     cfg.makcu_serial = makcuSerialEdit_->text().toStdString();
     cfg.makcu_port   = makcuPortEdit_->text().toStdString();
 
     engine_->UpdateConfig(cfg);
 
     // 切换堆叠页面
     if (index == 2) inputConfigStack_->setCurrentIndex(1);
     else if (index == 3) inputConfigStack_->setCurrentIndex(2);
     else inputConfigStack_->setCurrentIndex(0);
     // ????????
    // ????? emptyPage ?
    QLabel* statusLabel = findChild<QLabel*>("backendStatusLabel");
    if (statusLabel) {
        switch (index) {
            case 0: statusLabel->setText("当前后端: SendInput (已就绪)"); break;
            case 1: {
                bool ibOk = false;
                if (engine_) {
                    auto cfg = engine_->GetConfig();
                    cfg.mouse_backend = MouseBackend::IbInputSimulator;
                    engine_->UpdateConfig(cfg);
                    ibOk = true;
                }
                statusLabel->setText(ibOk ?
                    "当前后端: IbInputSimulator (已加载)" :
                    "当前后端: IbInputSimulator (加载失败)");
                break;
            }
            default: break;
        }
    }

}
 
 void SettingsPage::onKmboxConnect() {
     if (!engine_) return;
     auto cfg = engine_->GetConfig();
     cfg.kmbox_ip   = kmboxIpEdit_->text().toStdString();
     cfg.kmbox_port = kmboxPortEdit_->text().toStdString();
     cfg.kmbox_mac  = kmboxUuidEdit_->text().toStdString();
     cfg.mouse_backend = MouseBackend::KMBoxNet;
     engine_->UpdateConfig(cfg);
     comboInputMode_->setCurrentIndex(2);
     std::cout << "[KMBoxNet] Connecting to " << cfg.kmbox_ip << ":" << cfg.kmbox_port << std::endl;
 }
 
 void SettingsPage::onMakcuConnect() {
     if (!engine_) return;
     auto cfg = engine_->GetConfig();
     cfg.makcu_serial = makcuSerialEdit_->text().toStdString();
     cfg.makcu_port   = makcuPortEdit_->text().toStdString();
     cfg.mouse_backend = MouseBackend::MaKcu;
     engine_->UpdateConfig(cfg);
     comboInputMode_->setCurrentIndex(3);
     std::cout << "[MaKcu] Connecting on " << cfg.makcu_serial << ":" << cfg.makcu_port << std::endl;
 }
