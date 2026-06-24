#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// ============================================================
// HotkeyManager — v2.1 全局热键管理器
// 使用 RegisterHotKey 注册系统级热键，Qt nativeEvent 接收
// ============================================================

// 虚拟键码 → 可读名称
inline std::string VkToString(int vk) {
    switch (vk) {
    // 鼠标
    case VK_LBUTTON:    return "鼠标左键";
    case VK_RBUTTON:    return "鼠标右键";
    case VK_MBUTTON:    return "鼠标中键";
    case VK_XBUTTON1:   return "鼠标侧键1";
    case VK_XBUTTON2:   return "鼠标侧键2";
    // 功能键
    case VK_F1:  return "F1";   case VK_F2:  return "F2";
    case VK_F3:  return "F3";   case VK_F4:  return "F4";
    case VK_F5:  return "F5";   case VK_F6:  return "F6";
    case VK_F7:  return "F7";   case VK_F8:  return "F8";
    case VK_F9:  return "F9";   case VK_F10: return "F10";
    case VK_F11: return "F11";  case VK_F12: return "F12";
    // 特殊键
    case VK_SHIFT:    return "Shift";
    case VK_CONTROL:  return "Ctrl";
    case VK_MENU:     return "Alt";
    case VK_TAB:      return "Tab";
    case VK_CAPITAL:  return "CapsLock";
    case VK_SPACE:    return "空格";
    case VK_RETURN:   return "回车";
    case VK_BACK:     return "退格";
    case VK_ESCAPE:   return "Esc";
    case VK_DELETE:   return "Delete";
    case VK_INSERT:   return "Insert";
    case VK_HOME:     return "Home";
    case VK_END:      return "End";
    case VK_PRIOR:    return "PageUp";
    case VK_NEXT:     return "PageDown";
    case VK_UP:       return "↑";
    case VK_DOWN:     return "↓";
    case VK_LEFT:     return "←";
    case VK_RIGHT:    return "→";
    case VK_SNAPSHOT: return "PrintScreen";
    case VK_SCROLL:   return "ScrollLock";
    case VK_PAUSE:    return "Pause";
    case VK_NUMLOCK:  return "NumLock";
    // 数字
    case 0x30: return "0"; case 0x31: return "1";
    case 0x32: return "2"; case 0x33: return "3";
    case 0x34: return "4"; case 0x35: return "5";
    case 0x36: return "6"; case 0x37: return "7";
    case 0x38: return "8"; case 0x39: return "9";
    // 字母
    case 0x41: return "A"; case 0x42: return "B";
    case 0x43: return "C"; case 0x44: return "D";
    case 0x45: return "E"; case 0x46: return "F";
    case 0x47: return "G"; case 0x48: return "H";
    case 0x49: return "I"; case 0x4A: return "J";
    case 0x4B: return "K"; case 0x4C: return "L";
    case 0x4D: return "M"; case 0x4E: return "N";
    case 0x4F: return "O"; case 0x50: return "P";
    case 0x51: return "Q"; case 0x52: return "R";
    case 0x53: return "S"; case 0x54: return "T";
    case 0x55: return "U"; case 0x56: return "V";
    case 0x57: return "W"; case 0x58: return "X";
    case 0x59: return "Y"; case 0x5A: return "Z";
    // 小键盘
    case VK_NUMPAD0: return "Num0"; case VK_NUMPAD1: return "Num1";
    case VK_NUMPAD2: return "Num2"; case VK_NUMPAD3: return "Num3";
    case VK_NUMPAD4: return "Num4"; case VK_NUMPAD5: return "Num5";
    case VK_NUMPAD6: return "Num6"; case VK_NUMPAD7: return "Num7";
    case VK_NUMPAD8: return "Num8"; case VK_NUMPAD9: return "Num9";
    case VK_MULTIPLY: return "Num*"; case VK_ADD:      return "Num+";
    case VK_SUBTRACT: return "Num-"; case VK_DECIMAL:   return "Num.";
    case VK_DIVIDE:   return "Num/";
    default: return "VK_" + std::to_string(vk);
    }
}

// ============================================================
// HotkeyManager 全局热键管理器
// 线程安全，管理多个系统级热键注册
// ============================================================
class HotkeyManager {
public:
    using Callback = std::function<void(int hotkeyId)>;

    HotkeyManager()  = default;
    ~HotkeyManager();

    // 禁拷贝
    HotkeyManager(const HotkeyManager&) = delete;
    HotkeyManager& operator=(const HotkeyManager&) = delete;

    // 注册热键（每个热键需要一个唯一 id）
    // modifiers: MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT
    bool Register(int hotkeyId, UINT modifiers, UINT vk, Callback cb);

    // 注销热键
    void Unregister(int hotkeyId);

    // 注销所有热键
    void UnregisterAll();

    // 由 nativeEvent 调用，处理 WM_HOTKEY
    // 返回 true 表示已处理
    bool HandleMessage(MSG* msg);

private:
    struct Entry {
        int      id;
        UINT     modifiers;
        UINT     vk;
        Callback callback;
        bool     registered = false;
    };
    std::vector<Entry> entries_;
};
