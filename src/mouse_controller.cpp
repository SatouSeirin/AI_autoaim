#include "mouse_controller.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

// ============================================================
// 构造/析构
// ============================================================
MouseController::MouseController() {}

MouseController::~MouseController() {
    if (ib_dll_) {
        FreeLibrary(ib_dll_);
        ib_dll_ = nullptr;
    }
}

// ============================================================
// 初始化后端
// ============================================================
bool MouseController::Init(MouseBackend backend) {
    backend_ = backend;

    switch (backend) {
    case MouseBackend::SendInput:
        std::cout << "[Mouse] Backend: SendInput (Windows API)" << std::endl;
        return true;

    case MouseBackend::IbInputSimulator:
        ib_dll_ = LoadLibraryA("IbInputSimulator.dll");
        if (!ib_dll_) {
            std::cerr << "[Mouse] IbInputSimulator.dll not found!" << std::endl;
            return false;
        }
        ib_init_  = reinterpret_cast<FnInit>(GetProcAddress(ib_dll_, "Initialize"));
        ib_move_  = reinterpret_cast<FnMove>(GetProcAddress(ib_dll_, "MoveMouseRelative"));
        if (!ib_init_ || !ib_move_) {
            std::cerr << "[Mouse] Failed to get function pointers from IbInputSimulator.dll"
                      << std::endl;
            FreeLibrary(ib_dll_);
            ib_dll_ = nullptr;
            return false;
        }
        ib_initialized_ = ib_init_();
        std::cout << "[Mouse] Backend: IbInputSimulator (driver-level), init="
                  << (ib_initialized_ ? "OK" : "FAIL") << std::endl;
        return ib_initialized_;

    default:
        std::cerr << "[Mouse] Unknown backend" << std::endl;
        return false;
    }
}

// ============================================================
// 统一移动接口
// ============================================================
void MouseController::MoveMouse(double deltaX, double deltaY, double sensitivity) {
    double dx = deltaX * sensitivity;
    double dy = deltaY * sensitivity;

    // 限制最大单帧移动量（防止跳跃）
    double maxMove = 500.0;
    dx = std::max(-maxMove, std::min(maxMove, dx));
    dy = std::max(-maxMove, std::min(maxMove, dy));

    switch (backend_) {
    case MouseBackend::SendInput:
        MoveBySendInput(dx, dy);
        break;
    case MouseBackend::IbInputSimulator:
        MoveByIbInput(dx, dy);
        break;
    }
}

// ============================================================
// SendInput 后端实现
// ============================================================
void MouseController::MoveBySendInput(double dx, double dy) {
    INPUT input = {};
    input.type       = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx      = static_cast<LONG>(dx);
    input.mi.dy      = static_cast<LONG>(dy);
    SendInput(1, &input, sizeof(INPUT));
}

// ============================================================
// IbInputSimulator 驱动级后端
// ============================================================
void MouseController::MoveByIbInput(double dx, double dy) {
    if (ib_move_) {
        ib_move_(static_cast<int>(dx), static_cast<int>(dy));
    } else {
        // 降级到 SendInput
        MoveBySendInput(dx, dy);
    }
}

std::string MouseController::GetBackendName() const {
    switch (backend_) {
    case MouseBackend::SendInput:       return "SendInput";
    case MouseBackend::IbInputSimulator: return "IbInputSimulator";
    default:                             return "Unknown";
    }
}
