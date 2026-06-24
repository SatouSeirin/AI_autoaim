#include "hotkey_manager.hpp"
#include <iostream>

HotkeyManager::~HotkeyManager() {
    UnregisterAll();
}

bool HotkeyManager::Register(int hotkeyId, UINT modifiers, UINT vk, Callback cb) {
    // 如果已注册过同 id，先取消
    Unregister(hotkeyId);

    if (!RegisterHotKey(nullptr, hotkeyId, modifiers, vk)) {
        std::cerr << "[Hotkey] RegisterHotKey failed for id=" << hotkeyId
                  << " vk=0x" << std::hex << vk << std::dec << std::endl;
        return false;
    }

    Entry entry;
    entry.id         = hotkeyId;
    entry.modifiers  = modifiers;
    entry.vk         = vk;
    entry.callback   = std::move(cb);
    entry.registered = true;
    entries_.push_back(std::move(entry));

    std::cout << "[Hotkey] Registered id=" << hotkeyId
              << " key=" << VkToString(vk) << std::endl;
    return true;
}

void HotkeyManager::Unregister(int hotkeyId) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->id == hotkeyId) {
            if (it->registered) {
                UnregisterHotKey(nullptr, hotkeyId);
            }
            entries_.erase(it);
            return;
        }
    }
}

void HotkeyManager::UnregisterAll() {
    for (auto& e : entries_) {
        if (e.registered) {
            UnregisterHotKey(nullptr, e.id);
        }
    }
    entries_.clear();
    std::cout << "[Hotkey] All hotkeys unregistered" << std::endl;
}

bool HotkeyManager::HandleMessage(MSG* msg) {
    if (msg->message != WM_HOTKEY) return false;

    int id = static_cast<int>(msg->wParam);
    for (const auto& e : entries_) {
        if (e.id == id && e.callback) {
            e.callback(id);
            return true;
        }
    }
    return false;
}
