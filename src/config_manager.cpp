#include "config_manager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>

// 简陋的 JSON 读写（无第三方依赖，仅处理 AimConfig 结构）
// v2.1 MVP: 字段逐一手动序列化/反序列化

static std::string readAll(const std::string& path) {
    std::ifstream f(path);
    if (!f.good()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string getStr(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    // 跳过空格/引号
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '"')) pos++;
    auto end = json.find_first_of(",}\n\r\"", pos);
    if (end == std::string::npos) end = json.size();
    return json.substr(pos, end - pos);
}

static double getDouble(const std::string& json, const std::string& key, double def = 0.0) {
    auto s = getStr(json, key);
    if (s.empty() || s == "null") return def;
    return std::stod(s);
}

static int getInt(const std::string& json, const std::string& key, int def = 0) {
    auto s = getStr(json, key);
    if (s.empty() || s == "null") return def;
    return std::stoi(s);
}

static bool getBool(const std::string& json, const std::string& key, bool def = false) {
    auto s = getStr(json, key);
    if (s.empty() || s == "null") return def;
    return s == "true" || s == "1";
}

std::string ConfigManager::EscapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else out += c;
    }
    return out;
}

std::string ConfigManager::MouseBackendToString(MouseBackend b) {
    switch (b) {
        case MouseBackend::SendInput: return "SendInput";
        case MouseBackend::IbInputSimulator: return "IbInputSimulator";
        case MouseBackend::KMBoxNet: return "KMBoxNet";
         case MouseBackend::MaKcu: return "MaKcu";
        default: return "SendInput";
    }
}

MouseBackend ConfigManager::StringToMouseBackend(const std::string& s) {
    if (s == "IbInputSimulator") return MouseBackend::IbInputSimulator;
    if (s == "KMBoxNet") return MouseBackend::KMBoxNet;
     if (s == "MaKcu") return MouseBackend::MaKcu;
    return MouseBackend::SendInput;
}

AimConfig ConfigManager::Load(const std::string& filepath) {
    AimConfig cfg;
    auto json = readAll(filepath);
    if (json.empty()) {
        std::cout << "[Config] No config file found, using defaults." << std::endl;
        return cfg;
    }

    cfg.model_path            = getStr(json, "model_path");
    cfg.enable_fp16           = getBool(json, "enable_fp16", true);
    cfg.confidence_threshold  = static_cast<float>(getDouble(json, "confidence_threshold", 0.5));
    cfg.nms_threshold         = static_cast<float>(getDouble(json, "nms_threshold", 0.4));
    cfg.capture_size          = getInt(json, "capture_size", 640);
    // target_class_ids: 数组格式 "[0, 1, 2]" 或兼容旧格式单个 int
    {
        // 不能用 getStr（它在第一个逗号处截断，无法处理数组）
        // 手动查找 [ ... ] 区间
        std::string key_search = "\"target_class_ids\"";
        auto key_pos = json.find(key_search);
        if (key_pos != std::string::npos) {
            auto bracket_start = json.find('[', key_pos + key_search.size());
            if (bracket_start != std::string::npos) {
                auto bracket_end = json.find(']', bracket_start);
                if (bracket_end != std::string::npos) {
                    std::string idsStr = json.substr(bracket_start + 1, bracket_end - bracket_start - 1);
                    cfg.target_class_ids.clear();
                    std::string num;
                    for (char c : idsStr) {
                        if (c >= '0' && c <= '9') {
                            num += c;
                        } else if (!num.empty()) {
                            cfg.target_class_ids.push_back(std::stoi(num));
                            num.clear();
                        }
                    }
                    if (!num.empty()) cfg.target_class_ids.push_back(std::stoi(num));
                }
            }
        } else {
            // 兼容旧格式: 单个 int "target_class_id"
            int oldVal = getInt(json, "target_class_id", -1);
            if (oldVal >= 0) {
                cfg.target_class_ids = {oldVal};
            }
        }
        if (cfg.target_class_ids.empty()) cfg.target_class_ids = {0};
    }
    cfg.target_y_ratio        = static_cast<float>(getDouble(json, "target_y_ratio", 0.3));
    cfg.target_body_part      = getInt(json, "target_body_part", 0);
    cfg.aim_range_size        = getInt(json, "aim_range_size", 21);
    cfg.aim_range_circle      = getBool(json, "aim_range_circle", true);
    cfg.kp                    = getDouble(json, "kp", 0.25);
    cfg.ki                    = getDouble(json, "ki", 0.0);
     cfg.kd                    = getDouble(json, "kd", 0.08);
    cfg.kalman_prediction_steps = getInt(json, "kalman_prediction_steps", 3);
    cfg.sensitivity            = getDouble(json, "sensitivity", 0.15);
    cfg.aim_key               = getInt(json, "aim_key", 0x02);
    cfg.exit_key              = getInt(json, "exit_key", 0x71);
    cfg.switch_target_key     = getInt(json, "switch_target_key", 0x06);
    cfg.start_stop_key        = getInt(json, "start_stop_key", 0x74);
    cfg.mouse_backend         = StringToMouseBackend(getStr(json, "mouse_backend"));
    cfg.kmbox_ip              = getStr(json, "kmbox_ip");
    if (cfg.kmbox_ip.empty()) cfg.kmbox_ip = "192.168.2.188";
    cfg.kmbox_port            = getStr(json, "kmbox_port");
    if (cfg.kmbox_port.empty()) cfg.kmbox_port = "8888";
    cfg.kmbox_mac             = getStr(json, "kmbox_mac");
    if (cfg.kmbox_mac.empty()) cfg.kmbox_mac = "01FBC068";
    cfg.kmbox_encrypt         = getBool(json, "kmbox_encrypt", true);
    cfg.kmbox_bezier          = getBool(json, "kmbox_bezier", true);
     cfg.makcu_serial          = getStr(json, "makcu_serial");
     if (cfg.makcu_serial.empty()) cfg.makcu_serial = "COM1";
     cfg.makcu_port            = getStr(json, "makcu_port");
     if (cfg.makcu_port.empty()) cfg.makcu_port = "8888";
    cfg.enable_visualization  = getBool(json, "enable_visualization", true);
    cfg.show_infer_latency    = getBool(json, "show_infer_latency", true);
    cfg.draw_detection_boxes  = getBool(json, "draw_detection_boxes", true);
    cfg.draw_aim_on_preview  = getBool(json, "draw_aim_on_preview", true);
    cfg.debug_window_enabled  = getBool(json, "debug_window_enabled", false);
    cfg.window_name           = getStr(json, "window_name");
    if (cfg.window_name.empty()) cfg.window_name = "AutoAim v2.1";
    cfg.auto_start            = getBool(json, "auto_start", false);
    cfg.minimize_to_tray      = getBool(json, "minimize_to_tray", true);
    cfg.language              = getStr(json, "language");
    if (cfg.language.empty()) cfg.language = "zh-CN";

    std::cout << "[Config] Loaded from " << filepath << std::endl;
    std::cout << "[Config]   capture_size=" << cfg.capture_size << std::endl;
    std::cout << "[Config]   target_class_ids=[";
    for (size_t i = 0; i < cfg.target_class_ids.size(); i++) {
        if (i > 0) std::cout << ",";
        std::cout << cfg.target_class_ids[i];
    }
    std::cout << "]" << std::endl;
    return cfg;
}

bool ConfigManager::Save(const AimConfig& cfg, const std::string& filepath) {
    std::ofstream f(filepath);
    if (!f.good()) {
        std::cerr << "[Config] Failed to write " << filepath << std::endl;
        return false;
    }

    f << "{\n";
    f << "  \"model_path\": \""            << EscapeJson(cfg.model_path) << "\",\n";
    f << "  \"enable_fp16\": "             << (cfg.enable_fp16 ? "true" : "false") << ",\n";
    f << "  \"confidence_threshold\": "    << cfg.confidence_threshold << ",\n";
    f << "  \"nms_threshold\": "           << cfg.nms_threshold << ",\n";
    f << "  \"capture_size\": "            << cfg.capture_size << ",\n";
    // target_class_ids: 数组格式
    f << "  \"target_class_ids\": [";
    for (size_t i = 0; i < cfg.target_class_ids.size(); i++) {
        if (i > 0) f << ", ";
        f << cfg.target_class_ids[i];
    }
    f << "],\n";
    f << "  \"target_y_ratio\": "          << cfg.target_y_ratio << ",\n";
    f << "  \"target_body_part\": "        << cfg.target_body_part << ",\n";
    f << "  \"aim_range_size\": "          << cfg.aim_range_size << ",\n";
    f << "  \"aim_range_circle\": "        << (cfg.aim_range_circle ? "true" : "false") << ",\n";
    f << "  \"kp\": "                      << cfg.kp << ",\n";
     f << "  \"kd\": "                      << cfg.kd << ",\n";
    f << "  \"ki\": "                      << cfg.ki << ",\n";
    f << "  \"kalman_prediction_steps\": " << cfg.kalman_prediction_steps << ",\n";
    f << "  \"sensitivity\": "             << cfg.sensitivity << ",\n";
    f << "  \"aim_key\": "                 << cfg.aim_key << ",\n";
    f << "  \"exit_key\": "                << cfg.exit_key << ",\n";
    f << "  \"switch_target_key\": "       << cfg.switch_target_key << ",\n";
    f << "  \"start_stop_key\": "          << cfg.start_stop_key << ",\n";
    f << "  \"mouse_backend\": \""         << MouseBackendToString(cfg.mouse_backend) << "\",\n";
    f << "  \"kmbox_ip\": \""              << EscapeJson(cfg.kmbox_ip) << "\",\n";
    f << "  \"kmbox_port\": \""            << EscapeJson(cfg.kmbox_port) << "\",\n";
    f << "  \"kmbox_mac\": \""             << EscapeJson(cfg.kmbox_mac) << "\",\n";
    f << "  \"kmbox_encrypt\": "           << (cfg.kmbox_encrypt ? "true" : "false") << ",\n";
    f << "  \"kmbox_bezier\": "            << (cfg.kmbox_bezier ? "true" : "false") << ",\n";
     f << "  \"makcu_serial\": \""           << EscapeJson(cfg.makcu_serial) << "\",\n";
     f << "  \"makcu_port\": \""             << EscapeJson(cfg.makcu_port) << "\",\n";
    f << "  \"enable_visualization\": "    << (cfg.enable_visualization ? "true" : "false") << ",\n";
    f << "  \"show_infer_latency\": "      << (cfg.show_infer_latency ? "true" : "false") << ",\n";
    f << "  \"draw_detection_boxes\": "    << (cfg.draw_detection_boxes ? "true" : "false") << ",\n";
    f << "  \"draw_aim_on_preview\": "    << (cfg.draw_aim_on_preview ? "true" : "false") << ",\n";
    f << "  \"debug_window_enabled\": "    << (cfg.debug_window_enabled ? "true" : "false") << ",\n";
    f << "  \"window_name\": \""           << EscapeJson(cfg.window_name) << "\",\n";
    f << "  \"auto_start\": "              << (cfg.auto_start ? "true" : "false") << ",\n";
    f << "  \"minimize_to_tray\": "        << (cfg.minimize_to_tray ? "true" : "false") << ",\n";
    f << "  \"language\": \""              << EscapeJson(cfg.language) << "\"\n";
    f << "}\n";

    std::cout << "[Config] Saved to " << filepath << std::endl;
    return true;
}
