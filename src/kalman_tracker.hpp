#pragma once

#include <cmath>
#include <cstring>

// ============================================================
// KalmanTracker — 恒定速度卡尔曼滤波器
//
// 状态: [x, y, vx, vy]  (归一化坐标 [0,1])
// 测量: [x, y]
//
// 用途:
//   1. 平滑目标轨迹
//   2. 预测未来位置（prediction_steps 步前向预测）
//   3. 目标短暂丢失时惯性保持
//
// 暴露5个可调参数：
//   process_noise    — 过程噪声（信任预测程度）
//   measure_noise    — 测量噪声（信任检测程度）
//   prediction_steps — 预测步数（0=禁用预测）
//   error_cov_init   — 初始误差协方差
//   velocity_damping — 速度阻尼（每帧速度衰减）
// ============================================================
class KalmanTracker {
public:
    KalmanTracker();

    // 重置
    void Reset();

    // 一次性配置所有卡尔曼参数
    // 更新（检测结果 + 帧时间差）
    void Update(float cx, float cy, float confidence, float dt);

    // 前向预测（考虑 velocity_damping）
    // steps: 预测步数（通常 = prediction_steps 配置）
    // dt: 单步时间（约 0.033s）
    void PredictAhead(int steps, float dt, float& outX, float& outY) const;

    float GetX()  const { return state_[0]; }
    float GetY()  const { return state_[1]; }
    float GetVx() const { return state_[2]; }
    float GetVy() const { return state_[3]; }

    float GetSpeed() const;

    float GetUncertainty() const;
    bool IsInitialized() const { return initialized_; }
    int GetLostCount() const { return lost_count_; }
    void MarkLost();

private:
    void Predict(float dt);
    void Correct(float cx, float cy);

    void RebuildQ(float dt);

    float state_[4];           // [x, y, vx, vy]
    float P_[16];              // 协方差 4x4
    float Q_[16];              // 过程噪声 4x4
    float R_[4];               // 测量噪声 2x2
    float F_[16];              // 状态转移 4x4
    float K_[8];               // 卡尔曼增益 4x2

    bool initialized_ = false;
    int  lost_count_  = 0;
    float last_dt_ = 0.016f;

    // 可调参数（由 Configure() 设置）
    float process_noise_    = 0.1f;
    float measure_noise_    = 0.01f;
    float error_cov_init_   = 1.0f;
    float velocity_damping_ = 0.98f;

    static constexpr float STABILITY_EPSILON = 1e-6f;
    static constexpr float MIN_DT = 0.001f;
    static constexpr float MAX_DT = 0.1f;
};
