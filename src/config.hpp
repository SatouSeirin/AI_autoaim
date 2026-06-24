#pragma once

#include <string>
#include <vector>

// ============================================================
// 鼠标后端枚举（必须在 AimConfig 之前声明）
// ============================================================
enum class MouseBackend {
    SendInput,          // Windows API
    IbInputSimulator,   // 驱动级注入
    KMBoxNet            // 硬件 USB HID (v2.2)
};

// ============================================================
// AimConfig — v2.1（.onnx/.engine 双格式 + 完整瞄准参数）
// ============================================================
struct AimConfig {
    // ── 模型 ──
    std::string model_path = "";
    bool        enable_fp16      = true;
    float       confidence_threshold = 0.5f;
    float       nms_threshold        = 0.4f;

    // ── 画面 ──
    int         capture_size = 640;

    // ── 瞄准 ──
    std::vector<int> target_class_ids = {0};       // 用户勾选的目标类别（可多选）
    float       target_y_ratio  = 0.3f;          // 瞄准点 Y 偏移
    int         target_body_part = 0;            // 0=head 1=chest 2=abdomen
    int         aim_range_size  = 21;            // 瞄准范围 (%)
    bool        aim_range_circle = true;         // true=圆形 false=矩形
    double      sensitivity     = 1.0;
    double      aim_smoothing   = 1.0;

    // ── 鼠标曲线 ──
    double      kp              = 0.250;         // PD 比例增益
    double      ki              = 0.0008;        // PID 积分增益
    int         kalman_ms       = 0;            // 卡尔曼预测 (ms)
    int         delay_comp_ms   = 5;            // 延迟矫正补偿 (ms)

    // ── 按键 ──
    int         aim_key           = 0x02;         // VK_RBUTTON (右键瞄准，避免与UI左键冲突)
    int         exit_key          = 0x71;         // VK_F2
    int         switch_target_key = 0x06;         // VK_XBUTTON1 (鼠标侧键1)
    int         start_stop_key    = 0x74;         // VK_F5 (全局开始/停止推理)

    // ── 输入模式 ──
    MouseBackend mouse_backend = MouseBackend::SendInput;

    // KMBox 配置
    std::string kmbox_ip   = "192.168.2.188";
    std::string kmbox_port = "8888";
    std::string kmbox_mac  = "01FBC068";
    bool        kmbox_encrypt = true;
    bool        kmbox_bezier  = true;

    // ── 可视化 ──
    bool        enable_visualization   = true;
    bool        show_infer_latency     = true;
    bool        draw_detection_boxes   = true;
    bool        debug_window_enabled   = false;
    std::string window_name = "AutoAim v2.1";

    // ── 通用 ──
    bool        auto_start     = false;
    bool        minimize_to_tray = true;
    std::string language = "zh-CN";
};

