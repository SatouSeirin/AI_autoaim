#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickStyle>
#include <QPalette>
#include <QDir>
#include <QFileInfo>
#include <QCommandLineParser>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

#include "engineviewmodel.h"
#include "frameprovider.h"
#include "logindialogcontroller.h"
#include "hotkeycapturedispatcher.h"

// ============================================================
// MiniSense v2.1 — QML UI 入口
// ============================================================
int main(int argc, char* argv[]) {
    // 分配调试控制台
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);  // 禁用缓冲，立即输出

    // 高 DPI 支持
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Qt Quick Controls 风格
    QQuickStyle::setStyle("Fusion");

    QApplication app(argc, argv);
    app.setOrganizationName("MiniSense");
    app.setApplicationName("MiniSense v2.1");
    app.setApplicationVersion("2.1.0");

    // 深色调色板（Fusion 基础）
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor("#1A1A2E"));
    darkPalette.setColor(QPalette::WindowText,      QColor("#E8E8F0"));
    darkPalette.setColor(QPalette::Base,            QColor("#252540"));
    darkPalette.setColor(QPalette::AlternateBase,   QColor("#1E1E32"));
    darkPalette.setColor(QPalette::ToolTipBase,     QColor("#252540"));
    darkPalette.setColor(QPalette::ToolTipText,     QColor("#E8E8F0"));
    darkPalette.setColor(QPalette::Text,            QColor("#E8E8F0"));
    darkPalette.setColor(QPalette::Button,          QColor("#252540"));
    darkPalette.setColor(QPalette::ButtonText,      QColor("#E8E8F0"));
    darkPalette.setColor(QPalette::BrightText,      QColor("#FF5555"));
    darkPalette.setColor(QPalette::Link,            QColor("#6C63FF"));
    darkPalette.setColor(QPalette::Highlight,       QColor("#6C63FF"));
    darkPalette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    darkPalette.setColor(QPalette::PlaceholderText, QColor("#606078"));
    app.setPalette(darkPalette);

    // 命令行参数
    QCommandLineParser parser;
    parser.setApplicationDescription("MiniSense v2.1 - QML UI");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption modelOption("model", "Model file path (.onnx or .engine)", "path");
    parser.addOption(modelOption);

    QCommandLineOption skipLoginOption("skip-login", "Skip login dialog");
    parser.addOption(skipLoginOption);

    parser.process(app);

    // 登录对话框
    if (!parser.isSet(skipLoginOption)) {
        LoginDialogController loginDlg;
        if (loginDlg.exec() != QDialog::Accepted) {
            return 0;
        }
    }

    // 创建帧提供者
    FrameProvider frameProvider;

    // 创建引擎 ViewModel
    EngineViewModel engineVM(&frameProvider);

    // 命令行模型覆盖
    if (parser.isSet(modelOption)) {
        QString modelPath = parser.value(modelOption);
        engineVM.loadModel(modelPath);
    }

    // 设置 QML 引擎
    QQmlApplicationEngine qmlEngine;

    // 添加 Qt QML 模块导入路径（确保插件能找到依赖的 DLL）
    qmlEngine.addImportPath("C:/Qt/6.8.2/6.8.2/msvc2022_64/qml");

    // 注册图像提供者
    qmlEngine.addImageProvider("frames", &frameProvider);

    // 暴露 ViewModel 到 QML
    qmlEngine.rootContext()->setContextProperty("engine", &engineVM);

    // 热键捕获调度器（C++ 事件过滤器，绕过 QML 焦点系统）
    HotkeyCaptureDispatcher hotkeyDispatcher;
    QAbstractNativeEventFilter* nativeFilter = &hotkeyDispatcher;
    app.installNativeEventFilter(nativeFilter);
    qmlEngine.rootContext()->setContextProperty("hotkeyDispatcher", &hotkeyDispatcher);

    // 连接警告输出
    QObject::connect(&qmlEngine, &QQmlApplicationEngine::warnings,
                     [](const QList<QQmlError>& warnings) {
        for (const auto& w : warnings) {
            qWarning() << "QML Warning:" << w.toString();
        }
    });

    // 使用 QQmlComponent 加载以获取详细错误信息
    const QUrl url("qrc:/qml/main.qml");
    QQmlComponent component(&qmlEngine, url);
    if (component.isError()) {
        qCritical() << "QML component error:" << component.errorString();
        return -1;
    }

    QObject* rootObject = component.create();
    if (!rootObject) {
        qCritical() << "Failed to create QML root object";
        return -1;
    }

    return app.exec();
}
