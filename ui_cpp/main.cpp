#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "mainwindow.h"

// ============================================================
// Chimera AI v2.1 — 纯 C++ Qt6 UI + TensorRT 推理
//
// 命令行参数:
//   --model <path>      模型路径 (.onnx / .engine)
//   --conf <threshold>  置信度阈值 (默认 0.5)
//   --crop <size>       截取区域边长 (默认 640)
//   --headless          纯命令行模式（不启动 UI）
// ============================================================

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Chimera AI");
    app.setApplicationVersion("2.1.0");

    // 命令行解析
    QCommandLineParser parser;
    parser.setApplicationDescription("Chimera AI — Real-time AI aiming system");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({
        {"model",   "模型路径 (.onnx/.engine)", "path"},
        {"conf",    "置信度阈值 (0.0~1.0)",      "value"},
        {"crop",    "截取区域边长 (px)",          "size"},
        {"headless", "纯命令行模式（不启动 UI）"},
    });

    if (!parser.parse(app.arguments())) {
        qWarning() << parser.errorText();
    }

    QString modelPath = parser.value("model");

    MainWindow window(modelPath);
    window.show();

    return app.exec();
}
