#pragma once

#include "object_detector.hpp"
#include <unordered_set>

// ============================================================
// TargetSelector — v2.0 MVP
// 选距画面中心最近、符合类别的目标
// ============================================================
class TargetSelector {
public:
    TargetSelector() = default;

    // 选择最优目标：距画面中心最近 + 类别匹配（支持多类）
    // 返回 nullptr 表示没有有效目标
    const Detection* SelectBest(const std::vector<Detection>& detections,
                                float frameCenterX, float frameCenterY,
                                const std::vector<int>& targetClassIds) const;

    // 将模型坐标系坐标映射回画面坐标系
    // modelSize: 模型输入尺寸, frameSize: 实际画面尺寸
    void MapToFrame(const Detection& src, Detection& dst,
                    int modelSize, int frameSize) const;
};
