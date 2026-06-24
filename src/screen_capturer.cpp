#include "screen_capturer.hpp"
#include <iostream>
#include <algorithm>

ScreenCapturer::ScreenCapturer() {}

ScreenCapturer::~ScreenCapturer() {
    Shutdown();
}

bool ScreenCapturer::Initialize(int captureSize) {
    capture_size_ = captureSize;

    if (!CreateDXGIFactory()) {
        std::cerr << "[ScreenCapturer] CreateDXGIFactory failed" << std::endl;
        return false;
    }
    if (!GetOutputAndDevice()) {
        std::cerr << "[ScreenCapturer] GetOutputAndDevice failed" << std::endl;
        return false;
    }
    if (!CreateDuplication()) {
        std::cerr << "[ScreenCapturer] CreateDuplication failed" << std::endl;
        return false;
    }

    initialized_ = true;
    std::cout << "[ScreenCapturer] Initialized: " << capture_size_
              << "x" << capture_size_ << " on "
              << screen_width_ << "x" << screen_height_ << " screen" << std::endl;
    return true;
}

void ScreenCapturer::Shutdown() {
    if (duplication_)   { duplication_->Release();   duplication_   = nullptr; }
    if (d3d11_context_) { d3d11_context_->Release(); d3d11_context_ = nullptr; }
    if (d3d11_device_)  { d3d11_device_->Release();  d3d11_device_  = nullptr; }
    if (dxgi_output_)   { dxgi_output_->Release();   dxgi_output_   = nullptr; }
    if (dxgi_factory_)  { dxgi_factory_->Release();  dxgi_factory_  = nullptr; }
    initialized_ = false;
}

bool ScreenCapturer::Capture(cv::Mat& outFrame) {
    if (!initialized_ || !duplication_) return false;

    IDXGIResource*          desktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;

    // 获取桌面帧（超时 20ms）
    HRESULT hr = duplication_->AcquireNextFrame(20, &frameInfo, &desktopResource);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;
        // 桌面模式变化（锁屏/UAC 等）需重建
        duplication_->ReleaseFrame();
        duplication_->Release();
        duplication_ = nullptr;
        CreateDuplication();
        return false;
    }

    // 获取 D3D11 纹理
    ID3D11Texture2D* texture = nullptr;
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D),
                                          reinterpret_cast<void**>(&texture));
    desktopResource->Release();
    if (FAILED(hr) || !texture) {
        duplication_->ReleaseFrame();
        return false;
    }

    // 获取纹理描述
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);

    // 屏幕中心区域坐标
    int centerX = screen_width_  / 2;
    int centerY = screen_height_ / 2;
    int half    = capture_size_  / 2;
    int srcLeft  = (std::max)(0, centerX - half);
    int srcTop   = (std::max)(0, centerY - half);
    int srcRight  = (std::min)(screen_width_,  centerX + half);
    int srcBottom = (std::min)(screen_height_, centerY + half);

    // 创建 staging texture 用于 CPU 读取
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width              = srcRight - srcLeft;
    stagingDesc.Height             = srcBottom - srcTop;
    stagingDesc.MipLevels          = 1;
    stagingDesc.ArraySize          = 1;
    stagingDesc.Format             = desc.Format;
    stagingDesc.SampleDesc.Count   = 1;
    stagingDesc.Usage              = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* stagingTexture = nullptr;
    hr = d3d11_device_->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
    if (FAILED(hr)) {
        texture->Release();
        duplication_->ReleaseFrame();
        return false;
    }

    // 拷贝子区域
    D3D11_BOX srcBox = {
        (UINT)srcLeft, (UINT)srcTop, 0,
        (UINT)srcRight, (UINT)srcBottom, 1
    };
    d3d11_context_->CopySubresourceRegion(stagingTexture, 0, 0, 0, 0,
                                           texture, 0, &srcBox);
    texture->Release();
    duplication_->ReleaseFrame();

    // 映射到 CPU
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = d3d11_context_->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        stagingTexture->Release();
        return false;
    }

    int w = stagingDesc.Width;
    int h = stagingDesc.Height;
    outFrame = cv::Mat(h, w, CV_8UC4);
    // 逐行拷贝（处理 pitch 不对齐）
    for (int y = 0; y < h; y++) {
        memcpy(outFrame.ptr(y),
               static_cast<uint8_t*>(mapped.pData) + y * mapped.RowPitch,
               w * 4);
    }

    d3d11_context_->Unmap(stagingTexture, 0);
    stagingTexture->Release();
    return true;
}

bool ScreenCapturer::CreateDXGIFactory() {
    return SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                         reinterpret_cast<void**>(&dxgi_factory_)));
}

bool ScreenCapturer::GetOutputAndDevice() {
    // 枚举显示器
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; dxgi_factory_->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        IDXGIOutput* output = nullptr;
        for (UINT j = 0; adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND; j++) {
            DXGI_OUTPUT_DESC outDesc;
            output->GetDesc(&outDesc);
            screen_width_  = outDesc.DesktopCoordinates.right - outDesc.DesktopCoordinates.left;
            screen_height_ = outDesc.DesktopCoordinates.bottom - outDesc.DesktopCoordinates.top;

            if (screen_width_ > 0 && screen_height_ > 0) {
                dxgi_output_ = output;
                // 同时创建 D3D11 设备
                D3D_FEATURE_LEVEL featureLevels[] = {
                    D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
                };
                if (SUCCEEDED(D3D11CreateDevice(
                        adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                        featureLevels, ARRAYSIZE(featureLevels),
                        D3D11_SDK_VERSION, &d3d11_device_, nullptr, &d3d11_context_))) {
                    adapter->Release();
                    return true;
                }
                output->Release();
                dxgi_output_ = nullptr;
            } else {
                output->Release();
            }
        }
        adapter->Release();
    }
    return false;
}

bool ScreenCapturer::CreateDuplication() {
    if (!d3d11_device_ || !dxgi_output_) return false;

    IDXGIOutput1* output1 = nullptr;
    HRESULT hr = dxgi_output_->QueryInterface(__uuidof(IDXGIOutput1),
                                               reinterpret_cast<void**>(&output1));
    if (FAILED(hr)) return false;

    hr = output1->DuplicateOutput(d3d11_device_, &duplication_);
    output1->Release();
    return SUCCEEDED(hr);
}
