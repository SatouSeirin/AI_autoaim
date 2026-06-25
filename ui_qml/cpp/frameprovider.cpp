#include "frameprovider.h"

FrameProvider::FrameProvider()
    : QQuickImageProvider(QQmlImageProviderBase::Image) {
}

QImage FrameProvider::requestImage(const QString& /*id*/, QSize* size, const QSize& requestedSize) {
    QMutexLocker lock(&m_mutex);
    if (m_currentImage.isNull()) {
        // 返回占位图
        QImage placeholder(640, 640, QImage::Format_RGB888);
        placeholder.fill(QColor(26, 26, 46));
        if (size) *size = placeholder.size();
        return placeholder;
    }

    QImage img = m_currentImage;
    if (size) *size = img.size();

    // 如果指定了请求尺寸且差异较大，进行缩放
    if (requestedSize.isValid() && requestedSize != img.size()) {
        img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return img;
}

void FrameProvider::updateFrame(const cv::Mat& frame) {
    if (frame.empty()) return;

    // 引擎线程上完成颜色转换（避免阻塞 UI 线程）
    cv::Mat rgb;
    if (frame.channels() == 4) {
        cv::cvtColor(frame, rgb, cv::COLOR_BGRA2RGB);
    } else if (frame.channels() == 3) {
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    } else {
        return;
    }

    // 深拷贝脱离 cv::Mat 内存
    QImage qimg(rgb.data, rgb.cols, rgb.rows,
                static_cast<int>(rgb.step), QImage::Format_RGB888);

    QMutexLocker lock(&m_mutex);
    m_currentImage = qimg.copy();
    m_frameCounter++;
}
