#include "object_detector.hpp"

#include <NvOnnxParser.h>
#include <cuda_runtime.h>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <exception>
#include <set>
#include <sstream>

// ============================================================
// Helpers
// ============================================================
static inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

// ============================================================
// TensorRT 10.x Logger
// ============================================================
void ObjectDetector::Logger::log(Severity severity, const char* msg) noexcept {
    if (severity <= Severity::kWARNING)
        std::cerr << "[TRT] " << msg << std::endl;
}

// ============================================================
// 构造/析构
// ============================================================
ObjectDetector::ObjectDetector() {
    cudaStreamCreate(&stream_);
}

ObjectDetector::~ObjectDetector() {
    ReleaseBuffers();
}

void ObjectDetector::ReleaseBuffers() {
    // 释放执行上下文（必须在 engine 之前）
    context_.reset();

    // 释放 CUDA 缓冲区
    if (host_input_)  { cudaFreeHost(host_input_);  host_input_ = nullptr; }
    if (host_output_) { cudaFreeHost(host_output_); host_output_ = nullptr; }
    if (input_d_)     { cudaFree(input_d_);         input_d_ = nullptr; }
    if (output_d_)    { cudaFree(output_d_);         output_d_ = nullptr; }

    // 释放 CUDA 流
    if (stream_) { cudaStreamDestroy(stream_); stream_ = nullptr; }

    // 释放 TensorRT 组件（顺序：context → engine → runtime）
    engine_.reset();
    runtime_.reset();
}

// ============================================================
// 加载 ONNX 模型 → 构建 TensorRT 引擎（TRT 10.x API）
// ============================================================
bool ObjectDetector::LoadModel(const std::string& modelPath, const AimConfig& config) {
    if (modelPath.empty()) {
        last_error_ = "模型路径为空";
        std::cerr << "[Detector] Empty model path" << std::endl;
        return false;
    }

    // 释放旧模型资源，防止 GPU 内存泄漏
    ReleaseBuffers();

    for (char c : modelPath) {
        if (static_cast<unsigned char>(c) > 127) {
            last_error_ = "模型路径含非 ASCII 字符，请移到纯英文路径";
            std::cerr << "[Detector] Model path contains non-ASCII characters: "
                      << modelPath << std::endl;
            return false;
        }
    }

    config_ = config;

    // ── 0) 分析 ONNX 结构，确定输出格式 ──
    output_format_ = AnalyzeONNXOutputFormat(modelPath);

    std::cout << "[Detector] Loading model: " << modelPath << std::endl;

    if (!BuildEngineFromONNX(modelPath)) {
        std::cerr << "[Detector] BuildEngineFromONNX failed" << std::endl;
        return false;
    }

    QueryModelInfo();
    CreateExecutionContext();
    AllocateBuffers();

    // 如果 ONNX 分析没设置 decode_ref_size_，用 input_w_ 作为默认
    if (decode_ref_size_ <= 0) {
        decode_ref_size_ = input_w_;
        std::cout << "[Detector] decode_ref_size_ auto-set to input_w_=" << input_w_ << std::endl;
    }

    std::cout << "[Detector] Ready." << std::endl;
    return true;
}

// ============================================================
// 直接加载预编译 .engine 文件（拖入即用，~1 秒）
// ============================================================
bool ObjectDetector::LoadEngineFile(const std::string& enginePath, const AimConfig& config) {
    if (enginePath.empty()) {
        last_error_ = "引擎文件路径为空";
        return false;
    }

    for (char c : enginePath) {
        if (static_cast<unsigned char>(c) > 127) {
            last_error_ = "引擎文件路径含非 ASCII 字符，请移到纯英文路径";
            return false;
        }
    }

    // 释放旧模型资源，防止 GPU 内存泄漏
    ReleaseBuffers();

    config_ = config;

    // ── 0) 尝试找到对应的 ONNX 文件进行分析 ──
    std::string onnxPath = enginePath;
    size_t dotPos = onnxPath.rfind(".engine");
    if (dotPos != std::string::npos) {
        onnxPath.replace(dotPos, 7, ".onnx");
    }
    std::ifstream onnxFile(onnxPath);
    if (onnxFile.good()) {
        onnxFile.close();
        output_format_ = AnalyzeONNXOutputFormat(onnxPath);
    } else {
        // 没有 ONNX 文件，回退到自动检测
        output_format_ = ModelOutputFormat::AUTO_DETECT;
        std::cout << "[Detector] No ONNX file found alongside .engine, using auto-detect" << std::endl;
    }

    std::cout << "[Detector] Loading pre-built engine: " << enginePath << std::endl;

    if (!LoadEngineFromFile(enginePath)) {
        std::cerr << "[Detector] LoadEngineFromFile failed" << std::endl;
        return false;
    }

    QueryModelInfo();
    CreateExecutionContext();
    AllocateBuffers();

    // 如果 ONNX 分析没设置 decode_ref_size_，用 input_w_ 作为默认
    if (decode_ref_size_ <= 0) {
        decode_ref_size_ = input_w_;
        std::cout << "[Detector] decode_ref_size_ auto-set to input_w_=" << input_w_ << std::endl;
    }

    std::cout << "[Detector] Engine loaded (instant)." << std::endl;
    return true;
}

// ============================================================
// 查询模型 I/O 信息（TRT 10.x: 使用 Tensor API）
// ============================================================
void ObjectDetector::QueryModelInfo() {
    int nbTensors = engine_->getNbIOTensors();

    std::cout << "[Detector] I/O tensor count: " << nbTensors << std::endl;

    for (int i = 0; i < nbTensors; i++) {
        const char* name = engine_->getIOTensorName(i);
        auto mode = engine_->getTensorIOMode(name);
        auto shape = engine_->getTensorShape(name);

        // 打印所有 tensor 信息用于诊断
        std::string ioType = (mode == nvinfer1::TensorIOMode::kINPUT) ? "IN" : "OUT";
        std::cout << "[Detector]   [" << i << "] " << ioType << " " << name
                  << " shape=[";
        for (int d = 0; d < shape.nbDims; d++) {
            if (d > 0) std::cout << ",";
            std::cout << shape.d[d];
        }
        std::cout << "]" << std::endl;

        if (mode == nvinfer1::TensorIOMode::kINPUT) {
            input_tensor_name_ = name;
            input_c_ = shape.d[1];
            input_h_ = shape.d[2];
            input_w_ = shape.d[3];
        } else {
            output_tensor_name_ = name;
            // 自动检测输出格式
            int dim1 = shape.d[1];
            int dim2 = shape.d[2];
            if (dim1 > dim2) {
                // 转置格式: [1, num_boxes, 4+num_classes]  (dim1 是 boxes)
                transposed_ = true;
                num_classes_ = dim2 - 4;
                num_boxes_   = dim1;
            } else {
                // 标准格式: [1, 4+num_classes, num_boxes]  (dim1 是 4+nc, dim2 是 boxes)
                transposed_ = false;
                num_classes_ = dim1 - 4;
                num_boxes_   = dim2;
            }
            output_size_ = (num_classes_ + 4) * num_boxes_;
        }
    }

    std::cout << "[Detector] Resolved: input=[" << input_c_ << "x" << input_h_
              << "x" << input_w_ << "] output=[" << (num_classes_ + 4)
              << "x" << num_boxes_ << "]"
              << (transposed_ ? " (transposed)" : "")
              << " classes=" << num_classes_ << std::endl;
}

// ============================================================
// GPU 缓冲区分配
// ============================================================
void ObjectDetector::AllocateBuffers() {
    // 确保 CUDA 流存在（ReleaseBuffers 会将其置空）
    if (!stream_) {
        cudaStreamCreate(&stream_);
    }

    size_t inputSize  = input_c_ * input_h_ * input_w_ * sizeof(float);
    size_t outputSize = output_size_ * sizeof(float);

    cudaMallocHost(&host_input_,  inputSize);
    cudaMallocHost(&host_output_, outputSize);
    cudaMalloc(&input_d_,  inputSize);
    cudaMalloc(&output_d_, outputSize);
}

void ObjectDetector::CreateExecutionContext() {
    context_.reset(engine_->createExecutionContext());
}

// ============================================================
// 从 ONNX 构建引擎（TRT 10.x Builder API）
// 自动缓存：首次构建保存 .engine，后续直接加载（~1 秒 vs ~30-60 秒）
// ============================================================                                     
bool ObjectDetector::BuildEngineFromONNX(const std::string& onnxPath) {
    std::ifstream f(onnxPath, std::ios::binary);
    if (!f.good()) {
        last_error_ = "ONNX 文件不存在或无法读取: " + onnxPath;
        std::cerr << "[Detector] ONNX file not found: " << onnxPath << std::endl;
        return false;
    }
    f.close();

    // 推导缓存路径：同目录同名，.engine 后缀
    std::string cachePath = onnxPath;
    size_t dotPos = cachePath.rfind(".onnx");
    if (dotPos != std::string::npos) {
        cachePath.replace(dotPos, 5, ".engine");
    } else {
        cachePath += ".engine";
    }

    // ── 1) 尝试从缓存加载 ──
    std::ifstream cacheFile(cachePath, std::ios::binary);
    if (cacheFile.good()) {
        std::cout << "[Detector] Found cached engine: " << cachePath << std::endl;
        if (LoadEngineFromFile(cachePath)) {
            std::cout << "[Detector] Loaded from cache (skipped build)" << std::endl;
            return true;
        }
        std::cerr << "[Detector] Cache load failed, will rebuild" << std::endl;
    }

    // ── 2) 首次构建：从 ONNX → TensorRT 引擎 ──
    std::cout << "[Detector] Building TensorRT engine from ONNX (this may take 30-60s)..." << std::endl;
    std::cout << "[Detector] Cache will be saved to: " << cachePath << std::endl;

    try {
        auto builder = std::unique_ptr<nvinfer1::IBuilder>(
            nvinfer1::createInferBuilder(logger_));
        if (!builder) {
            last_error_ = "创建 TensorRT Builder 失败，请检查 CUDA/TensorRT 安装";
            return false;
        }

        const auto explicitBatch =
            1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);

        auto network = std::unique_ptr<nvinfer1::INetworkDefinition>(
            builder->createNetworkV2(explicitBatch));
        if (!network) {
            last_error_ = "创建 TensorRT Network 失败";
            return false;
        }

        auto parser = std::unique_ptr<nvonnxparser::IParser>(
            nvonnxparser::createParser(*network, logger_));
        if (!parser) {
            last_error_ = "创建 ONNX Parser 失败！nvonnxparser_10.dll 可能未加载或版本不兼容";
            return false;
        }

        if (!parser->parseFromFile(onnxPath.c_str(),
                                   static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
            last_error_ = "ONNX 解析失败！可能原因：\n"
                          "1) 模型 opset 版本不兼容 (TensorRT 10.x 支持 opset 7-21)\n"
                          "2) 模型含不支持的算子\n"
                          "3) 模型结构与 YOLOv8+ 输出格式不匹配\n"
                          "模型: " + onnxPath;
            std::cerr << "[Detector] Failed to parse ONNX: " << onnxPath << std::endl;
            return false;
        }

        auto bConfig = std::unique_ptr<nvinfer1::IBuilderConfig>(builder->createBuilderConfig());
        if (!bConfig) {
            last_error_ = "创建 TensorRT BuilderConfig 失败";
            return false;
        }
        bConfig->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, 1ULL << 30);  // 1 GB

        if (builder->platformHasFastFp16()) {
            bConfig->setFlag(nvinfer1::BuilderFlag::kFP16);
            std::cout << "[Detector] FP16 mode enabled" << std::endl;
        }

        // TRT 10.x: buildSerializedNetwork
        auto serializedModel = std::unique_ptr<nvinfer1::IHostMemory>(
            builder->buildSerializedNetwork(*network, *bConfig));
        if (!serializedModel) {
            last_error_ = "TensorRT 引擎构建失败（buildSerializedNetwork 返回空）";
            std::cerr << "[Detector] Engine build failed" << std::endl;
            return false;
        }

        // ── 3) 保存到缓存 ──
        std::cout << "[Detector] Build complete, saving cache..." << std::endl;
        SaveEngineToFile(cachePath, serializedModel.get());

        // ── 4) 反序列化使用 ──
        runtime_.reset(nvinfer1::createInferRuntime(logger_));
        if (!runtime_) {
            last_error_ = "创建 TensorRT Runtime 失败";
            return false;
        }

        engine_.reset(runtime_->deserializeCudaEngine(
            serializedModel->data(), serializedModel->size()));

        if (!engine_) {
            last_error_ = "CUDA 引擎反序列化失败（显存不足或设备不匹配）";
            return false;
        }

        std::cout << "[Detector] Engine built & cached successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("TensorRT 异常: ") + e.what();
        std::cerr << "[Detector] Exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        last_error_ = "TensorRT 未知异常（可能是 GPU 驱动或 CUDA 版本问题）";
        std::cerr << "[Detector] Unknown exception" << std::endl;
        return false;
    }
}

// ── 从 .engine 缓存文件直接加载 ──
bool ObjectDetector::LoadEngineFromFile(const std::string& enginePath) {
    std::ifstream file(enginePath, std::ios::binary | std::ios::ate);
    if (!file.good()) return false;

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> engineData(size);
    file.read(engineData.data(), size);
    file.close();

    if (file.fail()) {
        last_error_ = "读取引擎缓存文件失败: " + enginePath;
        return false;
    }

    runtime_.reset(nvinfer1::createInferRuntime(logger_));
    if (!runtime_) {
        last_error_ = "创建 TensorRT Runtime 失败";
        return false;
    }

    engine_.reset(runtime_->deserializeCudaEngine(engineData.data(), size));
    if (!engine_) {
        last_error_ = "引擎缓存反序列化失败（GPU 架构不匹配？删除 .engine 文件重试）";
        return false;
    }

    return true;
}

// ── 保存引擎到 .engine 缓存文件 ──
void ObjectDetector::SaveEngineToFile(const std::string& enginePath,
                                       const nvinfer1::IHostMemory* serialized) {
    std::ofstream file(enginePath, std::ios::binary);
    if (!file.good()) {
        std::cerr << "[Detector] WARNING: Cannot write cache to " << enginePath << std::endl;
        return;
    }
    file.write(static_cast<const char*>(serialized->data()), serialized->size());
    file.close();
    std::cout << "[Detector] Engine cached: " << enginePath
              << " (" << (serialized->size() / 1024 / 1024) << " MB)" << std::endl;
}

// ============================================================
// 推理入口
// ============================================================                                                                             
std::vector<Detection> ObjectDetector::Infer(const cv::Mat& bgraFrame, const AimConfig& config) {
    if (!context_ || bgraFrame.empty()) return {};

    Preprocess(bgraFrame);
    RunInference();
    return Postprocess(config);
}

// ============================================================
// CPU 预处理（OpenCV: BGRA→RGB + LetterBox + Normalize）
// ============================================================
void ObjectDetector::Preprocess(const cv::Mat& bgraFrame) {
    // 1) BGRA → RGB
    cv::Mat rgb;
    cv::cvtColor(bgraFrame, rgb, cv::COLOR_BGRA2RGB);

    // 2) LetterBox resize
    cv::Mat letterbox;
    float scale = std::min(static_cast<float>(input_w_) / rgb.cols,
                           static_cast<float>(input_h_) / rgb.rows);
    int newW = static_cast<int>(rgb.cols * scale);
    int newH = static_cast<int>(rgb.rows * scale);
    cv::resize(rgb, letterbox, cv::Size(newW, newH), 0, 0, cv::INTER_LINEAR);

    int padW = input_w_ - newW;
    int padH = input_h_ - newH;
    int padLeft = padW / 2;
    int padTop  = padH / 2;

    cv::Mat padded(input_h_, input_w_, CV_8UC3, cv::Scalar(114, 114, 114));
    letterbox.copyTo(padded(cv::Rect(padLeft, padTop, newW, newH)));

    // 3) HWC → CHW + float32 + /255
    padded.convertTo(padded, CV_32FC3, 1.0 / 255.0);

    std::vector<cv::Mat> channels(input_c_);
    cv::split(padded, channels);

    int channelSize = input_h_ * input_w_;
    for (int c = 0; c < input_c_; c++) {
        memcpy(host_input_ + c * channelSize, channels[c].data, channelSize * sizeof(float));
    }

    // 4) CPU→GPU 拷贝
    size_t inputSize = input_c_ * input_h_ * input_w_ * sizeof(float);
    cudaMemcpyAsync(input_d_, host_input_, inputSize, cudaMemcpyHostToDevice, stream_);
}

// ============================================================
// TensorRT 10.x 推理
// ============================================================                                        
void ObjectDetector::RunInference() {
    // TRT 10.x: setTensorAddress + enqueueV3
    context_->setTensorAddress(input_tensor_name_.c_str(), input_d_);
    context_->setTensorAddress(output_tensor_name_.c_str(), output_d_);
    context_->enqueueV3(stream_);

    // GPU→CPU 拷贝
    size_t outputSize = output_size_ * sizeof(float);
    cudaMemcpyAsync(host_output_, output_d_, outputSize, cudaMemcpyDeviceToHost, stream_);
    cudaStreamSynchronize(stream_);
}

// ============================================================
// ONNX 结构分析：判断模型是否内置后处理
//
// 检测策略：
// 1. 找到最后一个 Concat 之前的节点链
// 2. 检查是否有 Sigmoid + Split → 说明模型已经做了坐标激活
// 3. 检查是否有 Mul(constant=stride) + Add(constant=grid) → 说明已经解码到像素空间
//    → DECODED_PIXEL_XYWH (YOLOv11 内置后处理)
// 4. 只有 Sigmoid 但没有 stride 乘 → FEATURE_MAP_XYWH (YOLOv8 风格)
// 5. 都没有 → RAW_LOGITS 或回退 AUTO_DETECT
// ============================================================
ModelOutputFormat ObjectDetector::AnalyzeONNXOutputFormat(const std::string& onnxPath) {
    std::cout << "[Detector] Analyzing ONNX structure: " << onnxPath << std::endl;

    std::ifstream file(onnxPath, std::ios::binary);
    if (!file.good()) {
        std::cout << "[Detector] ONNX file not accessible, using AUTO_DETECT" << std::endl;
        return ModelOutputFormat::AUTO_DETECT;
    }
    file.close();

    // 轻量级分析：直接读 protobuf 二进制，提取检测头关键操作
    // 不依赖 onnx python 库，纯 C++ 解析关键特征

    // 读取整个文件
    std::ifstream in(onnxPath, std::ios::binary | std::ios::ate);
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!in.read(buffer.data(), size)) {
        std::cout << "[Detector] Failed to read ONNX, using AUTO_DETECT" << std::endl;
        return ModelOutputFormat::AUTO_DETECT;
    }
    in.close();

    std::string content(buffer.data(), size);

    // 特征检测：查找检测头后处理的关键模式
    // YOLOv11 内置后处理特征：
    //   1. Sigmoid 节点（坐标激活）
    //   2. Split 节点（分离 bbox 和 classes），split 属性 axis=4, split=[2,2,n]
    //   3. Mul 后紧跟 Add → grid 偏移解码
    //   4. Mul(constant=stride) → stride 缩放
    //
    // 简单判断：如果存在 Sigmoid + "Mul" 且包含 stride 特征值(8/16/32)的常量
    //          且输出 tensor shape 是 [1, N, 4+classes] 而非 [1, 4+classes, N]
    //          → DECODED_PIXEL_XYWH

    // 检查是否有 Sigmoid 操作（在检测头部分）
    bool hasSigmoid = (content.find("Sigmoid") != std::string::npos);

    // 检查是否有 Split 操作（用于分离 bbox 和 class）
    bool hasSplit = (content.find("Split") != std::string::npos);

    // 检查模型名称/结构特征
    // YOLOv8/v11 的 backbone 通常包含 "model.0" ~ "model.24" 节点名
    bool hasYoloDetect = (content.find("model.24") != std::string::npos ||
                          content.find("model.22") != std::string::npos);

    // 判断输出格式
    ModelOutputFormat fmt = ModelOutputFormat::AUTO_DETECT;

    if (hasSigmoid && hasSplit && hasYoloDetect) {
        // 有 Sigmoid + Split → 模型内置了坐标激活和 bbox/class 分离
        // 需要进一步判断是否已经乘了 stride（即解码到像素空间）

        // 检查是否包含 stride 相关的常量值（8.0, 16.0, 32.0）
        // 这些值通常在 Mul 节点的 Constant 输入中出现
        // 我们在二进制中搜索 float32 编码的 8.0, 16.0, 32.0
        bool hasStrideDecode = false;

        // 搜索 float 8.0f 的 IEEE 754 编码: 0x41000000
        // 和 16.0f: 0x41800000, 32.0f: 0x42000000
        // 在 protobuf 中 float 可能被编码为 fixed32 little-endian
        auto searchFloat = [&content](float val) -> bool {
            uint32_t bits;
            memcpy(&bits, &val, 4);
            // protobuf wire type: fixed32 = 5, field number 1 = 0x0D
            // 简单搜索原始字节序列
            char pattern[4];
            memcpy(pattern, &bits, 4);
            std::string pat(pattern, 4);
            return content.find(pat) != std::string::npos;
        };

        if (searchFloat(8.0f) || searchFloat(16.0f) || searchFloat(32.0f)) {
            hasStrideDecode = true;
        }

        if (hasStrideDecode) {
            fmt = ModelOutputFormat::DECODED_PIXEL_XYWH;
            // decode_ref_size_ 在 QueryModelInfo 之后设为 input_w_
            // 模型内部 decode 的像素空间等于输入尺寸
            // 如果 input_w_=320 但模型用 640 空间 decode，需要手动设置 ref_size_override
            decode_ref_size_ = 0;  // 0 表示用 input_w_ 自动设置
            std::cout << "[Detector] ONNX analysis: DECODED_PIXEL_XYWH (YOLOv11 with built-in decode)" << std::endl;
        } else {
            fmt = ModelOutputFormat::FEATURE_MAP_XYWH;
            std::cout << "[Detector] ONNX analysis: FEATURE_MAP_XYWH (needs stride decode)" << std::endl;
        }
    } else if (hasSigmoid && hasYoloDetect) {
        // 有 Sigmoid 但没 Split → 可能坐标已映射到特征图
        fmt = ModelOutputFormat::FEATURE_MAP_XYWH;
        std::cout << "[Detector] ONNX analysis: FEATURE_MAP_XYWH (sigmoid only)" << std::endl;
    } else if (!hasSigmoid) {
        // 没有 Sigmoid → 原始 logits
        fmt = ModelOutputFormat::RAW_LOGITS;
        std::cout << "[Detector] ONNX analysis: RAW_LOGITS (no sigmoid in model)" << std::endl;
    } else {
        std::cout << "[Detector] ONNX analysis: AUTO_DETECT (fallback)" << std::endl;
    }

    return fmt;
}

// ============================================================
// CPU 后处理（基于 ONNX 分析结果选择解码方案）
//
// 模型输出格式枚举：
//   DECODED_PIXEL_XYWH: 已解码像素坐标 [cx,cy,w,h] — 直接 /input_size 归一化
//   FEATURE_MAP_XYWH:   特征图坐标 [cx,cy,w,h] — 需要 *stride
//   RAW_LOGITS:         原始 logits — 需要 sigmoid + Dist2Box
//   AUTO_DETECT:        回退到之前的自动检测逻辑
//
// 3 个特征图层 (320x320 输入, 3 类, 6300 boxes):
//   feat0: 40x40x3=4800, stride=8
//   feat1: 20x20x3=1200, stride=16
//   feat2: 10x10x3=300,  stride=32
// ============================================================                                     
std::vector<Detection> ObjectDetector::Postprocess(const AimConfig& config) {
    // 模型输出: [1, num_boxes, 4+num_classes]
    //   每行 = [cx, cy, w, h, cls0, cls1, ..., clsN]
    //   坐标已在 decode_ref_size_ 像素空间（模型内置 YOLOv11 后处理）
    //
    // 策略：只处理用户勾选的 target_class_ids，多类 NMS（同类才抑制）

    std::vector<Detection> detections;
    float confThresh = config.confidence_threshold;
    const auto& targetClsIds = config.target_class_ids;
    if (targetClsIds.empty()) return {};  // 没勾选任何类别，不检测

    int outStride = transposed_ ? (num_classes_ + 4) : num_boxes_;

    auto flat_idx = [&](int i, int col) -> int {
        return transposed_ ? (i * outStride + col) : (col * outStride + i);
    };

    float refSize = static_cast<float>(decode_ref_size_);  // 模型内部解码参考尺寸

    // 诊断：每 300 帧打印一次原始坐标范围
    static int diagFrame = 0;
    bool doDiag = (++diagFrame % 300 == 0);
    float diagMaxX = 0, diagMaxY = 0;

    for (int i = 0; i < num_boxes_; i++) {
        // 对每个勾选的类别检查置信度
        for (int clsId : targetClsIds) {
            if (clsId < 0 || clsId >= num_classes_) continue;
            float score = host_output_[flat_idx(i, 4 + clsId)];
            if (score < confThresh) continue;

            // 读取像素坐标
            float cx = host_output_[flat_idx(i, 0)];
            float cy = host_output_[flat_idx(i, 1)];
            float bw = host_output_[flat_idx(i, 2)];
            float bh = host_output_[flat_idx(i, 3)];

            if (doDiag) {
                if (cx > diagMaxX) diagMaxX = cx;
                if (cy > diagMaxY) diagMaxY = cy;
            }

            // 归一化到 [0,1]
            Detection det;
            det.x1 = (cx - bw * 0.5f) / refSize;
            det.y1 = (cy - bh * 0.5f) / refSize;
            det.x2 = (cx + bw * 0.5f) / refSize;
            det.y2 = (cy + bh * 0.5f) / refSize;
            det.confidence = score;
            det.class_id   = clsId;

            // clamp
            det.x1 = std::max(0.0f, std::min(1.0f, det.x1));
            det.y1 = std::max(0.0f, std::min(1.0f, det.y1));
            det.x2 = std::max(0.0f, std::min(1.0f, det.x2));
            det.y2 = std::max(0.0f, std::min(1.0f, det.y2));

            if (det.x2 > det.x1 && det.y2 > det.y1)
                detections.push_back(det);
        }
    }

    if (detections.empty()) return detections;

    // 按置信度降序排序
    std::sort(detections.begin(), detections.end(),
              [](const Detection& a, const Detection& b) {
                  return a.confidence > b.confidence;
              });

    // 多类 NMS：同类才抑制，不同类不干涉
    std::vector<Detection> nmsResult;
    std::vector<bool>      suppressed(detections.size(), false);
    float nmsThresh = config.nms_threshold;

    for (size_t i = 0; i < detections.size(); i++) {
        if (suppressed[i]) continue;
        nmsResult.push_back(detections[i]);

        for (size_t j = i + 1; j < detections.size(); j++) {
            if (suppressed[j]) continue;
            // 不同类别不 NMS
            if (detections[i].class_id != detections[j].class_id) continue;

            float interX1 = std::max(detections[i].x1, detections[j].x1);
            float interY1 = std::max(detections[i].y1, detections[j].y1);
            float interX2 = std::min(detections[i].x2, detections[j].x2);
            float interY2 = std::min(detections[i].y2, detections[j].y2);
            float interW = std::max(0.0f, interX2 - interX1);
            float interH = std::max(0.0f, interY2 - interY1);
            float interArea = interW * interH;

            float area1 = (detections[i].x2 - detections[i].x1) * (detections[i].y2 - detections[i].y1);
            float area2 = (detections[j].x2 - detections[j].x1) * (detections[j].y2 - detections[j].y1);
            float iou = interArea / (area1 + area2 - interArea + 1e-6f);

            if (iou > nmsThresh)
                suppressed[j] = true;
        }
    }

    if (doDiag && !nmsResult.empty()) {
        std::cout << "[Post] Diag: input=" << input_w_ << "x" << input_h_
                  << " refSize=" << refSize
                  << " nc=" << num_classes_ << " targets=[";
        for (size_t t = 0; t < targetClsIds.size(); t++) {
            if (t > 0) std::cout << ",";
            std::cout << targetClsIds[t];
        }
        std::cout << "] rawXYmax=" << diagMaxX << "," << diagMaxY
                  << " nmsCnt=" << nmsResult.size()
                  << " 1st=[" << nmsResult[0].x1 << "," << nmsResult[0].y1
                  << " " << nmsResult[0].x2 << "," << nmsResult[0].y2
                  << "] cls=" << nmsResult[0].class_id
                  << std::endl;
    }

    return nmsResult;
}
