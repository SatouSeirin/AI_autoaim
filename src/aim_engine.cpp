#include "engine.hpp"
#include <iostream>
#include <chrono>
#include <cmath>
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
    std::lock_guard<std::mutex> lock(config_mutex_);

    // 检测中文路径
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

    // 初始化 DXGI 截屏
    if (!capturer_.Initialize(config_.capture_size)) {
        std::cerr << "[Engine] Screen capturer init failed" << std::endl;
        return false;
    }

    // 初始化鼠标
    mouse_.Init(MouseBackend::SendInput);

    std::cout << "[Engine] Model loaded & ready. Model=" << modelPath << std::endl;
    return true;
}

// ============================================================
// 加载预编译 .engine 文件（拖入即用）
// ============================================================
bool AimEngine::LoadEngineFile(const std::string& enginePath) {
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
        worker_.join();
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
// 推理主循环（运行在独立 std::thread）
// ============================================================
void AimEngine::RunLoop() {
    auto lastTime   = std::chrono::steady_clock::now();
    int  frameCount = 0;

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

        // 4) 坐标映射因子：模型输入空间 → 截图空间（必须在目标选择和鼠标移动之前计算）
        float sx = static_cast<float>(cfg.capture_size) / detector_.InputWidth();
        float sy = static_cast<float>(cfg.capture_size) / detector_.InputHeight();

        // 5) 目标选择（几何正中心）
        float frameCX = static_cast<float>(cfg.capture_size) / 2.0f;
        float frameCY = static_cast<float>(cfg.capture_size) / 2.0f;
        const Detection* target = selector_.SelectBest(detections, frameCX / sx, frameCY / sy, cfg.target_class_ids);

        // 6) 瞄准键按下 → 鼠标移动（坐标系转换 + FOV 范围检查 + 死区）
        if (target && (GetAsyncKeyState(cfg.aim_key) & 0x8000)) {
            // 目标坐标从模型空间缩放到截图空间
            // X = 框水平中心；Y = 框顶部 + height * target_y_ratio（头部/胸部/腹部偏移）
            float targetCX = target->center_x() * sx;
            float targetCY = (target->y1 + target->height() * cfg.target_y_ratio) * sy;

            double offsetX = targetCX - frameCX;
            double offsetY = targetCY - frameCY;

            // FOV 范围检查：只瞄准 aim_range_size% 范围内的目标
            float halfRange = (cfg.aim_range_size / 100.0f) * cfg.capture_size / 2.0f;
            float dist = std::sqrt(static_cast<float>(offsetX * offsetX + offsetY * offsetY));

            // 死区：偏移 < 3px 不移动（防止微小抖动）
            bool inDeadZone = (std::abs(offsetX) < 3.0 && std::abs(offsetY) < 3.0);

            if (dist <= halfRange && !inDeadZone) {
                double dx = offsetX * cfg.aim_smoothing;
                double dy = offsetY * cfg.aim_smoothing;
                mouse_.MoveMouse(dx, dy, cfg.sensitivity);
            }
        }

        // 7) 可视化：在帧上绘制检测框 + FOV圈（可选）
        cv::Mat visFrame;
        if (cfg.enable_visualization) {
            visFrame = frame.clone();

            int capSize = cfg.capture_size;
            // sx, sy 已在步骤4计算（模型输入空间 → 截图空间）

            // ── 检测框（受 draw_detection_boxes 控制） ──
            if (cfg.draw_detection_boxes) {
                static int drawDiagCount = 0;
                // 只绘制置信度最高的前 5 个框，减少视觉混乱
                const int maxDrawBoxes = 5;
                int drawnCount = 0;
                for (const auto& det : detections) {
                    if (drawnCount >= maxDrawBoxes) break;

                    // det.x1/y1/x2/y2 是归一化到 [0,1] 的坐标（已 / inputW/H）
                    // 映射到截图空间：直接 * capSize
                    int dx1 = static_cast<int>(det.x1 * capSize);
                    int dy1 = static_cast<int>(det.y1 * capSize);
                    int dx2 = static_cast<int>(det.x2 * capSize);
                    int dy2 = static_cast<int>(det.y2 * capSize);

                    // 每 60 帧打印一次绘制诊断（仅前 5 个）
                    if (drawDiagCount % 60 == 0 && drawnCount < 5) {
                        std::cout << "[Draw] det cls=" << det.class_id
                                  << " norm=(" << det.x1 << "," << det.y1 << " " << det.x2 << "," << det.y2 << ")"
                                  << " sx=" << sx << " sy=" << sy
                                  << " pixel=(" << dx1 << "," << dy1 << " " << dx2 << "," << dy2 << ")"
                                  << " capSize=" << capSize << std::endl;
                    }

                    bool isTargetClass = false;
                    for (int tcid : cfg.target_class_ids) {
                        if (det.class_id == tcid) { isTargetClass = true; break; }
                    }
                    cv::Scalar color = isTargetClass
                                       ? cv::Scalar(0, 255, 0)   // 目标类：绿色
                                       : cv::Scalar(0, 0, 255);  // 其他：红色
                    cv::rectangle(visFrame, cv::Point(dx1, dy1), cv::Point(dx2, dy2),
                                  color, 2);

                    drawnCount++;
                }
                drawDiagCount++;

                // 检测数调试文字（左上角） + 模型格式信息
                std::string detInfo = "dets:" + std::to_string(detections.size())
                                    + " fps:" + std::to_string(static_cast<int>(current_fps_.load()));
                cv::putText(visFrame, detInfo, cv::Point(6, 18),
                            cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 255, 0), 1, cv::LINE_8);

                // 模型输出格式信息（第2行）
                std::string fmtInfo = "fmt:" + std::to_string(detector_.InputWidth()) + "x"
                                    + std::to_string(detector_.InputHeight())
                                    + " nc:" + std::to_string(detector_.NumClasses())
                                    + " nb:" + std::to_string(detector_.NumBoxes())
                                    + (detector_.IsTransposed() ? " T" : " N");
                cv::putText(visFrame, fmtInfo, cv::Point(6, 38),
                            cv::FONT_HERSHEY_SIMPLEX, 0.40, cv::Scalar(255, 255, 0), 1, cv::LINE_8);

                // ── RAW 输出值诊断（采样 box 0~2）──
                // 格式: [cx, cy, w, h, cls0, cls1, cls2] (网格空间原始值)
                const float* raw = detector_.RawOutput();
                int outCols = detector_.OutputCols();
                int outStride = detector_.OutputStride();
                bool transp = detector_.IsTransposed();
                auto raw_idx = [&](int box, int col) -> int {
                    return transp ? (box * outStride + col) : (col * outStride + box);
                };
                for (int bi = 0; bi < 3; bi++) {
                    char rbuf[256];
                    snprintf(rbuf, sizeof(rbuf), "raw[%d]=%.2f,%.2f,%.2f,%.2f |c0:%.2f c1:%.2f c2:%.2f",
                        bi,
                        raw[raw_idx(bi, 0)], raw[raw_idx(bi, 1)],
                        raw[raw_idx(bi, 2)], raw[raw_idx(bi, 3)],
                        raw[raw_idx(bi, 4)], raw[raw_idx(bi, 5)],
                        outCols > 6 ? raw[raw_idx(bi, 6)] : 0.0f);
                    cv::putText(visFrame, std::string(rbuf), cv::Point(6, 56 + bi * 18),
                                cv::FONT_HERSHEY_SIMPLEX, 0.30, cv::Scalar(0, 255, 255), 1, cv::LINE_8);
                }

                // 前 3 个检测结果（继续往下画）
                for (int di = 0; di < 3 && di < static_cast<int>(detections.size()); di++) {
                    const auto& d = detections[di];
                    char buf[256];
                    snprintf(buf, sizeof(buf), "det#%d c%d (%.2f,%.2f %.2f,%.2f) cf:%.2f",
                        di, d.class_id,
                        d.x1, d.y1, d.x2, d.y2, d.confidence);
                    cv::putText(visFrame, std::string(buf), cv::Point(6, 110 + di * 18),
                                cv::FONT_HERSHEY_SIMPLEX, 0.32, cv::Scalar(255, 0, 255), 1, cv::LINE_8);
                }
            }

            // ── 几何中心十字准星（白色，始终在画面正中心） ──
            int geoCX = static_cast<int>(frameCX);
            int geoCY = capSize / 2;
            int crossLen = 16;
            cv::line(visFrame, cv::Point(geoCX - crossLen, geoCY),
                     cv::Point(geoCX + crossLen, geoCY),
                     cv::Scalar(255, 255, 255), 1, cv::LINE_8);
            cv::line(visFrame, cv::Point(geoCX, geoCY - crossLen),
                     cv::Point(geoCX, geoCY + crossLen),
                     cv::Scalar(255, 255, 255), 1, cv::LINE_8);

            // ── FOV 瞄准范围（白色细线，以几何中心为圆心） ──
            int fovR = static_cast<int>((cfg.aim_range_size / 100.0f) * capSize / 2.0f);
            if (fovR < 20) fovR = 20;

            if (cfg.aim_range_circle) {
                cv::circle(visFrame, cv::Point(geoCX, geoCY), fovR,
                           cv::Scalar(255, 255, 255), 1, cv::LINE_8);
            } else {
                cv::rectangle(visFrame,
                    cv::Point(geoCX - fovR, geoCY - fovR),
                    cv::Point(geoCX + fovR, geoCY + fovR),
                    cv::Scalar(255, 255, 255), 1, cv::LINE_8);
            }

            // ── 边框调试指示器：紫色 = 可视化激活 ──
            cv::rectangle(visFrame, cv::Point(2, 2),
                          cv::Point(capSize - 3, capSize - 3),
                          cv::Scalar(255, 0, 255), 2, cv::LINE_8);

            // 保存最新帧
            {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                last_frame_ = visFrame.clone();
            }

            // 帧回调（通知 UI）
            FrameCallback cb;
            {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                cb = frame_callback_;
            }
            if (cb) {
                cb(visFrame);
            }
        } else {
            // 不可视化时仍然保存最后一帧
            {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                last_frame_ = frame.clone();
            }
        }

        // 8) FPS 统计
        frameCount++;
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastTime).count();
        if (elapsed >= 1.0) {
            current_fps_.store(frameCount / elapsed);
            frameCount = 0;
            lastTime = now;
        }

        // 9) 退出键检测
        if (GetAsyncKeyState(cfg.exit_key) & 0x8000) {
            std::cout << "[Engine] Exit key pressed, stopping..." << std::endl;
            running_.store(false);
            break;
        }
    }
}
