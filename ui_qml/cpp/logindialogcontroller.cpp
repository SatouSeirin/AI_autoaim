#include "logindialogcontroller.h"

LoginDialogController::LoginDialogController(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("MiniSense - 登录"));
    setFixedSize(360, 220);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 深色样式
    setStyleSheet(
        "QDialog { background: #1A1A2E; }"
        "QLabel { color: #E8E8F0; font-size: 14px; }"
        "QLineEdit {"
        "  background: #252540; color: #E8E8F0; border: 1px solid #2A2A45;"
        "  border-radius: 6px; padding: 8px 12px; font-size: 14px;"
        "}"
        "QLineEdit:focus { border: 1px solid #6C63FF; }"
        "QPushButton {"
        "  background: #6C63FF; color: white; border: none; border-radius: 6px;"
        "  padding: 10px 24px; font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: #7B73FF; }"
        "QPushButton:pressed { background: #5A52E0; }"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(16);

    // 标题
    auto* titleLabel = new QLabel(QStringLiteral("MiniSense v2.1"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #6C63FF;");
    layout->addWidget(titleLabel);

    // 密码输入
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_passwordEdit);

    // 错误提示
    m_errorLabel = new QLabel(this);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #F44336; font-size: 12px;");
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    // 登录按钮
    auto* loginBtn = new QPushButton(QStringLiteral("登录"), this);
    layout->addWidget(loginBtn);

    // 信号连接
    connect(loginBtn, &QPushButton::clicked, this, &LoginDialogController::onLoginClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialogController::onLoginClicked);

    m_passwordEdit->setFocus();
}

void LoginDialogController::onLoginClicked() {
    if (m_passwordEdit->text() == "1") {
        accept();
    } else {
        m_errorLabel->setText(QStringLiteral("密码错误，请重试"));
        m_errorLabel->show();
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}
