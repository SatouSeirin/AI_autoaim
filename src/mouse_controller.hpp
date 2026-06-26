#pragma once

// 阻止 windows.h 包含旧版 winsock.h，避免与 kmboxNet.h 中的 Winsock2.h 冲突
#define _WINSOCKAPI_
#include <windows.h>
#include <string>

#include "config.hpp"

// ============================================================
// MouseController — v2.1
// 支持 SendInput + IbInputSimulator + KMBoxNet 三个后端
// 统一 MoveMouse(dx, dy) 接口
// ============================================================
class MouseController {
public:
    MouseController();
    ~MouseController();

    // 初始化：选择鼠标后端
    bool Init(MouseBackend backend);

    // KMBoxNet 专用连接（传入 IP/端口/MAC）
    bool InitKMBox(const std::string& ip, const std::string& port, const std::string& mac, bool encrypt);

    // 断开 KMBox 连接
    void Disconnect();

    // 连接状态查询
    bool IsConnected() const;

    // 统一移动接口（Δx, Δy 为屏幕像素偏移）
    void MoveMouse(double deltaX, double deltaY, double sensitivity = 1.0);

    // 当前后端
    MouseBackend GetBackend() const { return backend_; }
    std::string  GetBackendName() const;

private:
    // SendInput 后端
    void MoveBySendInput(double dx, double dy);

    // IbInputSimulator 驱动级后端
    void MoveByIbInput(double dx, double dy);

    // KMBoxNet 硬件后端
    void MoveByKmboxNet(double dx, double dy);

    MouseBackend backend_        = MouseBackend::SendInput;
    HMODULE      ib_dll_         = nullptr;
    bool         ib_initialized_ = false;

    // KMBoxNet 状态
    bool kmbox_connected_ = false;
    bool kmbox_encrypt_   = false;

    // IbInputSimulator 函数指针
    using FnInit  = bool(*)();
    using FnMove  = void(*)(int, int);
    using FnClick = void(*)(int);
    FnInit  ib_init_  = nullptr;
    FnMove  ib_move_  = nullptr;
    FnClick ib_click_ = nullptr;
};
