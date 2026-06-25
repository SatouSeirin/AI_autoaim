#include "settingspage.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QFrame>
#include <windows.h>

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(24);

    // ── 热键绑定 ──
    auto* hotkeyGroup = new QGroupBox("⌨ 热键绑定", this);
    setupHotkeys(hotkeyGroup);
    mainLayout->addWidget(hotkeyGroup);

    // Config Profiles
    auto* profileGroup = new QGroupBox("Config Profiles", this);
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
    auto* desc = new QLabel("Save or load configuration profiles (.json)", group);
    desc->setStyleSheet("font-size: 13px; color: #888888;");
    layout->addWidget(desc);
    auto* btnRow = new QHBoxLayout();
    btnSaveProfile_ = new QPushButton("Save Profile As...", group);
    btnSaveProfile_->setCursor(Qt::PointingHandCursor);
    btnSaveProfile_->setMinimumHeight(32);
    connect(btnSaveProfile_, &QPushButton::clicked, this, &SettingsPage::onSaveProfile);
    btnRow->addWidget(btnSaveProfile_);
    btnLoadProfile_ = new QPushButton("Load Profile...", group);
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
    chkAutoStart_->setStyleSheet("QCheckBox::indicator { width: 40px; height: 22px; }");
    row1->addWidget(lbl1);
    row1->addStretch();
    row1->addWidget(chkAutoStart_);
    layout->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    auto* lbl2 = new QLabel("最小化到托盘", group);
    lbl2->setStyleSheet("font-size: 14px; color: #333333;");
    chkMinimizeTray_ = new QCheckBox(group);
    chkMinimizeTray_->setChecked(true);
    chkMinimizeTray_->setStyleSheet("QCheckBox::indicator { width: 40px; height: 22px; }");
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
}

void SettingsPage::onSaveProfile() {
    QString path = QFileDialog::getSaveFileName(this, "Save Profile", "profile.json", "Config Files (*.json)");
    if (path.isEmpty()) return;
    if (engine_) {
        ConfigManager::Save(engine_->GetConfig(), path.toStdString());
    }
}

void SettingsPage::onLoadProfile() {
    QString path = QFileDialog::getOpenFileName(this, "Load Profile", "", "Config Files (*.json)");
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
void SettingsPage::startStopKeyChanged(int vk) {
    void* _a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&vk)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
void SettingsPage::aimKeyChanged(int vk) {
    void* _a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&vk)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
void SettingsPage::exitKeyChanged(int vk) {
    void* _a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&vk)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
void SettingsPage::switchKeyChanged(int vk) {
    void* _a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&vk)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

void SettingsPage::setSwitchKey(int vk)    { if (hkSwitch_)    hkSwitch_->setVkCode(vk); }
