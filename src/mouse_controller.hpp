#pragma once

#include <windows.h>
#include <string>

#include "config.hpp"

// ============================================================
// MouseController — v2.0 MVP
// 仅支持 SendInput + IbInputSimulator 两个后端
// 统一 MoveMouse(dx, dy) 接口
// KMBox 硬件后端留到 v2.2 扩展
// ============================================================
class MouseController {
public:
    MouseController();
    ~MouseController();

    // 初始化：选择鼠标后端
    bool Init(MouseBackend backend);

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

    MouseBackend backend_        = MouseBackend::SendInput;
    HMODULE      ib_dll_         = nullptr;
    bool         ib_initialized_ = false;

    // IbInputSimulator 函数指针
    using FnInit  = bool(*)();
    using FnMove  = void(*)(int, int);
    using FnClick = void(*)(int);
    FnInit  ib_init_  = nullptr;
    FnMove  ib_move_  = nullptr;
    FnClick ib_click_ = nullptr;
};
