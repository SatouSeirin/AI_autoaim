#include "target_selector.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

const Detection* TargetSelector::SelectBest(const std::vector<Detection>& detections,
                                             float frameCenterX, float frameCenterY,
                                             const std::vector<int>& targetClassIds,
                                             float fovRadius) const {
    const Detection* best = nullptr;
    float bestDist = 1e9f;

    // 构建快速查找集合
    std::unordered_set<int> clsSet(targetClassIds.begin(), targetClassIds.end());

    // 诊断：每 300 帧打印一次
    static int diagCount = 0;
    bool doDiag = (++diagCount % 300 == 0);

    for (const auto& det : detections) {
        if (clsSet.find(det.class_id) == clsSet.end()) continue;

        // det 坐标是归一化的 [0,1]，frameCenter 也必须是归一化的
        float dx = det.center_x() - frameCenterX;
        float dy = det.center_y() - frameCenterY;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (doDiag) {
            std::cout << "[Selector] det cls=" << det.class_id
                      << " cx=" << det.center_x() << " cy=" << det.center_y()
                      << " fcX=" << frameCenterX << " fcY=" << frameCenterY
                      << " dx=" << dx << " dy=" << dy
                      << " dist=" << dist << " fovR=" << fovRadius << std::endl;
        }

        // FOV 范围过滤（fovRadius > 0 时生效）
        if (fovRadius > 0.0f && dist > fovRadius) {
            if (doDiag) std::cout << "[Selector]   -> OUT of FOV" << std::endl;
            continue;
        }

        if (dist < bestDist) {
            bestDist = dist;
            best = &det;
        }
    }

    if (doDiag) {
        if (best) std::cout << "[Selector] BEST: cls=" << best->class_id
                           << " dist=" << bestDist << std::endl;
        else std::cout << "[Selector] NO target in FOV" << std::endl;
    }

    return best;
}

bool TargetSelector::IsInFOV(const Detection& det, float centerX, float centerY,
                              float fovRadius) const {
    if (fovRadius <= 0.0f) return true;
    float dx = det.center_x() - centerX;
    float dy = det.center_y() - centerY;
    return std::sqrt(dx * dx + dy * dy) <= fovRadius;
}

void TargetSelector::MapToFrame(const Detection& src, Detection& dst,
                                 int modelSize, int frameSize) const {
    float scale = static_cast<float>(frameSize) / modelSize;
    dst.x1         = src.x1 * scale;
    dst.y1         = src.y1 * scale;
    dst.x2         = src.x2 * scale;
    dst.y2         = src.y2 * scale;
    dst.confidence = src.confidence;
    dst.class_id   = src.class_id;
}
