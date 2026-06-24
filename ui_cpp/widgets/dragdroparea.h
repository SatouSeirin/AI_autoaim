#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QDropEvent>

// ============================================================
// DragDropArea — v2.1
// 拖拽 .onnx / .engine 文件到此区域触发加载
// 支持拖拽和点击选择文件两种方式
// ============================================================
class DragDropArea : public QWidget {
    Q_OBJECT

public:
    explicit DragDropArea(QWidget* parent = nullptr);

    // 设置是否显示选择文件按钮
    void setShowSelectButton(bool show);

signals:
    void fileDropped(const QString& filePath);  // 拖拽或选择文件后触发

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSelectFile();

private:
    void updateStyle(bool hover);

    QLabel*      cloudIcon_   = nullptr;
    QLabel*      hintLabel_   = nullptr;
    QLabel*      subHintLabel_ = nullptr;
    QPushButton* selectBtn_   = nullptr;
    bool         hovered_     = false;
};
