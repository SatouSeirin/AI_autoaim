#include "previewwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QFrame>
#include <QLabel>

PreviewWindow::PreviewWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QString::fromUtf8("\xe6\x8e\xa8\xe7\x90\x86\xe9\xa2\x84\xe8\xa7\x88"));
    setMinimumSize(280, 320);
    resize(670, 700);
    setStyleSheet("background-color: #1E1E1E;");
    // X 按钮关闭 = 窗口销毁，自动清理状态
    setAttribute(Qt::WA_DeleteOnClose);
    setupUI();
}

void PreviewWindow::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    // ── 顶部控制栏 ──
    auto* topBar = new QWidget(this);
    topBar->setFixedHeight(34);
    topBar->setStyleSheet("background: #2D2D2D; border-radius: 4px;");
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(10, 3, 10, 3);
    topLayout->setSpacing(8);

    auto* hintLabel = new QLabel(
        QString::fromUtf8("\xe6\x88\xaa\xe5\x9b\xbe\xe5\xb0\xba\xe5\xaf\xb8\xe7\x94\xb1\xe4\xb8\xbb\xe7\x95\x8c\xe9\x9d\xa2\xe2\x80\x9c\xe9\xa2\x84\xe8\xa7\x88\xe5\xb0\xba\xe5\xaf\xb8\xe2\x80\x9d\xe6\x8e\xa7\xe5\x88\xb6"),
        topBar);
    hintLabel->setStyleSheet("color: #888; font-size: 11px;");
    topLayout->addWidget(hintLabel);

    topLayout->addStretch();

    chkAlwaysOnTop_ = new QCheckBox(QString::fromUtf8("\xe7\xbd\xae\xe9\xa1\xb6"), topBar);
    chkAlwaysOnTop_->setStyleSheet("QCheckBox { color: #CCC; font-size: 12px; }"
                                    "QCheckBox::indicator { width: 30px; height: 16px; }");
    connect(chkAlwaysOnTop_, &QCheckBox::toggled, this, &PreviewWindow::setAlwaysOnTop);
    topLayout->addWidget(chkAlwaysOnTop_);

    mainLayout->addWidget(topBar);

    // ── 画面标签 ──
    frameLabel_ = new QLabel(this);
    frameLabel_->setAlignment(Qt::AlignCenter);
    frameLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    frameLabel_->setStyleSheet(
        "background: #252525; border: 1px solid #3A3A3A; border-radius: 4px;");
    mainLayout->addWidget(frameLabel_, 1);
}

// ============================================================
// 接收新帧 — 1:1 原始尺寸显示，Qt::FastTransformation 消除摩尔纹
// ============================================================
void PreviewWindow::updateFrame(const QImage& frame) {
    if (frame.isNull()) return;

    lastFrame_ = frame;

    // 窗口自适应到帧尺寸 + 边距
    int neededW = frame.width()  + 12;
    int neededH = frame.height() + 52;
    if (width() != neededW || height() != neededH) {
        resize(neededW, neededH);
    }

    frameLabel_->setPixmap(QPixmap::fromImage(frame));
}

void PreviewWindow::setAlwaysOnTop(bool on) {
    if (on) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    }
    show();
}
