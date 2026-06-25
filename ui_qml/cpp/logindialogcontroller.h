#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

// ============================================================
// LoginDialogController — 深色风格登录对话框
// ============================================================
class LoginDialogController : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialogController(QWidget* parent = nullptr);

private slots:
    void onLoginClicked();

private:
    QLineEdit* m_passwordEdit;
    QLabel* m_errorLabel;
};
