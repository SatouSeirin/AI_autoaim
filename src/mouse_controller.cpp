#include "mouse_controller.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include "kmboxNet.h"

// kmboxNet.cpp 中定义但头文件未声明的函数
extern int kmNet_Trace(int type, int value);

// kmboxNet.cpp 中 extern 声明但未定义的变量
unsigned int xbox_mac = 0;

// ============================================================
// 构造/析构
// ============================================================
MouseController::MouseController() {}

MouseController::~MouseController() {
    if (kmbox_connected_) {
        Disconnect();
    }
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

    case MouseBackend::KMBoxNet:
        std::cout << "[Mouse] Backend: KMBoxNet (hardware), use InitKMBox() to connect" << std::endl;
        return true;

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
    case MouseBackend::KMBoxNet:
        MoveByKmboxNet(dx, dy);
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
    case MouseBackend::SendInput:        return "SendInput";
    case MouseBackend::IbInputSimulator: return "IbInputSimulator";
    case MouseBackend::KMBoxNet:         return "KMBoxNet";
    default:                             return "Unknown";
    }
}

// ============================================================
// KMBoxNet 硬件后端
// ============================================================

bool MouseController::InitKMBox(const std::string& ip, const std::string& port,
                                 const std::string& mac, bool encrypt) {
    if (kmbox_connected_) {
        Disconnect();
    }

    backend_ = MouseBackend::KMBoxNet;
    kmbox_encrypt_ = encrypt;

    // kmNet_init 需要 char* (非 const)
    char ipBuf[64], portBuf[16], macBuf[32];
    strncpy(ipBuf, ip.c_str(), sizeof(ipBuf) - 1);   ipBuf[sizeof(ipBuf) - 1] = '\0';
    strncpy(portBuf, port.c_str(), sizeof(portBuf) - 1); portBuf[sizeof(portBuf) - 1] = '\0';
    strncpy(macBuf, mac.c_str(), sizeof(macBuf) - 1); macBuf[sizeof(macBuf) - 1] = '\0';

    int ret = kmNet_init(ipBuf, portBuf, macBuf);
    if (ret == 0) {
        kmbox_connected_ = true;

        // 关闭硬件曲线修正（避免固件内部拉长移动时间）
        kmNet_Trace(0, 0);

        std::cout << "[Mouse] KMBoxNet connected: " << ip << ":" << port
                  << " MAC=" << mac << " encrypt=" << (encrypt ? "ON" : "OFF") << std::endl;
    } else {
        kmbox_connected_ = false;
        std::cerr << "[Mouse] KMBoxNet connect FAILED: error=" << ret << std::endl;
    }
    return kmbox_connected_;
}

void MouseController::Disconnect() {
    if (kmbox_connected_) {
        // SDK 没有显式 disconnect 函数，关闭 socket 即可
        if (sockClientfd > 0) {
            closesocket(sockClientfd);
            sockClientfd = 0;
        }
        kmbox_connected_ = false;
        std::cout << "[Mouse] KMBoxNet disconnected" << std::endl;
    }
}

bool MouseController::IsConnected() const {
    return kmbox_connected_;
}

void MouseController::MoveByKmboxNet(double dx, double dy) {
    if (!kmbox_connected_) return;

    // 1000Hz 全速发送，每帧都发，与 SendInput 行为一致
    short mx = static_cast<short>(std::clamp(dx, -32767.0, 32767.0));
    short my = static_cast<short>(std::clamp(dy, -32767.0, 32767.0));
    if (mx == 0 && my == 0) return;

    kmNet_enc_mouse_move(mx, my);
}
