#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>
#include <opencv2/opencv.hpp>

// ============================================================
// FrameProvider — QQuickImageProvider 线程安全帧传递
// 引擎线程写入，QML 渲染线程读取
// ============================================================
class FrameProvider : public QQuickImageProvider {
public:
    FrameProvider();

    // QML Image 请求（渲染线程调用）
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    // 引擎线程调用：更新最新帧
    void updateFrame(const cv::Mat& frame);

    // 获取帧计数器（用于 QML 强制刷新 URL）
    qint64 frameCounter() const { return m_frameCounter; }

private:
    QMutex m_mutex;
    QImage m_currentImage;
    qint64 m_frameCounter = 0;
};
