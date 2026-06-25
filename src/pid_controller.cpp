#include "pid_controller.hpp"
#include <algorithm>
#include <cmath>

PIDController::PIDController() {}

void PIDController::SetGains(double kp, double ki, double kd) {
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
}

void PIDController::SetIntegralLimit(double limit) {
    integral_limit_ = limit;
}

void PIDController::Reset() {
    integral_          = 0.0;
    last_measurement_   = 0.0;
    last_derivative_    = 0.0;
    filtered_derivative_ = 0.0;
    first_run_         = true;
}

double PIDController::Compute(double error, double dt) {
    if (dt <= 0.0) dt = 0.001;
    if (dt > 0.1)  dt = 0.1;

    double pTerm = kp_ * error;

    integral_ += ki_ * error * dt;
    integral_ = std::clamp(integral_, -integral_limit_, integral_limit_);
    double iTerm = integral_;

    double dTerm = 0.0;
    if (!first_run_) {
        double measurementDelta = error - last_measurement_;
        double rawDerivative = measurementDelta / dt;
        filtered_derivative_ = (1.0 - DERIVATIVE_LP_ALPHA) * filtered_derivative_
                             + DERIVATIVE_LP_ALPHA * rawDerivative;
        last_derivative_ = filtered_derivative_;
        dTerm = kd_ * filtered_derivative_;
    }
    last_measurement_ = error;
    first_run_ = false;

    double output = pTerm + iTerm + dTerm;
    return std::clamp(output, -max_output_, max_output_);
}
