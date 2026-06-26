#pragma once

#include <NvInfer.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <memory>

#include "config.hpp"

// ============================================================
// 模型输出格式枚举
// ============================================================
enum class ModelOutputFormat {
    AUTO_DETECT,        // 自动检测（回退方案）
    DECODED_PIXEL_XYWH, // 已解码像素坐标 [cx, cy, w, h] — YOLOv11 内置后处理
    DECODED_PIXEL_XYXY, // 已解码像素坐标 [x1, y1, x2, y2]
    FEATURE_MAP_XYWH,   // 特征图坐标 [cx, cy, w, h] — 需要 *stride
    RAW_LOGITS,         // 原始 logits — 需要 sigmoid + Dist2Box 解码
};

// ============================================================
// 检测结果
// ============================================================
struct Detection {
    float x1, y1, x2, y2;       // 边界框（模型输入坐标系）
    float confidence;             // 置信度
    int   class_id;               // 类别 ID
    float center_x() const { return (x1 + x2) * 0.5f; }
    float center_y() const { return (y1 + y2) * 0.5f; }
    float width()    const { return x2 - x1; }
    float height()   const { return y2 - y1; }
};

// ============================================================
// TensorRT 推理器 — v2.0 MVP（仅 ONNX + CPU 前后处理）
// TensorRT 10.x API
// ============================================================
class ObjectDetector {
public:
    ObjectDetector();
    ~ObjectDetector();

    // 加载 ONNX 模型（首次构建引擎 + 缓存，后续秒载）
    bool LoadModel(const std::string& modelPath, const AimConfig& config);

    // 加载预编译 .engine 文件（拖入即用，~1 秒）
    bool LoadEngineFile(const std::string& enginePath, const AimConfig& config);

    // 单帧推理：输入 BGRA 帧 + 最新配置，返回检测结果
    std::vector<Detection> Infer(const cv::Mat& bgraFrame, const AimConfig& config);

    int  InputWidth()   const { return input_w_; }
    int  InputHeight()  const { return input_h_; }
    bool IsLoaded()     const { return engine_ != nullptr; }
    bool IsTransposed() const { return transposed_; }
    int  NumClasses()   const { return num_classes_; }
    int  NumBoxes()     const { return num_boxes_; }
    int  OutputCols()   const { return num_classes_ + 4; }
    const float* RawOutput() const { return host_output_; }
    int  OutputStride() const { return transposed_ ? (num_classes_ + 4) : num_boxes_; }
    std::string GetLastError() const { return last_error_; }

private:
    // 模型加载流程
    bool BuildEngineFromONNX(const std::string& onnxPath);
    bool LoadEngineFromFile(const std::string& enginePath);
    void SaveEngineToFile(const std::string& enginePath, const nvinfer1::IHostMemory* serialized);
    void CreateExecutionContext();
    void AllocateBuffers();
    void ReleaseBuffers();
    void QueryModelInfo();

    // 前处理 / 推理 / 后处理
    void Preprocess(const cv::Mat& bgraFrame);
    void RunInference();
    std::vector<Detection> Postprocess(const AimConfig& config);

    // TRT 10.x Logger
    class Logger : public nvinfer1::ILogger {
        void log(Severity severity, const char* msg) noexcept override;
    };
    Logger logger_;

    // TensorRT 组件（unique_ptr，析构自动 delete）
    std::unique_ptr<nvinfer1::IRuntime>          runtime_;
    std::unique_ptr<nvinfer1::ICudaEngine>       engine_;
    std::unique_ptr<nvinfer1::IExecutionContext> context_;

    cudaStream_t stream_ = nullptr;

    // GPU 缓冲区
    float* host_input_    = nullptr;  // CPU pinned memory
    float* input_d_       = nullptr;  // GPU device memory
    float* output_d_      = nullptr;  // GPU device memory
    float* host_output_   = nullptr;  // CPU pinned memory

    // 模型信息
    int input_w_     = 640;
    int input_h_     = 640;
    int input_c_     = 3;
    int output_size_ = 0;
    int num_classes_ = 0;
    int num_boxes_   = 0;
    bool transposed_ = false;      // 输出是否转置 [nb, 4+nc] vs [4+nc, nb]

    // TRT 10.x: Tensor 名称
    std::string input_tensor_name_;
    std::string output_tensor_name_;

    // 推理参数缓存
    AimConfig config_;

    // 模型输出格式（通过 ONNX 分析确定）
    ModelOutputFormat output_format_ = ModelOutputFormat::AUTO_DETECT;

    // 解码参考尺寸（模型内部后处理用的像素空间尺寸，通常为 640）
    int decode_ref_size_ = 0;

    // ONNX 分析：检查模型是否内置后处理
    ModelOutputFormat AnalyzeONNXOutputFormat(const std::string& onnxPath);

    // 错误信息
    std::string last_error_;
};
