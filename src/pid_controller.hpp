#pragma once

#include <cmath>
#include <algorithm>

// ============================================================
// PIDController — 标准 PID 控制器
//
//   output = Kp·e + Ki·∫e·dt + Kd·de/dt
//
//   - 比例(P): 快速响应误差
//   - 积分(I): 消除稳态偏差，带 anti-windup 钳位
//   - 微分(D): 基于测量值的微分（避免微分冲击）+ 低通滤波
//   - max_output 限制单次最大输出
// ============================================================
class PIDController {
public:
    PIDController();

    void SetGains(double kp, double ki, double kd);
    void SetIntegralLimit(double limit);
    void Reset();

    // error: 像素偏差, dt: 时间步长(秒)
    double Compute(double error, double dt);

    double GetIntegral()     const { return integral_; }
    double GetDerivative()   const { return last_derivative_; }

private:
    double kp_ = 0.250;
    double ki_ = 0.0;
    double kd_ = 0.08;
    double integral_limit_ = 100.0;  // 积分上限，防饱和
    double max_output_ = 50.0;       // 单次最大输出（像素）
    double integral_      = 0.0;
    double last_measurement_ = 0.0;
    double last_derivative_  = 0.0;
    double filtered_derivative_ = 0.0;
    bool   first_run_     = true;

    // 微分低通滤波
    static constexpr double DERIVATIVE_LP_ALPHA = 0.3;
};
