#include "engine.hpp"
#include <iostream>
#include <chrono>
#include <cmath>
#include <cstring>
#include <windows.h>

// ============================================================
// 构造/析构
// ============================================================
AimEngine::AimEngine() {}

AimEngine::~AimEngine() {
    Stop();
}

// ============================================================
// 配置
// ============================================================
void AimEngine::SetConfig(const AimConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

void AimEngine::UpdateConfig(const AimConfig& config) {
    SetConfig(config);
}

AimConfig AimEngine::GetConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

// ============================================================
// 模型加载（v2.0: 仅 .onnx）
// ============================================================
bool AimEngine::LoadModel(const std::string& modelPath) {
    bool wasRunning = running_.load();
    if (wasRunning) {
        std::cout << "[Engine] Stopping engine before loading new model..." << std::endl;
        Stop();
    }
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        for (char c : modelPath) {
            if (static_cast<unsigned char>(c) > 127) {
                std::cerr << "[Engine] ERROR: Model path contains Chinese characters. "
                          << "Please move model to a path without Chinese characters." << std::endl;
                return false;
            }
        }
        config_.model_path = modelPath;
        if (!detector_.LoadModel(modelPath, config_)) {
            std::cerr << "[Engine] Model load failed: " << detector_.GetLastError() << std::endl;
            return false;
        }
        if (!capturer_.Initialize(config_.capture_size)) {
            std::cerr << "[Engine] Screen capturer init failed" << std::endl;
            return false;
        }
        mouse_.Init(MouseBackend::SendInput);
        std::cout << "[Engine] Model loaded & ready. Model=" << modelPath << std::endl;
    }
    if (wasRunning) {
        std::cout << "[Engine] Restarting engine after model switch..." << std::endl;
        Start();
    }
    return true;
}

// ============================================================
// 加载预编译 .engine 文件（拖入即用）
// ============================================================
bool AimEngine::LoadEngineFile(const std::string& enginePath) {
    bool wasRunning = running_.load();
    if (wasRunning) {
        std::cout << "[Engine] Stopping engine before loading new engine..." << std::endl;
        Stop();
    }
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        for (char c : enginePath) {
            if (static_cast<unsigned char>(c) > 127) {
                std::cerr << "[Engine] ERROR: Engine path contains Chinese characters." << std::endl;
                return false;
            }
        }
        config_.model_path = enginePath;
        if (!detector_.LoadEngineFile(enginePath, config_)) {
            std::cerr << "[Engine] Engine load failed: " << detector_.GetLastError() << std::endl;
            return false;
        }
        if (!capturer_.Initialize(config_.capture_size)) {
            std::cerr << "[Engine] Screen capturer init failed" << std::endl;
            return false;
        }
        mouse_.Init(MouseBackend::SendInput);
        std::cout << "[Engine] Engine loaded & ready. Engine=" << enginePath << std::endl;
    }
    if (wasRunning) {
        std::cout << "[Engine] Restarting engine after engine switch..." << std::endl;
        Start();
    }
    return true;
}

bool AimEngine::IsModelLoaded() const {
    return detector_.IsLoaded() && capturer_.IsInitialized();
}

std::string AimEngine::GetLastError() const {
    return detector_.GetLastError();
}

// ============================================================
// 运行时修改截图尺寸（停止推理 → 重建截图器 → 恢复推理）
// ============================================================
bool AimEngine::SetCaptureSize(int size) {
    if (size < 64 || size > 1920) return false;
    bool wasRunning = running_.load();
    if (wasRunning) Stop();

    capturer_.Shutdown();
    if (!capturer_.Initialize(size)) {
        std::cerr << "[Engine] Failed to init capturer at " << size << std::endl;
        // 尝试恢复旧尺寸
        capturer_.Initialize(config_.capture_size);
        if (wasRunning) Start();
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_.capture_size = size;
    }

    if (wasRunning) Start();
    std::cout << "[Engine] Capture size changed to " << size << std::endl;
    return true;
}

// ============================================================
// 启动 / 停止推理线程
// ============================================================
void AimEngine::Start() {
    if (running_.load()) {
        std::cerr << "[Engine] Already running" << std::endl;
        return;
    }
    if (!IsModelLoaded()) {
        std::cerr << "[Engine] Model not loaded, cannot start" << std::endl;
        return;
    }

    running_.store(true);
    worker_ = std::thread(&AimEngine::RunLoop, this);
    std::cout << "[Engine] Started" << std::endl;
}

void AimEngine::Stop() {
    if (!running_.load()) return;
    running_.store(false);
    if (worker_.joinable()) {
        auto start = std::chrono::steady_clock::now();
        while (worker_.joinable()) {
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() > 3.0) {
                std::cerr << "[Engine] Stop timeout, detaching worker" << std::endl;
                worker_.detach();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    std::cout << "[Engine] Stopped" << std::endl;
}

// ============================================================
// 帧回调注册
// ============================================================
void AimEngine::SetFrameCallback(FrameCallback cb) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    frame_callback_ = std::move(cb);
}

// ============================================================
// 获取最新帧（线程安全）
// ============================================================
cv::Mat AimEngine::GetLastFrame() {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    if (last_frame_.empty()) return cv::Mat();
    return last_frame_.clone();
}

// ============================================================
// 获取最新检测结果（线程安全）
// ============================================================
std::vector<Detection> AimEngine::GetLastDetections() const {
    std::lock_guard<std::mutex> lock(detections_mutex_);
    return last_detections_;
}

// ============================================================
// 测试鼠标移动（供 UI 测试按钮调用）
// ============================================================
void AimEngine::MoveMouseRelative(double dx, double dy) {
    mouse_.MoveMouse(dx, dy, 1.0);
}

// ============================================================
// 推理主循环（运行在独立 std::thread）
// ============================================================
void AimEngine::RunLoop() {
    auto lastTime   = std::chrono::steady_clock::now();
    int  frameCount = 0;
    bool  prevTargetValid = false;
    float kalmanPredictX = 0.5f, kalmanPredictY = 0.5f;
    last_target_time_      = std::chrono::steady_clock::now();
    last_kalman_update_time_ = std::chrono::steady_clock::now();

    while (running_.load()) {
        auto frameStart = std::chrono::steady_clock::now();

        // 1) DXGI 截屏
        cv::Mat frame;
        if (!capturer_.Capture(frame)) {
            continue;  // 无新帧，立即重试
        }

        // 2) 读取当前配置（必须在推理前获取，因为 target_class_ids/conf/nms 需要实时）
        AimConfig cfg;
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            cfg = config_;
        }


        // 3) 推理
        auto inferStart   = std::chrono::steady_clock::now();
        auto detections   = detector_.Infer(frame, cfg);
        auto inferEnd     = std::chrono::steady_clock::now();
        double inferMs    = std::chrono::duration<double, std::milli>(inferEnd - inferStart).count();
        current_infer_ms_.store(inferMs);

        // 存储检测结果供 QML Canvas 绘制
        {
            std::lock_guard<std::mutex> lock(detections_mutex_);
            last_detections_ = detections;
        }

        // 4) 目标选择：在 FOV 范围内选距画面中心最近的敌人
        // Detection 坐标已归一化到 [0,1]，所以 frameCenter 和 fovRadius 也需归一化
        float frameCenterNorm = 0.5f;  // 画面中心归一化坐标
        // FOV 半径（归一化空间）：aim_range_size% 的一半
        float fovRadiusNorm = cfg.aim_range_size / 100.0f * 0.5f;
        const Detection* target = selector_.SelectBest(detections, frameCenterNorm, frameCenterNorm,
                                                       cfg.target_class_ids, fovRadiusNorm);

        // 5) PID + Kalman + behavior control
        {
            auto nowStep = std::chrono::steady_clock::now();
            double stepDt = std::chrono::duration<double>(nowStep - last_target_time_).count();
            last_target_time_ = nowStep;

            if (target && (GetAsyncKeyState(cfg.aim_key) & 0x8000)) {
                float tgtNormX = target->center_x();
                float tgtNormY = (target->y1 + target->height() * cfg.target_y_ratio);
                kalman_.Update(tgtNormX, tgtNormY, target->confidence, static_cast<float>(stepDt));

                float predX = 0.5f, predY = 0.5f;
                kalman_.PredictAhead(cfg.kalman_prediction_steps, static_cast<float>(stepDt), predX, predY);

                float targetCX = predX * cfg.capture_size;
                float targetCY = predY * cfg.capture_size;
                float frameCX  = static_cast<float>(cfg.capture_size) / 2.0f;
                float frameCY  = static_cast<float>(cfg.capture_size) / 2.0f;
                double offsetX = targetCX - frameCX;
                double offsetY = targetCY - frameCY;

                if (std::abs(offsetX) < 3) offsetX = 0;
                if (std::abs(offsetY) < 3) offsetY = 0;

                if (offsetX != 0 || offsetY != 0) {
                    pid_x_.SetGains(cfg.kp, cfg.ki, cfg.kd);
                    pid_y_.SetGains(cfg.kp, cfg.ki, cfg.kd);
                    double pidOutX = pid_x_.Compute(offsetX, stepDt);
                    double pidOutY = pid_y_.Compute(offsetY, stepDt);


                    double dx = pidOutX * cfg.sensitivity;
                    double dy = pidOutY * cfg.sensitivity;
                    dx = std::clamp(dx, -200.0, 200.0);
                    dy = std::clamp(dy, -200.0, 200.0);
                    mouse_.MoveMouse(dx, dy, 1.0);
                }
                prevTargetValid = true;

            } else if (prevTargetValid && kalman_.IsInitialized() && (GetAsyncKeyState(cfg.aim_key) & 0x8000)) {
                kalman_.MarkLost();
                int lostCount = kalman_.GetLostCount();
                if (lostCount <= 5) {
                    float pX, pY;
                    kalman_.PredictAhead(cfg.kalman_prediction_steps, static_cast<float>(stepDt), pX, pY);
                    float tCX = pX * cfg.capture_size;
                    float tCY = pY * cfg.capture_size;
                    float fCX = static_cast<float>(cfg.capture_size) / 2.0f;
                    float fCY = static_cast<float>(cfg.capture_size) / 2.0f;
                    double oX = tCX - fCX;
                    double oY = tCY - fCY;

                    if (std::abs(oX) < 3) oX = 0;
                    if (std::abs(oY) < 3) oY = 0;

                    if (oX != 0 || oY != 0) {
                        pid_x_.SetGains(cfg.kp, cfg.ki, cfg.kd);
                        pid_y_.SetGains(cfg.kp, cfg.ki, cfg.kd);
                        double poX = pid_x_.Compute(oX, stepDt);
                        double poY = pid_y_.Compute(oY, stepDt);

                        
                        double dx = poX * cfg.sensitivity;
                        double dy = poY * cfg.sensitivity;
                        dx = std::clamp(dx, -200.0, 200.0);
                        dy = std::clamp(dy, -200.0, 200.0);
                        mouse_.MoveMouse(dx, dy, 1.0);
                    }
                } else {
                    pid_x_.Reset();
                    pid_y_.Reset();
                    prevTargetValid = false;
                }
            } else {
                prevTargetValid = false;
            }
        }        // 6) 保存帧并通知 UI（所有叠加绘制由 QML 负责）
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            last_frame_ = frame.clone();
        }

        FrameCallback cb;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            cb = frame_callback_;
        }
        if (cb) {
            cb(frame);
        }

        // 7) FPS 统计
        frameCount++;
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastTime).count();
        if (elapsed >= 1.0) {
            current_fps_.store(frameCount / elapsed);
            frameCount = 0;
            lastTime = now;
        }

        // 8) 退出键检测
        if (GetAsyncKeyState(cfg.exit_key) & 0x8000) {
            std::cout << "[Engine] Exit key pressed, stopping..." << std::endl;
            running_.store(false);
            break;
        }
    }
}
