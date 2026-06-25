#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MiniSense");
    app.setApplicationVersion("2.1.0");

    // Login dialog
    QDialog loginDlg;
    loginDlg.setWindowTitle("MiniSense - Login");
    loginDlg.setFixedSize(400, 320);

    auto* layout = new QVBoxLayout(&loginDlg);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);

    auto* title = new QLabel("MiniSense");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 28px; font-weight: 700; color: #FFFFFF;");
    layout->addWidget(title);

    auto* pwdInput = new QLineEdit();
    pwdInput->setPlaceholderText("Enter card key");
    pwdInput->setEchoMode(QLineEdit::Password);
    layout->addWidget(pwdInput);

    auto* errLabel = new QLabel("");
    errLabel->setStyleSheet("color: red;");
    errLabel->setVisible(false);
    layout->addWidget(errLabel);

    auto* loginBtn = new QPushButton("Login");
    loginBtn->setMinimumHeight(40);
    layout->addWidget(loginBtn);

    QObject::connect(loginBtn, &QPushButton::clicked, [&]() {
        if (pwdInput->text() == "1") {
            loginDlg.accept();
        } else {
            errLabel->setText("Wrong password");
            errLabel->setVisible(true);
        }
    });

    if (loginDlg.exec() != QDialog::Accepted) return 0;

    QCommandLineParser parser;
    parser.addOptions({{"model", "path"}, {"conf", "value"}, {"crop", "size"}, {"headless", ""}});
    parser.process(app);

    MainWindow window(parser.value("model"));
    window.show();
    return app.exec();
}
