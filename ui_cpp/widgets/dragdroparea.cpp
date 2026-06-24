#include "dragdroparea.h"

#include <QVBoxLayout>
#include <QMimeData>
#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>
#include <QStyle>
#include <QUrl>

DragDropArea::DragDropArea(QWidget* parent)
    : QWidget(parent) {
    setAcceptDrops(true);
    setMinimumHeight(220);
    setObjectName("dragDropArea");

    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(10);

    // 云图标
    cloudIcon_ = new QLabel("☁", this);
    cloudIcon_->setAlignment(Qt::AlignCenter);
    cloudIcon_->setStyleSheet("font-size: 48px; color: #B0B0B0; background: transparent;");
    layout->addWidget(cloudIcon_);

    // 提示文字
    hintLabel_ = new QLabel("拖拽 .onnx 或 .engine 文件到此处", this);
    hintLabel_->setAlignment(Qt::AlignCenter);
    hintLabel_->setStyleSheet("font-size: 14px; color: #888888; background: transparent;");
    layout->addWidget(hintLabel_);

    // 小字警告
    subHintLabel_ = new QLabel("（模型和路径不要出现中文）", this);
    subHintLabel_->setAlignment(Qt::AlignCenter);
    subHintLabel_->setStyleSheet("font-size: 12px; color: #AAAAAA; background: transparent;");
    layout->addWidget(subHintLabel_);

    // 选择文件按钮
    selectBtn_ = new QPushButton(" 选择文件 ", this);
    selectBtn_->setObjectName("btnPrimary");
    selectBtn_->setCursor(Qt::PointingHandCursor);
    selectBtn_->setFixedWidth(140);
    connect(selectBtn_, &QPushButton::clicked, this, &DragDropArea::onSelectFile);
    layout->addWidget(selectBtn_, 0, Qt::AlignCenter);

    updateStyle(false);
}

void DragDropArea::setShowSelectButton(bool show) {
    selectBtn_->setVisible(show);
}

// ── 拖拽事件 ──
void DragDropArea::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (path.endsWith(".onnx", Qt::CaseInsensitive) ||
                path.endsWith(".engine", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                hovered_ = true;
                updateStyle(true);
                return;
            }
        }
    }
    event->ignore();
}

void DragDropArea::dragLeaveEvent(QDragLeaveEvent* event) {
    hovered_ = false;
    updateStyle(false);
    QWidget::dragLeaveEvent(event);
}

void DragDropArea::dropEvent(QDropEvent* event) {
    hovered_ = false;
    updateStyle(false);

    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (path.endsWith(".onnx", Qt::CaseInsensitive) ||
                path.endsWith(".engine", Qt::CaseInsensitive)) {
                emit fileDropped(path);
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void DragDropArea::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    // 样式由 QSS 控制，此处仅作为保留位
}

void DragDropArea::updateStyle(bool hover) {
    // 样式变化通过动态属性切换（QSS 中用 #dragDropArea[hover="true"]）
    setProperty("hover", hover);
    style()->unpolish(this);
    style()->polish(this);
}

void DragDropArea::onSelectFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "选择模型文件", "",
        "模型文件 (*.onnx *.engine);;ONNX 模型 (*.onnx);;TensorRT 引擎 (*.engine)");
    if (!path.isEmpty()) {
        emit fileDropped(path);
    }
}
