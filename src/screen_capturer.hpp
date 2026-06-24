#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

// ============================================================
// DXGI ScreenCapturer — v2.0 MVP
// 截取屏幕中心 crop_size×crop_size 区域
// ============================================================
class ScreenCapturer {
public:
    ScreenCapturer();
    ~ScreenCapturer();

    bool Initialize(int capture_size);
    void Shutdown();

    // 捕获一帧，返回 BGRA 格式 cv::Mat（引用传入避免拷贝）
    bool Capture(cv::Mat& outFrame);

    int  Width()  const { return capture_size_; }
    int  Height() const { return capture_size_; }
    bool IsInitialized() const { return initialized_; }

private:
    bool CreateDXGIFactory();
    bool GetOutputAndDevice();
    bool CreateDuplication();

    IDXGIFactory1*           dxgi_factory_   = nullptr;
    IDXGIOutput*             dxgi_output_    = nullptr;
    ID3D11Device*            d3d11_device_   = nullptr;
    ID3D11DeviceContext*     d3d11_context_  = nullptr;
    IDXGIOutputDuplication*  duplication_    = nullptr;

    int  capture_size_ = 640;
    int  screen_width_ = 1920;
    int  screen_height_ = 1080;
    bool initialized_  = false;
};
