# 项目文件结构与 CMake 环境设计方案

## 功能背景

当前仓库仍处于初始化阶段，除了 `computer-graphics-notes/` 与协作规范外，尚未形成可持续演进的工程骨架。

本项目目标不是一次性图形学实验，而是一个可长期迭代的 Windows 平台 C++ 软光栅渲染器。因此需要先建立稳定但不过度细碎的目录分层与 CMake 组织方式，让后续的平台层、渲染主线、资源系统和基础场景能力都能在统一结构下扩展。

## 需求摘要

- 当前仅考虑 `Windows` 平台。
- 本轮需要把结构设计方案实际落成最小工程骨架。
- 目录结构只保留最基本骨架，不展开过多模块。
- 目录分类应尽量克制，避免过早拆得过细。
- 当前不为 `JobSystem`、`ImGui` 等工程化模块预留独立目录。
- 平台相关代码统一放在 `platform/` 目录下，不再单独拆 `platform_windows/`。
- 当前不保留 `math/`、`tools/`、`tests/` 目录。
- CMake 采用最轻量的 target 划分，先满足工程启动与后续扩展。
- 当前不落地第三方依赖接入，但保留后续扩展位置。

## 对应阅读的 computer-graphics-notes 模块

- `computer-graphics-notes/README.md`
- `computer-graphics-notes/lesson01-project-overview-and-entry.md`
- `computer-graphics-notes/lesson02-platform-window-and-input.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 笔记对本方案的约束与启发

- `lesson01` 约束入口组织应保持清晰，围绕 `main -> Application -> LayerStack` 搭建整体结构。
- `lesson02` 约束平台相关代码应与上层抽象隔离，但当前阶段不必为平台再拆额外子目录。
- `lesson10` 提醒我们后续会有工程化扩展，但当前阶段不需要过度预埋复杂模块。
- `README` 启发本项目应该按“可学习、可维护、可扩展”的路线组织，而不是单文件实验工程。

## 设计目标

1. 先搭建一个足够清晰的最小工程骨架。
2. 让平台代码与应用层、渲染层分开。
3. 让渲染、资源、场景、应用层各自有明确落位。
4. 让 CMake 第一版足够轻，后续再按需要细拆。

## 推荐目录结构

```text
softRanderer/
├─ CMakeLists.txt
├─ cmake/
├─ apps/
│  └─ viewer/
├─ src/
│  ├─ app/
│  ├─ base/
│  ├─ platform/
│  │  └─ input/
│  ├─ render/
│  ├─ shader/
│  ├─ scene/
│  ├─ resource/
│  │  └─ loaders/
│  └─ layers/
├─ include/
│  └─ soft_renderer/
├─ assets/
│  ├─ models/
│  ├─ textures/
│  ├─ hdr/
│  └─ scenes/
├─ docs/
│  ├─ features/
│  └─ architecture/
├─ third_party/
└─ computer-graphics-notes/
```

## 目录设计说明

### 1. `apps/`

放可执行程序入口。第一阶段建议只保留一个 `viewer`。

好处：

- 入口与核心源码分离。
- 后续如果增加别的程序入口，不需要打乱 `src/`。

### 2. `src/app/`

放应用调度层与 Layer 组织层，是最贴近 `lesson01` 的模块。

### 3. `src/platform/`

统一放平台相关代码，包括：

- 平台抽象接口
- 窗口封装
- 输入定义
- Windows 具体实现

这样目录更简洁，也符合当前希望的更扁平结构。

### 4. `src/render/`

放软光栅主流程相关代码，只保留渲染执行相关内容。

### 5. `src/shader/`

放 C++ 侧的 shader 接口和实现。

### 6. `src/scene/`

放场景描述层数据，例如相机、网格、材质、光源等。

### 7. `src/resource/`

放资源对象与加载器，避免模型、纹理加载逻辑散落到别的模块。

### 8. `src/layers/`

放具体功能层实现，符合当前项目以 Layer 组织功能的主线。

### 9. `assets/`

统一放模型、纹理、HDR、场景文件，尽早规范资源路径。

## 不建议的目录组织方式

### 方案 A：全部平铺在 `src/`

问题：

- 初期看似简单，后续会很快失控。
- 平台代码、渲染代码、资源代码会互相污染。

### 方案 B：过早为未来复杂功能预留大量模块

问题：

- 当前收益不高。
- 会把第一版工程骨架做得过重。

### 方案 C：把应用层、渲染层和平台层完全混放

问题：

- 后续维护困难。
- 平台边界不清楚。

## 推荐 CMake 组织方式

## 总体原则

1. 顶层 `CMakeLists.txt` 只做项目声明、全局选项、子目录组织。
2. 第一版先保持最少 target 数量。
3. 可执行程序不直接挂全部源码，而是链接核心库。
4. 平台代码集中收敛在 `platform/` 目录下。

## 推荐 target 划分

第一阶段建议只保留 3 个 target：

- `sr_engine`
  - 放核心运行时代码
- `sr_platform`
  - 放平台相关代码
- `sr_viewer`
  - 放程序入口

当前实际依赖关系为：

```text
sr_viewer -> sr_engine -> sr_platform
```

这种划分足够支撑第一版工程搭建，也便于后续再细拆。

## 推荐顶层 CMake 结构

```cmake
cmake_minimum_required(VERSION 3.26)

project(SoftRenderer
    VERSION 0.1.0
    DESCRIPTION "Software rasterizer on Windows"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(src)
add_subdirectory(apps/viewer)
```

## `src/CMakeLists.txt` 的职责

建议只声明库 target，不在这里堆业务逻辑。

当前实际做法：

```cmake
add_library(sr_platform)
add_library(sr_engine)
```

再分别给 target 设置：

- `target_sources`
- `target_include_directories`
- `target_link_libraries`
- `target_compile_features`
- `target_compile_definitions`

## 平台隔离建议

`sr_platform` 中集中处理：

- Win32 头文件
- Windows 专属编译定义
- `user32` 等系统库链接

这样上层模块不需要直接感知平台实现细节。

## 编译标准与输出配置

建议默认：

- `CMAKE_CXX_STANDARD 20`
- `CMAKE_CXX_STANDARD_REQUIRED ON`
- `CMAKE_CXX_EXTENSIONS OFF`

输出目录统一到：

- `build-*/bin`
- `build-*/lib`

## 第三方依赖预留方案

当前不实际接入，但保留：

- `third_party/`
- `cmake/`

后续如果需要引入 `stb_image`、`imgui`、`tinyobjloader` 等库，再决定是放仓库内还是用 CMake 拉取。

## 实际落地文件

本轮已落地的最小工程骨架包括：

- `CMakeLists.txt`
- `src/CMakeLists.txt`
- `apps/viewer/CMakeLists.txt`
- `apps/viewer/main.cpp`
- `src/app/Application.h`
- `src/app/Application.cpp`
- `src/platform/Platform.h`
- `src/platform/Platform.cpp`
- `src/platform/Window.h`
- `src/platform/Window.cpp`

同时补齐了骨架目录占位文件：

- `src/base/.gitkeep`
- `src/platform/input/.gitkeep`
- `src/render/.gitkeep`
- `src/shader/.gitkeep`
- `src/scene/.gitkeep`
- `src/resource/.gitkeep`
- `src/resource/loaders/.gitkeep`
- `src/layers/.gitkeep`
- `include/soft_renderer/.gitkeep`
- `assets/models/.gitkeep`
- `assets/textures/.gitkeep`
- `assets/hdr/.gitkeep`
- `assets/scenes/.gitkeep`
- `docs/architecture/.gitkeep`
- `cmake/.gitkeep`
- `third_party/.gitkeep`

## 风险点与兼容性影响

### 风险点

- 如果骨架过轻，后续仍可能需要二次细拆。
- 如果 `platform/` 内部边界不守住，后面还是可能重新混杂。
- 本机默认 Visual Studio 生成器下未检测到 MSVC，需要显式使用可用编译器或正确的开发环境。

### 兼容性影响

- 当前仓库几乎为空，本方案不会引入兼容性破坏。

## 手动测试方案

1. 配置工程：
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程：
   - `cmake --build build-mingw`
3. 运行程序：
   - `build-mingw/bin/sr_viewer.exe`
4. 预期结果：
   - 控制台输出一行启动信息
   - 程序能够成功走通 `Application -> Platform -> Window` 这条最小链路
5. 如果结果异常，优先检查：
   - 本机是否安装可用的 C++ 编译器
   - 是否使用了和当前环境匹配的 CMake generator
   - `sr_platform` 是否正确链接了 Windows 系统库

## 完成标准 / 验收标准

本轮方案与骨架被视为通过，需要满足：

1. 用户确认接受当前最小目录骨架。
2. 用户确认暂不预留 `JobSystem`、`ImGui` 独立模块。
3. 用户确认平台相关代码统一归入 `platform/`。
4. 用户确认不保留 `math/`、`tools/`、`tests/`。
5. 用户确认 CMake 第一版只保留 `sr_engine`、`sr_platform`、`sr_viewer` 三个 target。
6. 用户确认这套骨架可以作为后续功能开发起点。

## 推荐结论

如果目标是“先把工程搭起来，再逐步长功能”，当前方案已经满足第一阶段需求：

- 目录只保留最基本分层。
- 不额外保留 `math/`、`platform_windows/`、`tools/`、`tests/`。
- CMake 只保留最小 target 划分。
- 最小工程已经完成配置、编译与运行验证。

这样更符合当前阶段：先把骨架搭稳，再往上长。
