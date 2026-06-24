#pragma once

#include "object_detector.hpp"
#include <unordered_set>

// ============================================================
// TargetSelector — v2.1
// 在 FOV 范围内选距画面中心最近、符合类别的目标
// ============================================================
class TargetSelector {
public:
    TargetSelector() = default;

    // 选择最优目标：在 FOV 范围内，选距画面中心最近 + 类别匹配
    // frameCenterX/Y: 画面中心坐标（模型输入空间）
    // fovRadius: FOV 半径（模型输入空间），0=不过滤
    // 返回 nullptr 表示没有有效目标
    const Detection* SelectBest(const std::vector<Detection>& detections,
                                float frameCenterX, float frameCenterY,
                                const std::vector<int>& targetClassIds,
                                float fovRadius = 0.0f) const;

    // 判断目标是否在 FOV 范围内
    bool IsInFOV(const Detection& det, float centerX, float centerY,
                 float fovRadius) const;

    // 将模型坐标系坐标映射回画面坐标系
    // modelSize: 模型输入尺寸, frameSize: 实际画面尺寸
    void MapToFrame(const Detection& src, Detection& dst,
                    int modelSize, int frameSize) const;
};
