#pragma once

#include "config.hpp"
#include <string>

// ============================================================
// ConfigManager — v2.1 配置文件持久化 (JSON)
// 自动保存/加载到 config.json
// ============================================================
class ConfigManager {
public:
    // 加载配置，文件不存在则返回默认值
    static AimConfig Load(const std::string& filepath = "config.json");

    // 保存配置
    static bool Save(const AimConfig& config, const std::string& filepath = "config.json");

private:
    static std::string EscapeJson(const std::string& s);
    static std::string MouseBackendToString(MouseBackend b);
    static MouseBackend StringToMouseBackend(const std::string& s);
};
