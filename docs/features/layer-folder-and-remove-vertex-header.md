# 功能：整理 layer 目录并删除 Vertex 过渡头

## 功能背景

当前 `src/app` 下的 layer 相关文件仍然散落在目录根部，和 `Application` 这样的应用调度文件并列，目录语义不够清晰。同时 `src/render/Vertex.h` 仍然只是一个过渡聚合头，已经不再承载核心职责，应该删除，避免继续形成错误入口。

## 需求摘要

- 将 `src/app` 下所有 layer 相关文件整理到 `src/app/layer/`
- 保持 layer 生命周期、职责边界和运行时行为不变
- 删除 `src/render/Vertex.h`
- 同步修改 include 路径、构建文件和文档
- 不改渲染行为，不改 layer 设计，不改 pipeline 算法

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 computer-graphics-notes 模块

- `computer-graphics-notes/lesson01-project-overview-and-entry.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- `lesson01` 约束这次只做工程化目录整理，不改变 `Application -> LayerStack -> Layer` 主调度链。
- `lesson10` 启发这次应把文件组织和依赖入口收清楚，但不顺手扩大到运行时行为改造。
- 架构图约束 layer 仍属于 `src/app` 模块，因此本次是模块内重组，不是跨模块迁移。

## 技术方案与关键数据流

### 目录调整

将以下文件移动到 `src/app/layer/`：

- `Layer.h`
- `LayerContext.h`
- `LayerStack.h`
- `CameraLayer.h/.cpp`
- `RenderLayer.h/.cpp`
- `SceneLayer.h/.cpp`

`Application.h/.cpp` 保持在 `src/app/`。

### include 调整

- `Application` 改为包含 `app/layer/*`
- layer 之间互相包含时统一走新的子目录路径
- 其它模块引用 `LayerContext` 等类型时也改到新路径

### Vertex 过渡头清理

- 删除 `src/render/Vertex.h`
- 将仍引用它的源码统一改为直接包含：
  - `render/VertexTypes.h`
  - `render/Program.h`
  - `render/shader/ShaderTypes.h`
  - `render/shader/Lighting.h`
  - `render/shader/LitShader.h`
- 保持渲染协议和行为不变，只移除过渡入口

## 计划修改的文件

- `docs/features/layer-folder-and-remove-vertex-header.md`
- `src/app/Application.h`
- `src/app/Application.cpp`
- `src/app/layer/*`（新增目录并迁移原有 layer 文件）
- `src/render/Vertex.h`（删除）
- 所有引用旧路径或旧聚合头的源码文件
- `src/CMakeLists.txt`
- `docs/architecture/project-architecture.md`

## 风险点与兼容性影响

- 文件迁移会引发 include 路径级联修改
- 如果有漏改的包含路径，编译会直接报错
- 删除 `Vertex.h` 后，隐藏的旧依赖会被暴露出来
- 本次会改变项目目录结构，因此架构文档必须同步更新

## 手动测试方案

1. 编译
   - `cmake --build build-mingw --target sr_viewer`
2. 运行
   - `build-mingw/bin/sr_viewer.exe`
3. 检查默认 OBJ 场景是否正常显示
4. 检查相机控制是否仍然正常
5. 如结果异常，优先检查：
   - include 路径是否全部更新
   - `src/CMakeLists.txt` 是否包含新路径
   - 删除 `Vertex.h` 后是否仍有旧引用

## 完成标准 / 验收标准

1. layer 相关文件已进入 `src/app/layer/`
2. `Application` 仍保持在 `src/app/`
3. `src/render/Vertex.h` 已删除
4. 所有 include 已切到新路径或新 render 头
5. 编译通过
6. 已检查并更新架构文档
7. 已提供手动测试方式
8. 等待用户 review
