# Auto_Aim v2.0 设计文档

> 基于 DXGI + TensorRT C++ API + IbInputSimulator 的实时 AI 自瞄系统
> 
> **纯 C++ 架构：C++ Qt6 界面 + C++ 推理引擎，同进程零开销调用**
> 
> 支持 .onnx / .engine 双格式，适配 NVIDIA GeForce / RTX / Quadro 全系列 GPU

---

## 架构

纯 C++：Qt6 UI 直接调用 C++ 引擎 API，同进程零开销，单一编译产物，部署简单。Python PySide6 UI 已弃用。

---

## 一、技术方案

### 1.1 方案选型：方案A — .onnx + .engine 双兼容 + 原生 TensorRT C++ API

**核心理念**：`.onnx` 和 `.engine` **都直接加载使用**，不需要中间构建转换步骤。用户提供什么格式，系统就直接用什么格式推理。

```
用户提供 → my_model.onnx  ─┐
                           ├─→ 智能识别格式
用户提供 → my_model.engine ─┘
                    │
         ┌─────────▼──────────┐
         │  .engine 文件?     │
         │  ├─ 是 → 检测 SM 兼容性   │
         │  │   ├─ 兼容 → 直接反序列化 (< 1s)  │
         │  │   └─ 不兼容 → ❌ 提示用户使用 .onnx  │
         │  └─ 否(.onnx) →    │
         │     nvonnxparser 直接解析   │  (运行时加载，自动适配任意 GPU)
         └────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────┐
│  GPU 全管线:                                 │
│  CUDA预处理 → TensorRT推理 → CUDA后处理       │
└─────────────────────────────────────────────┘
```

**两种使用场景**：

| 场景 | 输入 | 启动耗时 | 适用人群 |
|------|------|---------|---------|
| 入门用户 | `.onnx` | 直接加载 | 训练完直接用的用户 |
| 高级用户 | `.engine` | < 1s | 已预编译优化的用户 |

#### ⚠️ 关键限制：`.engine` 文件绑定 GPU 架构

TensorRT 引擎是**硬件相关**的二进制文件，绑定构建时的 GPU 计算能力 (SM version)：

| GPU 系列 | 架构 | SM | 兼容 `.engine` 范围 |
|----------|------|----|-------------------|
| GTX 10xx | Pascal | 6.1 | 仅 Pascal |
| GTX 16xx | Turing | 7.5 | 仅 Turing |
| RTX 20xx | Turing | 7.5 | 仅 Turing |
| RTX 30xx | Ampere | 8.6 | 仅 Ampere |
| RTX 40xx | Ada Lovelace | 8.9 | 仅 Ada |
| RTX 50xx | Blackwell | 10.x | 仅 Blackwell |
| Quadro / A 系列 | 各异 | 各异 | 同 SM 版本 |

> **结论：RTX 4090 导出的 `.engine` 无法在 GTX 1060 上加载。**

**本项目的应对策略**：

| 格式 | 跨 GPU 兼容性 | 原理 |
|------|-------------|------|
| **`.onnx`** | ✅ **全 NVIDIA GPU 通用** | `nvonnxparser` 在运行时自动根据当前 GPU 架构构建引擎 |
| **`.engine`** | ❌ 绑死导出时的 GPU | 纯序列化二进制，SM 不匹配则加载失败 |

**系统行为**：
- `.onnx`：无需关心 GPU 型号，运行时自动适配，是推荐的通用格式
- `.engine`：加载时检测 SM 兼容性；不匹配则弹窗提示「此引擎文件为 RTX 4090 (SM 8.9) 构建，当前 GPU 为 GTX 1060 (SM 6.1)，请使用 .onnx 格式」
- 对于需要在多台不同 GPU 机器间分发的场景，**推荐始终使用 .onnx**

### 1.2 为什么不直接用 ONNX Runtime

| 方案 | ONNX | Engine | GPU预处理 | GPU后处理 | CUDA Graph | 依赖 |
|------|------|--------|----------|----------|------------|------|
| **ONNX RT + TRT EP** (v1.0) | ✅ | ❌ | ❌ CPU | ❌ CPU | ❌ | onnxruntime |
| **TensorRT-YOLO 库** | ❌ | ✅ | ✅ | ✅ | ✅ | trtyolo 动态库 |
| **方案A：TRT C++ API** | ✅ 直接解析 | ✅ 直接加载 | ✅ 自实现 | ✅ 自实现 | ✅ 自实现 | 仅 TensorRT + CUDA |

**关键优势**：
- 用户可提供 `.onnx`（训练的通用格式）或 `.engine`（预编译格式），**两种都直接加载使用，无需转换缓存**
- `.onnx`：nvonnxparser 直接解析为 TensorRT 引擎，运行时加载
- `.engine`：直接反序列化加载，毫秒级冷启动
- 预处理和后处理可迁移到 GPU 端，降低整管线延迟
- **C++ Qt6 原生 UI + C++ 推理引擎**：同进程零开销调用，编译期类型检查，单一 .exe 发布

### 1.3 模型加载（双格式直接加载）

```cpp
// model_loader.hpp
class ModelLoader {
public:
    // 传入 .onnx 或 .engine 路径，自动识别并直接加载
    // .engine → 检测 SM 兼容性 → 兼容则反序列化加载，不兼容则报错
    // .onnx   → nvonnxparser 直接解析为引擎（运行时自动适配当前 GPU）
    // 返回时同时填充 model_family 和 output_format
    static nvinfer1::ICudaEngine* Load(
        const std::string& model_path,      // .onnx 或 .engine
        nvinfer1::ILogger& logger,
        bool fp16 = true,
        VersionDetector::ModelFamily* out_family = nullptr,  // 输出: 模型家族
        VersionDetector::OutputFormat* out_format = nullptr   // 输出: 输出格式
    );

private:
    static bool IsEngineFile(const std::string& path);
    // 检测 .engine 文件的 SM 版本是否与当前 GPU 兼容
    static bool CheckSMCompatibility(const std::string& engine_path, 
                                      std::string& out_engine_sm, 
                                      std::string& out_device_sm);
    static nvinfer1::ICudaEngine* LoadEngine(const std::string& engine_path, nvinfer1::ILogger& logger);
    static nvinfer1::ICudaEngine* LoadFromONNX(
        const std::string& onnx_path, 
        nvinfer1::ILogger& logger, 
        bool fp16
    );
};
```

### 1.4 CUDA 预处理 Kernel（替代 CPU OpenCV）

将 DXGI 捕获的 BGRA 帧直接在 GPU 上完成：通道交换（SwapRB）、Resize、LetterBox 填充、归一化。

```cpp
// cuda_preprocess.cu
__global__ void PreprocessKernel(
    const uchar4* src,    // DXGI BGRA 输入
    float* dst,           // 模型输入 tensor (NCHW, float32)
    int src_width, int src_height, size_t src_pitch,
    int dst_width, int dst_height,
    float scale_x, float scale_y, int offset_x, int offset_y,
    float mean_r, float mean_g, float mean_b,
    float std_r, float std_g, float std_b
);
```

**性能对比**：

| 环节 | v1.0 CPU OpenCV | v2.0 CUDA Kernel |
|------|----------------|-----------------|
| BGRA→RGB + Resize + LetterBox + Normalize | 3-5ms | ~0.1ms |
| 数据位置 | GPU→CPU Map → CPU处理 → CPU→GPU Copy | GPU 零拷贝 |

### 1.5 CUDA 后处理（多路径自动切换）

```cpp
// cuda_postprocess.cu — 根据post_kind_自动选择 kernel

// 路径 A: Native (nvonnxparser 直接解析，v8+ 统一格式)
__global__ void AnchorFreeDecodeKernel(
    const float* raw_output,    // [1, 4+C, N]
    Detection* detections,
    int* detection_count,
    int num_classes, int num_boxes,
    float conf_threshold, float nms_threshold,
    float scale_x, float scale_y,
    int offset_x, int offset_y
);

// 路径 B: PassThrough (trtyolo-export 已内嵌 EfficientIdxNMS)
// 输出 tensor 直接是最终检测结果 [num_dets, boxes, scores, classes]
// 无需任何解码，直接 memcpy
__global__ void PassThroughCopyKernel(
    const float* nms_output,    // 已过滤 + NMS 的结果
    Detection* detections,
    int* detection_count,
    int max_detections
);
```

**性能对比**：

| 环节 | v1.0 CPU | v2.0 Native GPU | v2.0 PassThrough |
|------|---------|-----------------|-----------------|
| 解码 + NMS | 1-2ms | ~0.2ms | ~0ms (引擎内完成) |
| 适用场景 | - | 拖入 ONNX 即用 | 先 trtyolo-export 再加载 |

### 1.6 整体架构

```
┌──────────────────────────────────────────────────────────────────┐
│                    C++ Qt6 UI (ui_cpp/)                           │
│  主窗口 · 模型页 · 预览页 · 瞄准参数页 · 性能监控                   │
│  ↓ 直接调用 (同进程，零开销)                                       │
├──────────────────────────────────────────────────────────────────┤
│                    C++ 引擎 (src/engine.hpp)                      │
│  ┌────────────┬──────────────┬──────────────────┬───────────────┐│
│  │DXGI        │TensorRT      │Target            │Mouse          ││
│  │Capturer    │Inferencer    │Selector           │Controller     ││
│  │屏幕捕获     │(CUDA预处理   │最优目标选择        │├─ IbInputSim  ││
│  │            │ +推理+后处理) │                   │├─ SendInput   ││
│  │            │              │                   │└─ KMBox ★    ││
│  └────────────┴──────────────┴──────────────────┴───────────────┘│
│  + ModelLoader (onnx/engine 双格式直接加载)                       │
│  + VersionDetector (v8~v26 自动识别)                              │
│  + AimEngine (主循环协调器)                                       │
└──────────────────────────────────────────────────────────────────┘
```

**分工**：UI 和引擎均在 C++ 同进程中，`AimEngine` 类定义在 `src/engine.hpp`，Qt6 UI 直接 `#include` 即可调用。

### 1.7 KMBox 硬件级鼠标模拟（闭环系统）

KMBox 是一款硬件级别的键盘鼠标模拟盒子，通过 USB 连接到目标电脑（游戏机），在目标电脑上表现为标准 USB HID 设备。控制电脑通过网络（UDP）向盒子发送移动指令，盒子将指令转换为 HID 报告注入目标电脑。

**核心优势**：纯硬件级 USB HID 模拟，对目标电脑操作系统和任何反作弊软件完全透明，无可检测的软件特征。

#### 闭环工作流程

```
┌──────────────────────────────────────────────────────────────────────┐
│                        控制电脑 (运行 Auto_Aim)                        │
│                                                                      │
│  DXGI 截屏 → GPU 预处理 → TensorRT 推理 → 目标选择                    │
│                                              ↓                       │
│                                     计算鼠标移动距离 ← 从网络获取     │
│                                       (Δx, Δy)       真实鼠标坐标     │
│                                              ↓                       │
│                        ┌──→ 发送 UDP 指令到 KMBox ──→  ┐             │
│                        │                              │             │
│  ══════════════════════╪══════   以太网 / WiFi  ═══════╪══════════  │
│                        │                              │             │
│  ┌─────────────────────┼── KMBox 盒子 ────────────────┼──────────┐  │
│  │                     ↓                              │          │  │
│  │   UDP 接收 → 加密解密 → 鼠标移动指令 → USB HID 报告             │  │
│  │                                   ↑  ↓                         │  │
│  │                    物理键鼠 ← 监控/屏蔽  → USB HID 注入          │  │
│  └────────────────────────────────────────────────────────────────┘  │
│                              │                                       │
│  ════════════════════════════╪═══════  USB 线 ═══════════════════════ │
│                              ↓                                       │
│  ┌─────────────────────────────────────────────────────────────────┐ │
│  │                 目标电脑 (游戏机)                                  │ │
│  │  KMBox 被识别为标准 USB 键盘 + 鼠标  →  游戏接收鼠标移动事件      │ │
│  └─────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────┘
```

#### 与软件方案对比

| 维度 | SendInput | IbInputSimulator (驱动) | Razer/Logitech 驱动 | **KMBox 硬件** |
|------|-----------|------------------------|---------------------|---------------|
| 实现层 | 应用层 API | 内核驱动 | 厂商驱动 | **独立硬件 USB HID** |
| 可检测性 | ★★★★★ 极易 | ★★★ 中等 | ★★★ 中等 | ★☆☆ 极难（物理设备） |
| 部署复杂度 | 无 | 需安装驱动 | 需特定鼠标 | 需硬件 + 网线 |
| 双机支持 | ❌ | ❌ | ❌ | ✅ 控制/游戏机分离 |
| 物理键鼠监控 | ❌ | ❌ | ❌ | ✅ 盒子直接监听 |
| 物理键鼠屏蔽 | ❌ | ❌ | ❌ | ✅ 按轴/按键屏蔽 |
| 加密传输 | N/A | N/A | N/A | ✅ XTEA 变体加密 |
| 人类轨迹模拟 | ❌ | ❌ | ❌ | ✅ 贝塞尔曲线 + 自动移动 |
| 调用延迟 | ~0ms | ~0ms | ~0ms | **<1ms (UDP 局域网)** |

### 1.8 多版本 YOLO 模型适配策略（v8 ~ v26）

用户可能使用 YOLOv8 到 YOLOv26（甚至未来版本）的训练模型，需要在后处理层面做到**版本无关**。

#### 为什么 v8+ 版本可以统一？

YOLOv8 及之后所有版本共享同一套输出约定：

| 特性 | v5 及更早 | **v8 / v9 / v10 / v11 / ... / v26** |
|------|----------|--------------------------------------|
| 框编码 | Anchor-based (需解码) | **Anchor-free**，直接输出 `[x1,y1,x2,y2]` |
| 输出头 | 分 3 层不同尺度 | **端到端单输出**，一次前向完成 |
| 分数格式 | 需 Sigmoid | 已是概率值 |
| 后处理 | 每个版本不同 | **统一用 NMS**，与版本无关 |

版本之间的差异（不同的 neck、head、训练策略）全部封装在 ONNX 的计算图中，TensorRT 推理引擎无需感知。

#### 两条适配路径

```
用户 ONNX 模型 (v8~v26 任意版本)
        │
        ├── 路径 A: 直接 nvonnxparser 解析 (内置，零依赖)
        │    引擎自动推断输出维度 → 选择后处理解码器
        │    适用: 标准 detect 任务，输出格式规范的模型
        │
        └── 路径 B: trtyolo-export 预处理 (推荐高级用户)
             将 EfficientIdxNMS 插件嵌入 engine
             → C++ 推理引擎直接拿到最终检测结果，无需后处理
             适用: 需要 segment/pose/obb 等复杂任务
```

#### 版本自动检测 (VersionDetector)

```cpp
// version_detector.hpp — 通过 ONNX 图属性或输出 shape 推断模型版本
class VersionDetector {
public:
    enum class ModelFamily {
        YOLOv5,         // Anchor-based (暂不支持，提示用户升级)
        YOLOv8_Plus,    // v8~v26，Anchor-free，端到端
        Unknown
    };

    // 从 onnx 文件路径检测
    static ModelFamily DetectFromONNX(const std::string& onnx_path);

    // 从已加载的 ICudaEngine 检测（通过输出 tensor 维度）
    static ModelFamily DetectFromEngine(nvinfer1::ICudaEngine* engine);

    // 获取输出格式描述
    struct OutputFormat {
        int num_outputs;          // 输出 tensor 数量 (detect=1, segment=2, pose=2)
        int num_classes;          // 类别数 (从输出维度推断)
        int num_boxes;            // 每帧最大检测数
        bool box_decoder_needed;  // 是否需要额外的框解码 (v8+ 不需要)
    };
    static OutputFormat GetOutputFormat(nvinfer1::ICudaEngine* engine);
};
```

#### 后处理策略：根据输出格式自动切换

```cpp
// trt_inferencer.hpp (补充)
enum class PostProcessKind {
    Native,         // 路径 A: 自己 CUDA kernel 解码 + NMS
    PassThrough,    // 路径 B: trtyolo-export 已内嵌 EfficientIdxNMS，输出直接可用
};

class TRTInferencer {
    // ...
    PostProcessKind post_kind_;     // 构建引擎时确定
    OutputFormat    output_format_; // 从引擎自动推断
};
```

**路径 A (Native) 的 CUDA 后处理**：对 v8+ 系列模型统一处理——

```cpp
// 对 v8+ 统一的后处理：输出 shape = [1, 84, 8400] (以 80 类为例)
//   其中 84 = 4(框坐标) + 80(类别概率)
//   → GPU kernel: 过滤低置信度 → 按类别分组 NMS → 选出 top-k
__global__ void AnchorFreeDecodeKernel(
    const float* raw_output,     // [1, 4+C, N]
    Detection* detections,
    int* detection_count,
    int num_classes, int num_boxes,
    float conf_threshold, float nms_threshold,
    float scale_x, float scale_y, int offset_x, int offset_y
);
```

**路径 B (PassThrough)**：trtyolo-export 导出时已嵌入 `EfficientIdxNMS` 插件——

```
原始 ONNX 输入:
  image → Conv → ... → [bboxes, scores, classes]

trtyolo-export 处理后:
  image → Conv → ... → EfficientIdxNMS → [num_dets, boxes, scores, classes]
                                                  ↑
                                    C++ 直接 memcpy 到 Detection 数组即可
```

- `trtyolo-export` 是 TensorRT-YOLO 官方提供的导出 CLI 工具（Python）
- 用户可在 UI 中一键调用（集成 Python 子进程）或预先手动导出
- 引擎构建成功后，C++ 推理层判断输出 tensor 数量：**1 个 = Native，3-4 个 = PassThrough**

#### 用户工作流

```
┌─ 标准模式 ──────────────────────────────────────┐
│  用户拖入 my_v11.onnx                           │
│  → 自动检测版本 → 确认 v8+                    │
│  → nvonnxparser 直接解析 engine                  │
│  → Native 后处理路径                           │
│  → 一切自动，用户无感知                          │
└────────────────────────────────────────────────┘

┌─ 高级模式（追求极致性能）───────────────────────┐
│  用户拖入 my_v11.engine (含 EfficientIdxNMS)     │
│  → 直接反序列化加载，毫秒级启动                  │
│  → PassThrough 后处理路径 (延迟再降 ~0.5ms)     │
│  → engine 由用户预先用 trtyolo-export 导出       │
└────────────────────────────────────────────────┘
```

#### 不支持的场景与降级

| 场景 | 处理 |
|------|------|
| 用户提供 v5 模型 (anchor-based) | ⚠️ 弹窗提示："检测到 YOLOv5 模型，建议升级到 v8+" |
| 输出格式无法识别 | ⚠️ 弹窗提示："无法识别模型输出格式，请确认是否为 YOLO detect 模型" |
| segment / pose / obb 任务 | 提示使用 .engine 格式（trtyolo-export 预导出），内置的 Native 后处理仅支持 detect |
| `.engine` SM 版本不匹配 | ⚠️ 弹窗提示："此引擎为 RTX 4090 (SM 8.9) 构建，当前 GPU 为 GTX 1060 (SM 6.1)，请使用 .onnx 格式" |

---

## 二、模块详细设计（技术）

### 2.1 依赖项

| 依赖 | 版本 | 用途 |
|------|------|------|
| Qt6 | ≥ 6.5 | C++ Qt6 Widgets 模块，原生 UI 框架 |
| CUDA Toolkit | ≥ 11.0 | GPU 计算 + CUDA kernel |
| cuDNN | 与 CUDA 匹配 | TensorRT 依赖 |
| TensorRT | ≥ 8.6 | 推理引擎 + ONNX 解析 (nvonnxparser) |
| OpenCV | ≥ 4.5 | 画面帧格式转换、可视化辅助 |
| DirectX 11 SDK | Windows 自带 | DXGI 屏幕捕获 |
| IbInputSimulator | 已提供 | 驱动级鼠标输入 |
| KMBox Net | 硬件 (已提供 SDK) | UDP 硬件级 USB HID 模拟（需物理盒子） |
| CMake | ≥ 3.18 | 构建系统 |

**不再依赖**：
- ~~PySide6~~ → 已弃用
- ~~Python 作为运行时~~ → C++ Qt UI 编译为单一 `.exe`，无需 Python
- ~~pybind11~~ → 已弃用

### 2.2 Config（配置管理）

```cpp
// config.hpp
struct AimConfig {
    // ── 模型 ──
    std::string model_path = "yolov8n.onnx";   // 模型路径（支持 .onnx / .engine）
    bool enable_fp16 = true;                    // FP16 推理（仅 .onnx 构建时生效）
    float confidence_threshold = 0.5f;
    float nms_threshold = 0.4f;
    int max_detections = 300;                   // 最大检测数

    // ── 画面 ──
    int crop_size = 640;                        // 截取区域边长

    // ── 瞄准 ──
    int max_lock_distance = 100;
    int target_class_id = 0;                    // 目标类别（0=person）
    float target_y_ratio = 0.3f;                // 瞄准点Y偏移

    // ── 鼠标 ──
    double sensitivity = 1.0;
    double pixels_for_360 = 16410.0;
    double horizontal_fov = 120.0;
    double aim_smoothing = 1.0;

    // ── 按键 ──
    int aim_key = VK_LBUTTON;
    int single_shot_key = VK_F8;

    // ── 可视化 ──
    bool enable_visualization = true;
    std::string window_name = "AutoAim v2.0";
};
```

### 2.3 DXGI ScreenCapturer

与 v1.0 一致，DXGI Desktop Duplication 捕获屏幕中心 `crop_size × crop_size` 区域到 staging texture。

### 2.4 TensorRT Inferencer（核心改造）

```cpp
// trt_inferencer.hpp
class TRTInferencer {
public:
    // 从 .onnx 或 .engine 直接加载（自动识别，无需构建缓存）
    TRTInferencer(const std::string& model_path, const AimConfig& config);
    ~TRTInferencer();

    // 单帧推理（帧数据在 GPU 上零拷贝处理）
    std::vector<Detection> Infer(const cv::Mat& bgra_frame);

    // 获取模型输入尺寸
    int InputWidth()  const { return input_w_; }
    int InputHeight() const { return input_h_; }

private:
    void LoadEngine();                  // 直接加载引擎（onnx→nvonnxparser / engine→反序列化）
    void CreateExecutionContext();      // 创建执行上下文
    void AllocateBuffers();             // 分配 GPU buffer
    void Preprocess(const cv::Mat& frame);  // CUDA 预处理
    void RunInference();                    // TensorRT 推理
    void Postprocess();                     // CUDA 后处理

    nvinfer1::IRuntime* runtime_ = nullptr;
    nvinfer1::ICudaEngine* engine_ = nullptr;
    nvinfer1::IExecutionContext* context_ = nullptr;
    cudaStream_t stream_;

    // GPU Buffers
    void* buffers_[3] = {nullptr};  // input, output0, output1
    float* input_d_ = nullptr;      // GPU 输入 tensor
    float* output_d_ = nullptr;     // GPU 输出 tensor
    float* output_h_ = nullptr;     // CPU 输出（后处理结果拷贝）

    int input_w_, input_h_;
    int output_size_;
    std::vector<Detection> detections_;

    Logger logger_;                 // TensorRT Logger
};
```

### 2.5 TargetSelector + MouseController

与 v1.0 保持一致，选择距画面中心最近且符合类别的目标。MouseController 现在支持三种输出后端，通过统一接口切换：

```cpp
// mouse_controller.hpp
enum class MouseBackend {
    SendInput,        // Windows API (软件，兼容性最好但可检测)
    IbInputSimulator, // 驱动级 (本项目已提供的 DLL)
    KMBoxNet,         // 硬件级 UDP 盒子 (KMBox Net)
};

class MouseController {
public:
    // 选择后端
    bool Init(MouseBackend backend, const std::string& extra_config = "");
    
    // 统一接口：移动鼠标 (Δx, Δy 相对于当前帧中心)
    void MoveMouse(double delta_x, double delta_y, double sensitivity = 1.0);
    
    // 按键控制
    void PressButton(int button);   // 0=左键 1=右键 2=中键
    void ReleaseButton(int button);
    void ClickButton(int button, int hold_ms = 10);
    
    // KMBox 特有
    void MoveMouseAuto(int x, int y, int time_ms);      // 模拟人工轨迹
    void MoveMouseBezier(int x, int y, int ms,           // 贝塞尔曲线
                         int x1, int y1, int x2, int y2);
    void EnableEncryption(bool enable);                  // 加密模式
    void RebootBox();                                    // 重启盒子
    
    // 物理键鼠监控 (KMBox)
    bool IsPhysicalLeftDown();
    bool IsPhysicalRightDown();
    void GetPhysicalMousePos(int* x, int* y);
    
    // 物理键鼠屏蔽 (KMBox)
    void MaskPhysicalMouse(bool mask_x, bool mask_y, bool mask_buttons);

private:
    MouseBackend backend_;
    std::unique_ptr<IbInputSimulator> ib_sim_;   // 驱动级
    std::unique_ptr<KMBoxClient> kmbox_;          // 硬件级
};
```

**KMBox 接口实现**：直接链接 KMBox Net SDK（已在项目中 `kmboxnet-main/c++_demo/NetConfig/`），封装为 `KMBoxClient` 类。

### 2.6 命令行参数 / 启动方式

```bash
# 启动方式: C++ Qt 构建产物
auto_aim_ui.exe --model yolov8n.onnx

# 可选参数:
  --model <path>       模型路径，支持 .onnx / .engine
  --fp16               FP16 推理模式 (默认: ON)
  --crop <size>        截取区域边长 (默认: 640)
  --conf <threshold>   置信度阈值 (默认: 0.5)
  --smooth <factor>    平滑系数 (默认: 1.0)
  --fov <degrees>      水平FOV (默认: 120)
  --headless           纯命令行模式（不启动 UI）
  -h, --help           帮助

示例:
  auto_aim_ui.exe --model yolov8n.onnx                       # v8 (直接加载)
  auto_aim_ui.exe --model yolo11n.onnx --conf 0.6            # v11 + 高置信度
  auto_aim_ui.exe --model my_model.engine                    # 预编译引擎直接启动
```

### 2.7 文件结构

```
Auto_aim/
├── CMakeLists.txt                  # C++ 引擎 DLL + Qt UI 编译
├── DESIGN.md
├── README.md
│
├── src/                            # C++ 引擎源码
│   ├── engine.hpp                  # AimEngine 类定义
│   ├── config.hpp                  # 配置结构体
│   ├── screen_capturer.hpp/cpp     # DXGI 屏幕捕获
│   ├── trt_inferencer.hpp/cpp      # TensorRT 推理（onnx/engine 双兼容）
│   ├── version_detector.hpp/cpp    # YOLO 版本/输出格式自动检测
│   ├── cuda_preprocess.cu/.h       # CUDA 预处理 Kernel
│   ├── cuda_postprocess.cu/.h      # CUDA 后处理 Kernel
│   ├── engine_builder.hpp/cpp      # onnx/engine 双格式直接加载
│   ├── target_selector.hpp/cpp     # 最优目标选择
│   ├── mouse_controller.hpp/cpp    # 鼠标控制（多后端统一接口）
│   ├── kmbox_client.hpp/cpp        # KMBox Net UDP 通信封装
│
├── include/
│   └── InputSimulator.hpp          # IbInputSimulator 头文件
│
├── lib/
│   ├── IbInputSimulator.dll
│   └── kmboxnet_netconfig/         # KMBox Net C++ SDK
│       ├── kmboxNet.h / .cpp
│       ├── HidTable.h
│       └── my_enc.h / .cpp
│
├── models/                         # 用户放置模型（.onnx 或 .engine）
│
├── ui_cpp/                         # C++ Qt6 原生 UI
│   ├── CMakeLists.txt              # Qt UI 独立构建
│   ├── main.cpp                    # 应用入口 main()
│   ├── mainwindow.h / .cpp         # 主窗口（左侧导航 + 右侧内容区）
│   ├── pages/
│   │   ├── modelpage.h / .cpp      # 模型导入页（☁ 云朵+箭头）
│   │   ├── previewpage.h / .cpp    # 画面源预览页（🖥 显示器）
│   │   ├── aimpage.h / .cpp        # 瞄准参数页（🎯 靶心）
│   │   └── settingspage.h / .cpp   # 系统设置页（⚙ 齿轮）
│   ├── widgets/
│   │   ├── dragdroparea.h / .cpp   # 拖拽区域组件
│   │   ├── paramsider.h / .cpp     # 参数滑块组件（滑块+数字框联动）
│   │   └── hotkeybutton.h / .cpp   # 热键绑定按钮
│   └── resources/
│       └── style.qss               # QSS 全局样式表（现代简约白色主题）
│
└── cmake/
    └── FindTensorRT.cmake
```

### 2.8 KMBox Net API 参考

KMBox Net 是来自 `kmboxnet-main` 的 C++ SDK，项目直接链接其源码。

#### 2.8.1 通信协议

```
传输层:  UDP (无连接)
模式:    同步请求-响应 (sendto → recvfrom, 3s 超时)
端口:    盒子显示屏显示 (默认 8888)
加密:    XTEA 变体自定义加密 (可选)
包头:    16 字节 (mac + rand + indexpts + cmd)
数据包:  16~1024 字节 (取决于指令类型)
```

**数据包结构体**：

```cpp
typedef struct {
    unsigned int  mac;       // 盒子 MAC 地址 (8 位 hex → 4 字节)
    unsigned int  rand;      // 随机值 (不同命令复用含义)
    unsigned int  indexpts;  // 序列号 (每次调用递增)
    unsigned int  cmd;       // 命令码 (如 0xaede7345 = 鼠标移动)
} cmd_head_t;

// 鼠标数据体 (64 字节)
typedef struct {
    int button;     // 按键: 0=无 1=左键 2=右键 4=中键
    int x;          // 相对移动 X
    int y;          // 相对移动 Y
    int wheel;      // 滚轮
    int point[10];  // 贝塞尔曲线控制点 (预留 5 阶导)
} soft_mouse_t;

// 联合体发送包
typedef struct {
    cmd_head_t head;
    union {
        cmd_data_t      u8buff;       // 通用字节数组 [1024]
        soft_mouse_t    cmd_mouse;    // 鼠标指令
        soft_keyboard_t cmd_keyboard; // 键盘指令
    };
} client_tx;
```

#### 2.8.2 完整 API 列表

```cpp
// ═══════════ 初始化 ═══════════
int kmNet_init(char* ip, char* port, char* mac);
//   连接盒子。ip/port/mac 见盒子 LCD 显示屏
//   返回 0 = 成功，其他 = 错误码

// ═══════════ 鼠标移动 (核心) ═══════════
int kmNet_mouse_move(short x, short y);
//   相对移动 (Δx, Δy)，立即执行
//   调用延迟 <1ms (局域网)

int kmNet_mouse_move_auto(int x, int y, int time_ms);
//   模拟人工移动，在 time_ms 毫秒内平滑完成
//   自动生成随机中间轨迹点

int kmNet_mouse_move_beizer(int x, int y, int ms,
                             int x1, int y1, int x2, int y2);
//   二阶贝塞尔曲线移动，通过 2 个控制点塑造轨迹

// ═══════════ 鼠标按键 ═══════════
int kmNet_mouse_left(int isdown);     // 1=按下 0=松开
int kmNet_mouse_right(int isdown);
int kmNet_mouse_middle(int isdown);
int kmNet_mouse_side1(int isdown);    // 侧键 1
int kmNet_mouse_side2(int isdown);    // 侧键 2
int kmNet_mouse_wheel(int wheel);     // 滚轮增量
int kmNet_mouse_all(int button, int x, int y, int wheel);
//   一次调用同时控制按键 + 移动 + 滚轮

// ═══════════ 加密模式 API (推荐) ═══════════
// 与上面同名函数一一对应，加 enc_ 前缀
// 使用 XTEA 变体加密防止网络数据包被抓包识别
int kmNet_enc_mouse_move(short x, short y);
int kmNet_enc_mouse_move_auto(int x, int y, int time_ms);
int kmNet_enc_mouse_move_beizer(int x, int y, int ms,
                                 int x1, int y1, int x2, int y2);
int kmNet_enc_mouse_left(int isdown);
int kmNet_enc_mouse_right(int isdown);
int kmNet_enc_mouse_middle(int isdown);
int kmNet_enc_mouse_side1(int isdown);
int kmNet_enc_mouse_side2(int isdown);
int kmNet_enc_mouse_wheel(int wheel);
int kmNet_enc_mouse_all(int button, int x, int y, int wheel);

// ═══════════ 键盘 (可选) ═══════════
int kmNet_keydown(int vkey);    // 按下按键 (HID 键码)
int kmNet_keyup(int vkey);      // 松开按键
int kmNet_keypress(int vk_key, int ms = 10); // 按下 → 等待 ms → 松开
int kmNet_enc_keydown(int vkey);
int kmNet_enc_keyup(int vkey);

// ═══════════ 物理键鼠监控 ═══════════
int kmNet_monitor(short port);
//   port=0 关闭监控，port>0 开启并指定监听端口

int kmNet_monitor_mouse_left();       // 返回 1=物理左键按下
int kmNet_monitor_mouse_right();      //    1=物理右键按下
int kmNet_monitor_mouse_middle();     //    1=物理中键按下
int kmNet_monitor_mouse_side1();      //    1=物理侧键1按下
int kmNet_monitor_mouse_side2();      //    1=物理侧键2按下
int kmNet_monitor_mouse_xy(int* x, int* y);  // 获取物理鼠标坐标
int kmNet_monitor_mouse_wheel(int* wheel);   // 获取滚轮值
int kmNet_monitor_keyboard(short vk_key);    // 1=指定按键按下

// ═══════════ 物理键鼠屏蔽 ═══════════
int kmNet_mask_mouse_left(int enable);    // 屏蔽/恢复左键
int kmNet_mask_mouse_right(int enable);   // 屏蔽/恢复右键
int kmNet_mask_mouse_middle(int enable);  // 屏蔽/恢复中键
int kmNet_mask_mouse_side1(int enable);   // 屏蔽/恢复侧键1
int kmNet_mask_mouse_side2(int enable);   // 屏蔽/恢复侧键2
int kmNet_mask_mouse_x(int enable);       // 屏蔽/恢复 X 轴移动
int kmNet_mask_mouse_y(int enable);       // 屏蔽/恢复 Y 轴移动
int kmNet_mask_mouse_wheel(int enable);   // 屏蔽/恢复滚轮
int kmNet_mask_keyboard(short vkey);      // 屏蔽指定按键
int kmNet_unmask_keyboard(short vkey);    // 恢复指定按键
int kmNet_unmask_all();                   // 解除所有屏蔽

// ═══════════ 配置 / 调试 ═══════════
int kmNet_reboot(void);              // 重启盒子
int kmNet_setconfig(char* ip, unsigned short port);  // 修改盒子 IP/端口
int kmNet_setvidpid(unsigned short vid, unsigned short pid); // 设置 USB VID/PID
int kmNet_debug(short port, char enable); // 调试使能
int kmNet_lcd_color(unsigned short rgb565);  // LCD 纯色填充
int kmNet_lcd_picture(unsigned char* buff_128_160); // LCD 满屏图片
int kmNet_lcd_picture_bottom(unsigned char* buff_128_80); // LCD 下半屏图片

// ═══════════ 错误码 ═══════════
// err_creat_socket  = -9000   // 创建 socket 失败
// err_net_version   = -9001   // socket 版本错误
// err_net_tx        = -9002   // 发送失败
// err_net_rx_timeout= -9003   // 接收超时 (3s)
// err_net_cmd       = -9004   // 响应命令码不匹配
// err_net_pts       = -9005   // 时间戳不匹配
// success           = 0       // 成功
// usb_dev_tx_timeout= 1       // USB 设备端发送超时
```

#### 2.8.3 自瞄集成示例

```cpp
// 典型自瞄帧循环 (C++ 引擎端)
void AimApplication::OnFrame(const std::vector<Detection>& detections) {
    // 1. 选最优目标 (最近画面中心的 person)
    auto target = selector_.SelectBest(detections);
    if (!target) return;
    
    // 2. 计算屏幕像素偏移 (Δx, Δy)
    double dx = (target->center_x - frame_center_x_) * pixels_per_degree_;
    double dy = (target->center_y - frame_center_y_) * pixels_per_degree_;
    
    // 3. 根据后端发送移动指令
    if (mouse_backend_ == MouseBackend::KMBoxNet) {
        // 方案 A: 直接相对移动 (最快)
        kmNet_mouse_move((short)dx, (short)dy);
        
        // 方案 B: 带人类轨迹模拟
        kmNet_mouse_move_auto((short)dx, (short)dy, 50);  // 50ms 内完成
        
        // 方案 C: 贝塞尔曲线 (最自然)
        int cx1 = (int)(dx * 0.3), cy1 = (int)(dy * -0.2);
        int cx2 = (int)(dx * 0.7), cy2 = (int)(dy * 0.3);
        kmNet_mouse_move_beizer((short)dx, (short)dy, 80, cx1, cy1, cx2, cy2);
    }
    
    // 4. 物理按键触发时射击
    if (kmNet_monitor_mouse_left()) {
        kmNet_mouse_left(0);  // 松开 (防止按住不放)
        kmNet_mouse_left(1);  // 单击
        kmNet_mouse_left(0);
    }
}
```

```cpp
// C++ 引擎端连接 KMBox
engine.SetMouseBackend("kmbox", "192.168.2.188", "8888", "01FBC068");
engine.SetConfig("kmbox_encrypt", 1);          // 开启加密
engine.SetConfig("kmbox_bezier", 1);           // 贝塞尔曲线
engine.SetConfig("kmbox_move_time_ms", 50);    // 移动耗时

// 正常推理循环，引擎内部自动用 KMBox 移动鼠标
engine.Start();
```

---

## 三、界面设计（C++ Qt6）

> Python PySide6 UI 已弃用。以下为 C++ Qt6 原生 UI 的完整规范。

### 3.0 设计概述

- **风格**：现代简约风，白色主背景 + 蓝色功能强调色 (#1890FF)
- **布局**：左侧垂直导航栏 (宽约 200px) + 右侧主内容区 (`QStackedWidget`)
- **导航**：4 个图标导航项，自上而下排列，选中态蓝色高亮
- **顶部信息栏**：横向排列系统状态、模型状态、卡密有效期
- **品牌标识**：左侧导航栏顶部显示 "Chimera AI"

#### 颜色体系

| 用途 | 颜色 | 色值 |
|------|------|------|
| 主背景 | 白色 | `#FFFFFF` |
| 卡片/导航背景 | 浅灰 | `#F5F5F5` |
| 功能强调色 | 蓝色 | `#1890FF` |
| 导航选中态图标 | 蓝色 | `#1890FF` |
| 正文文字 | 深灰 | `#333333` |
| 小字说明 | 中灰 | `#888888` |
| 系统运行状态/开关开启 | 绿色 | `#52C41A` |
| 按钮悬停 | 浅蓝 | `#40A9FF` |
| 按钮点击 | 深蓝 | `#096DD9` |

#### 字体与排版

| 层级 | 字号 | 字体 |
|------|------|------|
| 标题 | 16-18px | 无衬线 (微软雅黑 / PingFang SC) |
| 正文 | 14px | 无衬线 |
| 小字说明 | 12px | 无衬线 |
| 卡片内边距 | 20px | — |
| 模块间上下间距 | 24px | — |

---

### 3.1 整体布局

```
┌──────────────────────────────────────────────────────────────────┐
│  🔵 系统运行中    ⬜ 尚未导入模型    📅 卡密有效期 2026-03-10 剩余3天  │  ← 顶部全局信息栏
├────────┬─────────────────────────────────────────────────────────┤
│Chimera │                                                         │
│  AI    │                 右侧主内容区 (QStackedWidget)              │
│        │                                                         │
│ ⚙ 设置  │   页面 1: 模型导入页 (☁ 云朵+箭头 导航激活)               │
│        │      · 导入模型卡片（拖拽区 + 选择文件按钮）                 │
│ 🖥 画面  │      · 当前模型卡片（已加载模型文件名）                     │
│        │      · 推理参数卡片（置信度/最大检测数/类别过滤）             │
│ 🎯 瞄准  │                                                         │
│        │   页面 2: 画面源预览页 (🖥 显示器 导航激活)                  │
│ ☁ 模型   │      · 顶部状态栏（模型名 + 操作按钮）                     │
│ ←蓝色    │      · 画面源预览卡片（实时预览/占位引导）                  │
│ ↑当前    │      · 输入模式卡片（SendInput 单选）                     │
│        │      · 显示设置卡片（开关组）                               │
│        │                                                         │
│        │   页面 3: 鼠标曲线/瞄准参数页 (🎯 靶心 导航激活)             │
│        │      · 鼠标曲线输入方案卡片（Kp/Ki/卡尔曼/延迟补偿）          │
│        │      · 瞄准设置卡片（范围/布局/部位 + 人体示意图）            │
│        │      · 热键绑定卡片（自瞄开关/切换目标）                     │
│        │                                                         │
│        │   页面 4: 系统设置页 (⚙ 齿轮 导航激活)                      │
│        │      · 通用设置 / 检查更新 / 关于                          │
└────────┴─────────────────────────────────────────────────────────┘
```

**顶部全局信息栏**（固定在所有页面上方）：
- 🟢 系统运行中 — 绿色圆点 + 文字，表示服务正常
- ⬜ 尚未导入模型 — 灰色圆点 + 文字，加载后替换为模型名
- 📅 卡密有效期 2026-03-10 00:05 剩余3天 — 日历图标 + 剩余天数提示

**左侧导航栏** (4 项，从上到下)：

| 图标 | 名称 | 对应页面 | 对应 UI 文件 |
|------|------|---------|------------|
| ⚙ 齿轮 | 系统设置 | SettingsPage | `settingspage.h/.cpp` |
| 🖥 显示器 | 画面源预览 | PreviewPage | `previewpage.h/.cpp` |
| 🎯 靶心 | 瞄准参数 | AimPage | `aimpage.h/.cpp` |
| ☁ 云朵+箭头 | 模型导入 | ModelPage | `modelpage.h/.cpp` |

---

### 3.2 分页面详细设计

使用 `QStackedWidget` + `QButtonGroup` 实现四页导航切换。

---

#### 页面 1：模型导入页（☁ 云朵+箭头 激活）

```
┌─────────────────────────────────────────────────────────────┐
│  📁 导入模型                                                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                                                       │  │
│  │                      ☁                                │  │
│  │        拖拽 .onnx 或 .engine 文件到此处                  │  │
│  │          （模型和路径不要出现中文）                        │  │
│  │                                                       │  │
│  │                 [ 选择文件 ]  ← 蓝色圆角按钮              │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  📦 当前模型                                                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │              未加载模型（浅灰背景，文字居中）              │  │
│  │   [加载成功后替换为: CS2.onnx]                          │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  ⚙ 推理参数                                                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  置信度阈值    ━━━━━━━━━━○────────  0.50               │  │
│  │  最大检测数    ━━━━━━━━━━○────────  10                 │  │
│  │  类别过滤     [全部类别         ▼]                      │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**组件树**：

```
ModelPage (QWidget)
├── QGroupBox "📁 导入模型"
│   ├── DragDropArea (自定义 QWidget)
│   │   ├── QLabel 云图标 (居中大图标)
│   │   ├── QLabel "拖拽 .onnx 或 .engine 文件到此处"
│   │   ├── QLabel "(模型和路径不要出现中文)" (12px 灰色小字)
│   │   └── QPushButton "选择文件" (蓝色圆角 #1890FF)
├── QGroupBox "📦 当前模型"
│   ├── QLabel 模型文件名 (未加载: "未加载模型" 灰色居中)
│   └── QLabel 模型附加信息 (加载后显示)
└── QGroupBox "⚙ 推理参数"
    ├── ParamSlider "置信度阈值" (0.01~1.00, 默认 0.50)
    ├── ParamSlider "最大检测数" (1~100, 默认 10, 整数步进)
    └── QComboBox "类别过滤" (默认 "全部类别")
```

**交互**：
- **拖拽加载**：`DragDropArea` 重写 `dragEnterEvent` / `dropEvent`，检测 `.onnx` / `.engine` 扩展名；文件悬停时边框变蓝，释放后触发加载
- **选择文件**：`QFileDialog::getOpenFileName(filter="*.onnx *.engine")`
- **中文检测**：模型路径包含中文字符时弹窗警告并拒绝加载
- **滑块同步**：拖动滑块时右侧数字框实时更新，数字框手动输入时滑块同步

---

#### 页面 2：画面源预览页（🖥 显示器 激活）

```
┌─────────────────────────────────────────────────────────────┐
│  ✅ CS2.onnx      [ 开始推理 ]  [ 自瞄范围大小 ]  [ 推流接收 ]  │ ← 顶部状态栏
│                                                             │
│  🖥 画面源预览                                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                                                       │  │
│  │              👁‍🗨 预览窗口已关闭                          │  │
│  │         点击「调试窗口」按钮开启实时预览                   │  │
│  │                                                       │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  ⌨ 输入模式                                                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  ● 标准 SendInput                                      │  │
│  │    使用原生 Windows API，部分游戏可能会屏蔽该输入方式        │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  🖥 显示设置                                                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  显示推理延迟          [────● 绿色开关]                  │  │
│  │  独立显示调试窗口       [────○ 灰色关闭]                  │  │
│  │  绘制检测框            [────● 绿色开启]                  │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**组件树**：

```
PreviewPage (QWidget)
├── QWidget 顶部状态栏
│   ├── QLabel "✅ CS2.onnx" (绿色对勾 + 模型名)
│   ├── QPushButton "开始推理" (蓝色)
│   ├── QPushButton "自瞄范围大小"
│   └── QPushButton "推流接收"
├── QGroupBox "🖥 画面源预览"
│   ├── QLabel 预览区域
│   │   ├── 默认: 眼睛+斜线图标 + "预览窗口已关闭" + 引导提示
│   │   └── 调试窗口开启后: QImage 实时渲染画面
│   └── QPushButton "调试窗口"
├── QGroupBox "⌨ 输入模式"
│   ├── QRadioButton "标准 SendInput" (默认选中)
│   │   └── QLabel "(使用原生 Windows API，部分游戏可能会屏蔽该输入方式)"
│   ├── QRadioButton "IbInputSimulator (驱动级)"
│   │   └── QLabel "(内核驱动注入，兼容性更好)"
│   └── QRadioButton "KMBox Net ★"
│       └── QLabel "(硬件 USB HID 模拟，不可检测，支持双机部署)"
│       └── QGroupBox "KMBox 连接设置" (KMBox 选中时展开)
│           ├── QLineEdit "盒子 IP" (占位 "192.168.2.188")
│           ├── QLineEdit "端口" (占位 "8888")
│           ├── QLineEdit "MAC 地址" (占位 "01FBC068")
│           ├── QLabel 连接状态 (●已连接 / ○未连接)
│           ├── QPushButton "测试连接"
│           ├── QCheckBox "启用加密传输"
│           ├── QCheckBox "启用贝塞尔曲线移动"
│           └── QPushButton "重启盒子"
└── QGroupBox "🖥 显示设置"
    ├── QLabel "显示推理延迟" + QSwitch
    ├── QLabel "独立显示调试窗口" + QSwitch (默认关)
    └── QLabel "绘制检测框" + QSwitch (默认开)
```

**画面渲染**（C++ Qt，引擎帧回调直接更新）：

```cpp
void PreviewPage::updateFrame(const cv::Mat& frameBgr) {
    if (frameBgr.empty()) return;
    QImage qimg(frameBgr.data, frameBgr.cols, frameBgr.rows,
                static_cast<int>(frameBgr.step), QImage::Format_BGR888);
    imageLabel_->setPixmap(QPixmap::fromImage(qimg).scaled(
        imageLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
```

---

#### 页面 3：鼠标曲线/瞄准参数页（🎯 靶心 激活）

```
┌─────────────────────────────────────────────────────────────┐
│  🖱 鼠标曲线输入方案                                           │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  PD 比例增益 (Kp)    ━━━━━━━━━━○────────  0.250        │  │
│  │  PID 积分增益 (Ki)   ━━━━━━━━━━○────────  0.0008       │  │
│  │  卡尔曼预测          ━━━━━━━━━━○────────  0 ms         │  │
│  │  延迟矫正补偿         ━━━━━━━━━━○────────  5 ms         │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  🎯 瞄准设置                                                  │
│  ┌──────────────────────────────────┬────────────────────┐  │
│  │  范围大小    ━━━━━━○─────  21 %   │    [人体示意图]      │  │
│  │  范围布局    ● 圆形  ○ 矩形      │      ○ ← 头部蓝圈    │  │
│  │                                  │     /|\             │  │
│  │  对准部位   [头部] [胸部] [腹部]  │     / \             │  │
│  │                  ↑蓝色选中                                │  │
│  └──────────────────────────────────┴────────────────────┘  │
│                                                             │
│  ⌨ 热键绑定                                                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  自瞄开关    [ 鼠标右键 ]        ← 点击可修改热键        │  │
│  │  切换目标    [ 鼠标侧键1 ]       ← 点击可修改热键        │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**组件树**：

```
AimPage (QWidget)
├── QGroupBox "🖱 鼠标曲线输入方案"
│   ├── ParamSlider "PD 比例增益 (Kp)" (0.0~1.0, 默认 0.250)
│   ├── ParamSlider "PID 积分增益 (Ki)" (0.0~0.01, 默认 0.0008, 四位小数)
│   ├── ParamSlider "卡尔曼预测" (0~200, 默认 0, 单位 ms)
│   └── ParamSlider "延迟矫正补偿" (0~50, 默认 5, 单位 ms)
├── QGroupBox "🎯 瞄准设置"
│   ├── QHBoxLayout 主内容区
│   │   ├── QVBoxLayout 左侧参数
│   │   │   ├── ParamSlider "范围大小" (1~100, 默认 21, 单位 %)
│   │   │   ├── QLabel "范围布局"
│   │   │   │   └── QRadioButton "圆形" (默认) | QRadioButton "矩形"
│   │   │   └── QLabel "对准部位"
│   │   │       └── QHBoxLayout: QPushButton "对准头部" | "对准胸部" | "对准腹部"
│   │   │           (选中按钮高亮蓝色, 同组互斥)
│   │   └── QWidget 右侧人体示意图
│   │       └── 用 QPainter 绘制人体轮廓 + 蓝色圆圈标注当前瞄准部位
└── QGroupBox "⌨ 热键绑定"
    ├── HotkeyButton "自瞄开关" (默认显示 "鼠标右键")
    └── HotkeyButton "切换目标" (默认显示 "鼠标侧键1")
```

**人体示意图** (`QPainter` 绘制，右侧展示)：

```cpp
void AimPage::paintBodyDiagram(QPainter& painter) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#333333"), 2));
    // 头部
    painter.drawEllipse(headRect);
    // 躯干
    painter.drawLine(neckPoint, waistPoint);
    painter.drawLine(shoulderLeft, shoulderRight);
    // 当前瞄准部位 — 蓝色圆圈高亮
    painter.setBrush(QColor("#1890FF"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(aimSpot);  // 直径约 20px
}
```

---

#### 页面 4：系统设置页（⚙ 齿轮 激活）

```
┌─────────────────────────────────────────────────────────────┐
│  ⚙ 通用设置                                                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  开机自启              [────○ 灰色关闭]                  │  │
│  │  最小化到托盘           [────● 绿色开启]                  │  │
│  │  语言                 [简体中文        ▼]               │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  🔄 检查更新                                                 │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  当前版本: v2.0.0                                      │  │
│  │  [ 检查更新 ]                                           │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  ℹ 关于                                                     │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Chimera AI v2.0                                       │  │
│  │  Built with Qt6 + TensorRT + CUDA                      │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**组件树**：

```
SettingsPage (QWidget)
├── QGroupBox "⚙ 通用设置"
│   ├── QLabel "开机自启" + QSwitch (默认关)
│   ├── QLabel "最小化到托盘" + QSwitch (默认开)
│   └── QLabel "语言" + QComboBox (简体中文 / English)
├── QGroupBox "🔄 检查更新"
│   ├── QLabel "当前版本: v2.0.0"
│   └── QPushButton "检查更新"
└── QGroupBox "ℹ 关于"
    └── QLabel "Chimera AI v2.0 — Built with Qt6 + TensorRT + CUDA"
```

---

### 3.3 QSS 视觉样式

**样式文件**：`ui_cpp/resources/style.qss`

**主题**：现代简约白色主题，蓝色 (#1890FF) 主色调

**核心 QSS 定义**：

```css
/* 全局 */
* { font-family: "Microsoft YaHei", "PingFang SC", sans-serif; }

/* 主窗口 */
QMainWindow { background: #FFFFFF; }

/* 左侧导航栏 */
#navBar {
    background: #F5F5F5;
    min-width: 200px;
    max-width: 200px;
}
#navBrand {
    font-size: 18px;
    font-weight: bold;
    color: #1890FF;
    padding: 20px;
}
#navItem {
    padding: 12px 20px;
    color: #888888;
    font-size: 14px;
    border-left: 3px solid transparent;
}
#navItem:hover {
    background: #E8E8E8;
    color: #333333;
}
#navItem[active="true"] {
    color: #1890FF;
    background: #E6F7FF;
    border-left: 3px solid #1890FF;
}

/* 顶部全局信息栏 */
#topInfoBar {
    background: #FAFAFA;
    border-bottom: 1px solid #E8E8E8;
    padding: 8px 20px;
}

/* 卡片 */
QGroupBox {
    background: #FFFFFF;
    border: 1px solid #E8E8E8;
    border-radius: 8px;
    margin-top: 16px;
    padding: 20px;
    font-size: 16px;
    font-weight: bold;
    color: #333333;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 20px;
    padding: 0 5px;
}

/* 主按钮 */
QPushButton#btnPrimary {
    background: #1890FF;
    color: white;
    border: none;
    border-radius: 6px;
    padding: 8px 24px;
    font-size: 14px;
}
QPushButton#btnPrimary:hover { background: #40A9FF; }
QPushButton#btnPrimary:pressed { background: #096DD9; }

/* 拖拽区域 */
#dragDropArea {
    border: 2px dashed #D9D9D9;
    border-radius: 8px;
    background: #FAFAFA;
}
#dragDropArea:hover {
    border-color: #1890FF;
    background: #E6F7FF;
}

/* 滑块 */
QSlider::groove:horizontal {
    height: 6px;
    background: #E8E8E8;
    border-radius: 3px;
}
QSlider::sub-page:horizontal {
    background: #1890FF;
    border-radius: 3px;
}
QSlider::handle:horizontal {
    width: 16px;
    height: 16px;
    background: white;
    border: 2px solid #1890FF;
    border-radius: 8px;
    margin: -6px 0;
}

/* 开关 */
#switch[checked="true"] { background: #52C41A; }
#switch[checked="false"] { background: #D9D9D9; }

/* 状态标签 */
#statusRunning { color: #52C41A; }
#statusNoModel { color: #888888; }
#statusError { color: #FF4D4F; }
```

---

### 3.4 组件设计规范

#### ParamSlider（参数滑块组件）

```
┌── 置信度阈值 ────────────── [━━━━━━○──────] [0.50] ──┐
   QLabel (120px)      QSlider (弹性)  QDoubleSpinBox (70px)
```

- 左侧标签固定宽度 120-140px
- 滑块范围映射到 `min_` ~ `max_`（内部用整数 0~1000，显示用浮点）
- 右侧数字框宽度 70px，可手动编辑
- **双向绑定**：滑块拖动 → 数字框同步；数字框输入 → 滑块同步
- 值变更通过信号通知 `MainWindow` → `engine.SetConfig()`

#### HotkeyButton（热键绑定按钮）

- 继承 `QPushButton`，显示当前绑定按键名
- 点击进入监听模式：按钮变红闪烁，文字变为 "按下按键..."
- 捕获键盘/鼠标事件，显示按键名
- 3 秒超时自动取消监听

#### 开关 (QSwitch)

- 自定义 `QWidget`，用 `QPainter` 绘制
- 两种状态：绿色 (#52C41A) = 开启，灰色 (#D9D9D9) = 关闭
- 点击时平滑过渡动画 (QPropertyAnimation, 200ms)
- 参考尺寸：宽 44px，高 22px，圆角 11px

#### DragDropArea（拖拽区域）

- 继承 `QWidget`，重写 `dragEnterEvent` / `dragLeaveEvent` / `dropEvent`
- 默认显示：虚线边框 (#D9D9D9) + 云图标 + 提示文字
- 文件悬停：边框变蓝 (#1890FF) + 背景浅蓝 (#E6F7FF)
- 释放：触发 `fileDropped(QString path)` 信号 → `MainWindow` → 引擎加载

---

### 3.5 推理线程架构

```
┌── UI 线程 (C++ Qt6, 主线程) ──────────────────────────┐
│  Qt 事件循环 · QSS 渲染 · 用户交互                      │
│  ↓ Qt 信号槽 (QueuedConnection)                         │
│  ModelPage→modelLoaded(path)  ←──→  SetConfig()        │
│  PreviewPage→startClicked()                            │
└─────────┼──────────────────────────────────────────────┘
          │
┌─────────▼── 推理子线程 (std::thread, C++) ────────────┐
│  每帧循环:                                              │
│  DXGI 截图 → GPU 预处理 → TensorRT 推理                │
│  → 目标选择 → [按键按下] → 鼠标移动                      │
│                                                        │
│  FrameCallback 通知 UI:                                │
│  engine.SetFrameCallback([](const cv::Mat& f) {        │
│    QMetaObject::invokeMethod(receiver, [f]{             │
│      preview->updateFrame(f);                          │
│    }, Qt::QueuedConnection);                            │
│  });                                                    │
└────────────────────────────────────────────────────────┘
```

**关键**：
- 推理在独立 `std::thread`（不参与 Qt 事件循环）
- 帧回调通过 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 安全跨线程传递 `cv::Mat`
- 模型加载通过 `QThread` 在后台线程执行，UI 不会卡死
- 性能统计用 `QTimer(100ms)` 定时轮询 `engine.GetStats()`

---

### 3.6 状态机

```
[启动 auto_aim_ui.exe] → AimEngine 初始化 → Qt6 MainWindow 显示
                        ↓
             用户拖入/选择 .onnx 或 .engine
                        ↓
              ┌─ .engine? → 检测 SM 兼容性
              │   ├─ 兼容 → 直接反序列化加载（瞬间）
              │   └─ 不兼容 → 弹窗提示使用 .onnx
              └─ .onnx?   → nvonnxparser 直接解析（运行时自动适配当前 GPU）
                        ↓
                  模型就绪（顶部状态栏变绿，显示模型名）
                        ↓
               用户调整参数 → slider 实时推送给引擎
                        ↓
           点击 [▶ 开始推理] → engine.Start() → std::thread
                        ↓
       DXGI截图 → GPU预处理 → TensorRT推理 → GPU后处理
                    ↓
       FrameCallback → QMetaObject::invokeMethod → 实时画面
                    ↓
            目标选择 → [按键按下] → 鼠标移动
                        ↓
        [⏹ 停止] 或关闭窗口 → engine.Stop() → join 线程
```

---

### 3.7 交互反馈规范

| 元素 | 默认 | 悬停 (hover) | 点击/激活 |
|------|------|-------------|----------|
| 主按钮 | 蓝 `#1890FF` | 浅蓝 `#40A9FF` | 深蓝 `#096DD9` |
| 导航项 (未选) | 灰字 `#888888` | 浅灰底 `#E8E8E8` | — |
| 导航项 (选中) | 蓝字 `#1890FF` + 左侧蓝条 + 浅蓝底 `#E6F7FF` | — | — |
| HotkeyButton | 显示按键名 | 边框变蓝 | 红色闪烁 "按下按键..." |
| 拖拽区边框 | 虚线 `#D9D9D9` | 蓝色实线 `#1890FF` | 释放后触发加载 |
| 滑块轨道 | 灰色 `#E8E8E8` | — | 蓝色已过区域 `#1890FF` |
| 开关 (开) | 绿 `#52C41A` | — | 平滑过渡到灰色 |
| 开关 (关) | 灰 `#D9D9D9` | — | 平滑过渡到绿色 |
| 状态标签 (运行中) | 绿 `#52C41A` | — | — |
| 状态标签 (未加载) | 灰 `#888888` | — | — |
| 状态标签 (错误) | 红 `#FF4D4F` | — | — |

---

## 四、性能预期

基于 YOLOv8n，RTX 3060，FP16，640×640 输入：

| 环节 | v1.0 ONNX RT | v2.0 TRT C++ API | 提升 |
|------|-------------|-----------------|------|
| 屏幕捕获 (DXGI) | ~1-2ms | ~1-2ms | - |
| 预处理 (BGRA→RGB+Resize+LetterBox) | ~3-5ms (CPU) | ~0.1ms (GPU) | 30-50x |
| 推理 | ~3-5ms | ~2-3ms | ~1.5x |
| 后处理 (解码+NMS) | ~1-2ms (CPU) | ~0.2ms (GPU) | 5-10x |
| **总延迟** | **~8-14ms** | **~3-7ms** | **~50%** |

---

## 五、分版本实现路线图

> 三个版本递进：**v2.0 最小可用 → v2.1 好用增强 → v2.2 极致完整**

---

### v2.0 — 核心闭环（MVP，目标：能跑通）

**一句话目标**：从 ONNX 模型加载到屏幕捕获到推理到鼠标移动，端到端跑通。

| 编号 | 模块 | 内容 | 产出标准 |
|------|------|------|---------|
| V1 | **CMake 工程** | CMake 构建系统 + TensorRT/CUDA/OpenCV 环境配置 | `cmake --build` 通过 |
| V2 | **ModelLoader** | `.onnx` → `nvonnxparser` 直接解析加载，暂不支持 `.engine` | 成功加载模型，输出 engine |
| V3 | **TRTInferencer** | TensorRT 推理 + **CPU 预处理/后处理**（复用 v1.0 的 OpenCV 方式） | 单帧推理正确，输出检测框 |
| V4 | **DXGI Capturer** | DXGI Desktop Duplication 截屏中心区域 | 稳定 60+ FPS 捕获 |
| V5 | **TargetSelector** | 选距画面中心最近、符合类别的目标 | 给定检测列表，正确选出最优目标 |
| V6 | **MouseController** | 仅 **SendInput + IbInputSimulator** 两个后端，统一接口 | 鼠标按 Δx/Δy 移动 |
| V7 | **AimEngine** | `src/engine.hpp` 主循环协调器：截图→推理→选目标→移动鼠标 | 按住按键自瞄闭环可运行 |
| V8 | **Qt6 最简 UI** | 左侧导航栏 + 4 页面骨架（模型导入页+预览页占位+瞄准页占位+设置页），仅模型导入页的拖拽/选择文件 + 置信度滑块 + 开始/停止按钮可用 | 可用 UI 操作替代命令行 |

**v2.0 交付物**：一个带最简 Qt 界面的 `.exe`，用户拖入 `.onnx` 即可自瞄。4 页面框架已搭好，但仅模型导入页有完整交互。

> ⚠️ v2.0 暂不做：`.engine` 格式、CUDA 预处理/后处理、KMBox、VersionDetector、多页面完整交互

---

### v2.1 — 性能 + 体验增强（目标：好用）

**一句话目标**：GPU 全管线加速 + `.engine` 支持 + 完整 Qt6 三页面 UI。

| 编号 | 模块 | 内容 | 产出标准 |
|------|------|------|---------|
| V9 | **CUDA 预处理** | `cuda_preprocess.cu`：BGRA→RGB+Resize+LetterBox+Normalize 全 GPU | 预处理从 3-5ms 降至 ~0.1ms |
| V10 | **`.engine` 支持** | `ModelLoader` 加入 `.engine` 反序列化加载 + SM 兼容性检测 | 两种格式都能加载 |
| V11 | **VersionDetector** | 自动识别 YOLOv8~v26 + 输出格式 (Native/PassThrough) | 拖入 ONNX 自动识别版本 |
| V12 | **CUDA 后处理（Native）** | `cuda_postprocess.cu`：GPU 端 Anchor-free 解码 + NMS | 后处理从 1-2ms 降至 ~0.2ms |
| V13 | **Qt6 完整 4 页面 UI** | 模型导入页（拖拽区+滑块联动）+ 预览页（QImage 实时渲染+输入模式）+ 瞄准参数页（ParamSlider + HotkeyButton + 人体示意图）+ 设置页 | 4 页面完整交互，导航切换流畅 |
| V14 | **推理线程** | `std::thread` + `QMetaObject::invokeMethod` UI 通信 | 推理不卡 UI |
| V15 | **命令行参数** | `--model` `--conf` `--headless` 等 | 无 UI 模式也可用 |
| V16 | **配置文件持久化** | JSON 配置读写，启动恢复上次设置 | 关掉重开参数不丢失 |

**v2.1 交付物**：完整 4 页面 Qt6 UI + GPU 全管线 + `.onnx`/`.engine` 双格式，总延迟 3-7ms。

> ⚠️ v2.1 暂不做：KMBox、PassThrough 后处理、性能监控面板、打包

---

### v2.2 — 硬件级 + 极致体验（目标：完整）

**一句话目标**：KMBox 硬件支持 + PassThrough 零后处理 + 性能面板 + 打包发布。

| 编号 | 模块 | 内容 | 产出标准 |
|------|------|------|---------|
| V17 | **KMBoxClient** | 封装 kmboxNet SDK → 统一 `MouseController` 接口 | KMBox 后端 Switch 切换即用 |
| V18 | **KMBox UI 面板** | 预览页中加入 IP/端口/MAC 配置、连接状态、加密/贝塞尔开关 | 界面上配网即连 |
| V19 | **KMBox 高级功能** | 贝塞尔曲线移动、人工轨迹模拟 (AutoMove)、物理键鼠监控/屏蔽 | 完整 KMBox API 覆盖 |
| V20 | **PassThrough 后处理** | `.engine` 含 EfficientIdxNMS 时跳过解码，直接 memcpy | 含 NMS 的 engine 后处理 ~0ms |
| V21 | **性能监控面板** | 预处理/推理/后处理 耗时进度条，FPS，GPU 显存 | 可视化实时性能 |
| V22 | **打包发布** | CMake install + NSIS/MSI 安装包 | 用户一个安装包搞定 |
| V23 | **Benchmark** | 多 GPU/多模型性能测试报告 | 量化性能数据 |

**v2.2 交付物**：完整功能安装包，覆盖所有鼠标后端 + 所有后处理路径 + 性能监控。

---

### 版本总结

```
v2.0 (MVP)          v2.1 (增强)           v2.2 (完整)
══════════════      ══════════════        ══════════════
.onnx only    →     + .engine + SM检测    + PassThrough
CPU 前后处理  →     GPU 全管线             + 性能面板
2 种鼠标      →     2 种                   + KMBox 硬件
4 页骨架 UI   →     完整 4 页面 UI          + 打包发布
闭环可跑      →     延迟 3-7ms             + 人类轨迹模拟
```
