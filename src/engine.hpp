#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <opencv2/opencv.hpp>

#include "config.hpp"
#include "screen_capturer.hpp"
#include "object_detector.hpp"
#include "target_selector.hpp"
#include "mouse_controller.hpp"

// ============================================================
// AimEngine — v2.0 MVP 主循环协调器
//
// 工作流：DXGI 截图 → CPU 预处理 → TensorRT 推理
//        → CPU 后处理 → 目标选择 → 鼠标移动
//
// 用法:
//   AimEngine engine;
//   engine.SetConfig(config);
//   engine.LoadModel("model.onnx");
//   engine.Start();       // 启动推理线程
//   engine.Stop();        // 停止
// ============================================================
class AimEngine {
public:
    // 帧回调类型（跨线程安全传递 cv::Mat 给 UI）
    using FrameCallback = std::function<void(const cv::Mat&)>;

    AimEngine();
    ~AimEngine();

    // ── 配置 + 模型 ──
    void SetConfig(const AimConfig& config);
    void UpdateConfig(const AimConfig& config);  // 运行时更新参数
    AimConfig GetConfig() const;
    bool SetCaptureSize(int size);               // 运行时修改截图尺寸（需重启推理）

    bool LoadModel(const std::string& modelPath);      // .onnx（自动缓存）
    bool LoadEngineFile(const std::string& enginePath); // .engine（拖入即用）

    // ── 控制 ──
    void Start();
    void Stop();
    bool IsRunning() const { return running_.load(); }

    // ── 帧回调（UI 线程注册，推理线程调用）──
    void SetFrameCallback(FrameCallback cb);

    // ── 运行时统计 ──
    double GetFPS()       const { return current_fps_.load(); }
    double GetInferMs()   const { return current_infer_ms_.load(); }
    bool   IsModelLoaded() const;
    std::string GetLastError() const;

    // ── 画面预览帧（线程安全）──
    cv::Mat GetLastFrame();

private:
    void RunLoop();       // 推理线程主循环

    // 组件
    ScreenCapturer  capturer_;
    ObjectDetector  detector_;
    TargetSelector  selector_;
    MouseController mouse_;

    // 配置
    AimConfig config_;
    mutable std::mutex config_mutex_;

    // 线程控制
    std::atomic<bool> running_{false};
    std::thread       worker_;

    // 帧回调
    FrameCallback frame_callback_;
    std::mutex    callback_mutex_;

    // 最新帧（供 UI 读取）
    cv::Mat last_frame_;
    std::mutex frame_mutex_;

    // 性能统计
    std::atomic<double> current_fps_{0.0};
    std::atomic<double> current_infer_ms_{0.0};
};
