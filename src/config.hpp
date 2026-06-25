#pragma once

#include <string>
#include <vector>

enum class MouseBackend {
    SendInput,
    IbInputSimulator,
    KMBoxNet
};

struct AimConfig {
    // 模型
    std::string model_path = "";
    bool        enable_fp16      = true;
    float       confidence_threshold = 0.5f;
    float       nms_threshold        = 0.4f;

    // 画面
    int         capture_size = 640;

    // 瞄准
    std::vector<int> target_class_ids = {0};
    float       target_y_ratio  = 0.3f;
    int         target_body_part = 0;
    int         aim_range_size  = 21;
    bool        aim_range_circle = true;

    // PID
    double      kp = 0.25;
    double      ki = 0.0;
    double      kd = 0.08;

    // 卡尔曼
    int         kalman_prediction_steps = 3;

    // 灵敏度
    double      sensitivity = 0.15;

    // 按键
    int         aim_key           = 0x02;
    int         exit_key          = 0x71;
    int         switch_target_key = 0x06;
    int         start_stop_key    = 0x74;

    // 鼠标后端
    MouseBackend mouse_backend = MouseBackend::SendInput;
    std::string kmbox_ip   = "192.168.2.188";
    std::string kmbox_port = "8888";
    std::string kmbox_mac  = "01FBC068";
    bool        kmbox_encrypt = true;
    bool        kmbox_bezier  = true;

    // 可视化
    bool enable_visualization   = true;
    bool show_infer_latency     = true;
    bool draw_detection_boxes   = true;
    bool debug_window_enabled   = false;
    std::string window_name = "AutoAim v2.1";

    // 通用
    bool auto_start     = false;
    bool minimize_to_tray = true;
    std::string language = "zh-CN";
};
