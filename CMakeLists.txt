cmake_minimum_required(VERSION 3.18)
project(AutoAim VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ============================================================
# Paths — 第三方 SDK（相对于项目根目录）
# ============================================================
set(TRT_ROOT          ${CMAKE_SOURCE_DIR}/TensorRT-10.16.1.11.Windows.amd64.cuda-13.2/TensorRT-10.16.1.11)
set(ONNXRUNTIME_ROOT  ${CMAKE_SOURCE_DIR}/onnxruntime)
set(OPENCV_ROOT       ${CMAKE_SOURCE_DIR}/opencv)
set(CUDA_ROOT         "${CMAKE_SOURCE_DIR}/NVIDIA GPU Computing Toolkit/CUDA/v13.3")
set(KMBOX_ROOT        ${CMAKE_SOURCE_DIR}/kmboxnet-main/c++_demo/NetConfig)

# ============================================================
# TensorRT
# ============================================================
set(TRT_INCLUDE_DIR ${TRT_ROOT}/include)
set(TRT_LIB_DIR     ${TRT_ROOT}/lib)

# ============================================================
# OpenCV
# ============================================================
set(OpenCV_INCLUDE_DIRS ${OPENCV_ROOT}/include)
set(OpenCV_LIBS
    ${OPENCV_ROOT}/x64/vc16/lib/opencv_world4110.lib
)

# ============================================================
# CUDA (link directly — no .cu files to compile)
# ============================================================
set(CUDA_INCLUDE_DIR "${CUDA_ROOT}/include")
set(CUDA_LIB_DIR     "${CUDA_ROOT}/lib/x64")

# ============================================================
# ====== 引擎静态库 (v2.0 MVP) ======
# ============================================================
set(ENGINE_SOURCES
    src/screen_capturer.cpp
    src/object_detector.cpp
    src/target_selector.cpp
    src/mouse_controller.cpp
    src/aim_engine.cpp
    src/config_manager.cpp
    src/hotkey_manager.cpp
    src/pid_controller.cpp
    src/kalman_tracker.cpp
    ${KMBOX_ROOT}/kmboxNet.cpp
    ${KMBOX_ROOT}/my_enc.cpp
)

set(ENGINE_HEADERS
    src/config.hpp
    src/engine.hpp
    src/config_manager.hpp
    src/hotkey_manager.hpp
    src/screen_capturer.hpp
    src/object_detector.hpp
    src/target_selector.hpp
    src/mouse_controller.hpp
    src/pid_controller.hpp
    src/kalman_tracker.hpp
)

add_library(aim_engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
target_include_directories(aim_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
    ${TRT_INCLUDE_DIR}
    ${ONNXRUNTIME_ROOT}/include
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIR}
    ${KMBOX_ROOT}
)
target_compile_definitions(aim_engine PRIVATE NOMINMAX)
target_compile_options(aim_engine PRIVATE /utf-8)
target_link_libraries(aim_engine PUBLIC
    # TensorRT 10.x
    ${TRT_LIB_DIR}/nvinfer_10.lib
    ${TRT_LIB_DIR}/nvonnxparser_10.lib
    ${TRT_LIB_DIR}/nvinfer_dispatch_10.lib
    # ONNX Runtime
    ${ONNXRUNTIME_ROOT}/lib/onnxruntime.lib
    # OpenCV
    ${OpenCV_LIBS}
    # CUDA runtime
    ${CUDA_LIB_DIR}/cudart.lib
    # Windows
    ws2_32 dxgi d3d11 dxguid
)

# ============================================================
# ====== Qt6 UI 可执行文件 (v2.1 完整 4 页面 UI) ======
# ============================================================
find_package(Qt6 QUIET COMPONENTS Widgets Core Gui Concurrent)
if(Qt6_FOUND)
    message(STATUS "Qt6 found, building auto_aim_ui.exe")

    set(UI_SOURCES
        ui_cpp/main.cpp
        ui_cpp/mainwindow.cpp
        # 页面
        ui_cpp/pages/modelpage.cpp
        ui_cpp/pages/previewpage.cpp
        ui_cpp/pages/aimpage.cpp
        ui_cpp/pages/settingspage.cpp
        # 自定义组件
        ui_cpp/widgets/dragdroparea.cpp
        ui_cpp/widgets/paramslider.cpp
        ui_cpp/widgets/hotkeycapture.cpp
        ui_cpp/widgets/previewwindow.cpp
    )

    set(UI_HEADERS
        ui_cpp/mainwindow.h
        ui_cpp/pages/modelpage.h
        ui_cpp/pages/previewpage.h
        ui_cpp/pages/aimpage.h
        ui_cpp/pages/settingspage.h
        ui_cpp/widgets/dragdroparea.h
        ui_cpp/widgets/paramslider.h
        ui_cpp/widgets/hotkeycapture.h
        ui_cpp/widgets/previewwindow.h
    )

    qt_add_resources(UI_RESOURCES ui_cpp/resources.qrc)

    add_executable(auto_aim_ui WIN32 ${UI_SOURCES} ${UI_HEADERS} ${UI_RESOURCES})
    target_include_directories(auto_aim_ui PRIVATE
        ${CMAKE_SOURCE_DIR}/ui_cpp
        ${CMAKE_SOURCE_DIR}
    )
    target_compile_definitions(auto_aim_ui PRIVATE NOMINMAX)
    target_compile_options(auto_aim_ui PRIVATE /utf-8)
    target_link_libraries(auto_aim_ui PRIVATE
        Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Concurrent
        aim_engine
    )
    # 将运行时 DLL 复制到输出目录
    add_custom_command(TARGET auto_aim_ui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPENCV_ROOT}/x64/vc16/bin/opencv_world4110.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/IbInputSimulator.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvonnxparser_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_dispatch_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_ptx_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        # 复制所有 SM 架构 builder resource DLL（按需加载）
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm75_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm80_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm86_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm89_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm90_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm100_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm120_10.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CUDA_ROOT}/bin/x64/cudart64_13.dll
            $<TARGET_FILE_DIR:auto_aim_ui>
        # 使用 windeployqt 自动部署 Qt6 依赖
        COMMAND C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/windeployqt.exe
            --no-translations --no-compiler-runtime
            $<TARGET_FILE:auto_aim_ui>
    )
else()
    message(WARNING "Qt6 not found — UI executable will NOT be built. Only aim_engine.lib will be compiled.")
endif()

# ============================================================
# ====== QML UI 可执行文件 (v2.1 QML 深色主题) ======
# ============================================================
find_package(Qt6 QUIET COMPONENTS Quick QuickControls2 Qml Svg)
if(Qt6_FOUND AND Qt6Quick_FOUND)
    message(STATUS "Qt6 Quick found, building auto_aim_qml.exe (QML UI)")

    set(QML_UI_SOURCES
        ui_qml/cpp/main.cpp
        ui_qml/cpp/engineviewmodel.cpp
        ui_qml/cpp/configviewmodel.cpp
        ui_qml/cpp/frameprovider.cpp
        ui_qml/cpp/logindialogcontroller.cpp
    )

    set(QML_UI_HEADERS
        ui_qml/cpp/engineviewmodel.h
        ui_qml/cpp/configviewmodel.h
        ui_qml/cpp/frameprovider.h
        ui_qml/cpp/logindialogcontroller.h
        ui_qml/cpp/hotkeycapturedispatcher.h
    )

    qt_add_resources(QML_UI_RESOURCES ui_qml/qml_resources.qrc)

    add_executable(auto_aim_qml WIN32 ${QML_UI_SOURCES} ${QML_UI_HEADERS} ${QML_UI_RESOURCES})
    target_include_directories(auto_aim_qml PRIVATE
        ${CMAKE_SOURCE_DIR}/ui_qml/cpp
        ${CMAKE_SOURCE_DIR}
    )
    target_compile_definitions(auto_aim_qml PRIVATE NOMINMAX)
    target_compile_options(auto_aim_qml PRIVATE /utf-8)
    target_link_libraries(auto_aim_qml PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Qml
        Qt6::Svg
        aim_engine
    )

    # 将运行时 DLL 复制到输出目录
    add_custom_command(TARGET auto_aim_qml POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPENCV_ROOT}/x64/vc16/bin/opencv_world4110.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/IbInputSimulator.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvonnxparser_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_dispatch_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_ptx_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm75_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm80_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm86_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm89_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm90_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm100_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TRT_ROOT}/bin/nvinfer_builder_resource_sm120_10.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CUDA_ROOT}/bin/x64/cudart64_13.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        # Qt Quick Controls 风格 DLL（windeployqt 遗漏）
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickControls2Fusion.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickControls2FusionStyleImpl.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickControls2Basic.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickControls2BasicStyleImpl.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickControls2Impl.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickLayouts.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickDialogs2.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickDialogs2QuickImpl.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/Qt6QuickDialogs2Utils.dll
            $<TARGET_FILE_DIR:auto_aim_qml>
        # 使用 windeployqt 自动部署 Qt6 依赖
        COMMAND C:/Qt/6.8.2/6.8.2/msvc2022_64/bin/windeployqt.exe
            --no-translations --no-compiler-runtime
            $<TARGET_FILE:auto_aim_qml>
    )
else()
    message(WARNING "Qt6 Quick not found — QML UI executable will NOT be built.")
endif()
