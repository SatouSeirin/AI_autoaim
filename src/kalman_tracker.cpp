#include "kalman_tracker.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

KalmanTracker::KalmanTracker() {
    Reset();
}

void KalmanTracker::Reset() {
    std::memset(state_, 0, sizeof(state_));
    std::memset(P_, 0, sizeof(P_));
    std::memset(K_, 0, sizeof(K_));

    P_[0] = P_[5]  = error_cov_init_;    // position covariance
    P_[10] = P_[15] = error_cov_init_;   // velocity covariance

    R_[0] = R_[3] = measure_noise_;

    initialized_ = false;
    lost_count_  = 0;
    last_dt_     = 0.016f;
}

void KalmanTracker::RebuildQ(float dt) {
    std::memset(Q_, 0, sizeof(Q_));
    float pn = process_noise_ * dt;
    Q_[0]  = Q_[5]  = pn;
    Q_[10] = Q_[15] = pn;
}

void KalmanTracker::Update(float cx, float cy, float confidence, float dt) {
    (void)confidence;
    if (dt <= 0.0f) dt = MIN_DT;
    if (dt > MAX_DT) dt = MAX_DT;
    last_dt_ = dt;

    if (!initialized_) {
        state_[0] = cx;
        state_[1] = cy;
        state_[2] = 0.0f;
        state_[3] = 0.0f;
        P_[0] = P_[5]  = error_cov_init_ * 0.01f;
        P_[10] = P_[15] = error_cov_init_ * 0.1f;
        initialized_ = true;
        lost_count_  = 0;
        return;
    }

    Predict(dt);
    Correct(cx, cy);
    lost_count_ = 0;
}

void KalmanTracker::Predict(float dt) {
    // State transition with velocity damping
    state_[0] += state_[2] * dt;
    state_[1] += state_[3] * dt;
    state_[2] *= velocity_damping_;
    state_[3] *= velocity_damping_;

    // F matrix
    std::memset(F_, 0, sizeof(F_));
    F_[0]  = 1.0f; F_[2]  = dt;
    F_[5]  = 1.0f; F_[7]  = dt;
    F_[10] = 1.0f;
    F_[15] = 1.0f;

    // FP = F * P
    float FP[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            FP[i*4+j] = 0.0f;
            for (int k = 0; k < 4; k++)
                FP[i*4+j] += F_[i*4+k] * P_[k*4+j];
        }
    }

    RebuildQ(dt);

    // P = FP * F^T + Q
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            P_[i*4+j] = Q_[i*4+j];
            for (int k = 0; k < 4; k++)
                P_[i*4+j] += FP[i*4+k] * F_[j*4+k];
        }
    }
}

void KalmanTracker::Correct(float cx, float cy) {
    R_[0] = R_[3] = measure_noise_;

    float y0 = cx - state_[0];
    float y1 = cy - state_[1];

    float S00 = P_[0] + R_[0];
    float S01 = P_[4];
    float S10 = P_[1];
    float S11 = P_[5] + R_[3];

    float detS = S00 * S11 - S01 * S10;
    if (std::abs(detS) < STABILITY_EPSILON)
        detS = STABILITY_EPSILON;
    float invDet = 1.0f / detS;

    float K00 = (P_[0]*S11 - P_[4]*S10) * invDet;
    float K01 = (P_[4]*S00 - P_[0]*S01) * invDet;
    float K10 = (P_[1]*S11 - P_[5]*S10) * invDet;
    float K11 = (P_[5]*S00 - P_[1]*S01) * invDet;
    float K20 = (P_[2]*S11 - P_[6]*S10) * invDet;
    float K21 = (P_[6]*S00 - P_[2]*S01) * invDet;
    float K30 = (P_[3]*S11 - P_[7]*S10) * invDet;
    float K31 = (P_[7]*S00 - P_[3]*S01) * invDet;

    state_[0] += K00 * y0 + K01 * y1;
    state_[1] += K10 * y0 + K11 * y1;
    state_[2] += K20 * y0 + K21 * y1;
    state_[3] += K30 * y0 + K31 * y1;

    float t00 = 1.0f - K00, t01 = -K01;
    float t10 = -K10,       t11 = 1.0f - K11;
    float t20 = -K20,       t21 = -K21;
    float t30 = -K30,       t31 = -K31;

    float newP[16];
    for (int j = 0; j < 4; j++) {
        float c0 = P_[j], c1 = P_[4+j];
        newP[0*4+j] = t00*c0 + t01*c1;
        newP[1*4+j] = t10*c0 + t11*c1;
        newP[2*4+j] = t20*c0 + t21*c1 + P_[8+j];
        newP[3*4+j] = t30*c0 + t31*c1 + P_[12+j];
    }
    std::memcpy(P_, newP, sizeof(P_));

    K_[0]=K00; K_[1]=K01; K_[2]=K10; K_[3]=K11;
    K_[4]=K20; K_[5]=K21; K_[6]=K30; K_[7]=K31;
}

void KalmanTracker::PredictAhead(int steps, float dt, float& outX, float& outY) const {
    if (!initialized_) {
        outX = 0.5f;
        outY = 0.5f;
        return;
    }
    float px = state_[0], py = state_[1];
    float vx = state_[2], vy = state_[3];

    for (int i = 0; i < steps; i++) {
        px += vx * dt;
        py += vy * dt;
        vx *= velocity_damping_;
        vy *= velocity_damping_;
    }

    outX = std::max(0.0f, std::min(1.0f, px));
    outY = std::max(0.0f, std::min(1.0f, py));
}

float KalmanTracker::GetSpeed() const {
    return std::sqrt(state_[2] * state_[2] + state_[3] * state_[3]);
}

float KalmanTracker::GetUncertainty() const {
    return P_[0] + P_[5] + P_[10] + P_[15];
}

void KalmanTracker::MarkLost() {
    lost_count_++;
    if (lost_count_ > 3) {
        P_[0]  += 0.02f;
        P_[5]  += 0.02f;
        P_[10] += 0.05f;
        P_[15] += 0.05f;
    }
}
