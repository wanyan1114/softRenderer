# 帧率优化第二轮

## 功能背景

第一轮优化后，viewer 在用户实测中已经从约 `3 FPS` 提升到约 `65.3 FPS`，对应终端输出大致为：

- `FPS: 65.3`
- `Frame: 15.3 ms`
- `Render: 14.8 ms`
- `Present: 0.5 ms`

这说明当前主要瓶颈已经不在 `Platform::Present()` 或 `StretchDIBits`，而是继续集中在 `src/render` 的像素级热路径中。

结合现有实现，当前每个命中像素仍会频繁经过：

- `Framebuffer::GetDepth(x, y)`
- `Framebuffer::SetDepth(x, y, depth)`
- `Framebuffer::SetPixel(x, y, color)`

这些调用虽然语义清晰，但会在高频像素路径中重复执行：

- 边界检查
- `y * width + x` 索引计算
- 成员函数调用

本轮目标是进一步压缩这些每像素固定成本，在不改变渲染结果和模块边界的前提下继续降低 `Render` 耗时。

## 需求摘要

- 在保持当前画面结果不变的前提下，继续降低 `Render` 平均耗时。
- 优先优化 `Framebuffer` 与片元写回热路径。
- 保持当前终端性能输出格式继续可用，便于对比优化前后效果。
- 不改变 viewer 的交互、模型加载、窗口显示和渲染语义。

本次不做：

- 不引入并行化或 JobSystem
- 不改 `Platform::Present()` 路径
- 不改 shader 协议、资源格式或相机系统
- 不做 SIMD、tile-based rasterizer 或大规模 draw call 重组
- 不改变颜色/深度缓冲的数据语义

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson04-framebuffer-depth-and-msaa.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束 `Framebuffer` 仍然是渲染结果的唯一主容器，本次只优化其访问方式，不改变其职责。
- `lesson04` 启发这次优化的核心是颜色/深度缓冲读写路径，而不是应用层调度。
- `lesson06` 约束深度测试和颜色写回顺序不能回退，必须保持现有从三角形到像素的主流程语义。
- `lesson10` 启发我们根据第一轮新增的性能观测结果继续聚焦真正热点，即 `Render` 而不是 `Present`。

## 当前代码与笔记差异

- 当前仓库仍没有完整的 profiling 基建，主要依赖终端阶段耗时输出。
- 现有 `Framebuffer` 公开接口更偏安全与通用，而不是为像素级热路径专门设计。
- 因此本轮会在保留现有安全接口的同时，补一组明确用于渲染主线内部的快速访问路径。

## 技术方案与关键数据流

当前热路径：

```text
RasterizeTriangle()
-> 命中像素
-> FragmentStage::Execute(x, y, varyings, ...)
-> framebuffer.GetDepth(x, y)
-> framebuffer.SetDepth(x, y, depth)
-> framebuffer.SetPixel(x, y, color)
```

目标热路径：

```text
RasterizeTriangle()
-> 命中像素
-> 直接计算 pixelIndex
-> FragmentStage::Execute(pixelIndex, varyings, ...)
-> 直接访问 depth/pixel buffer
-> 仅保留必要的深度测试与颜色写回
```

方案拆分：

1. 在 `Framebuffer` 中补充面向渲染内部的快速访问接口
   - 暴露颜色/深度缓冲可变指针或引用
   - 暴露无边界检查的索引计算辅助
2. 在 `PipelineStages` 中把片元阶段改为按 `pixelIndex` 工作
   - 避免重复计算 `y * width + x`
   - 避免重复走 `GetDepth/SetDepth/SetPixel` 的边界检查
3. 保留现有公共安全接口
   - 窗口显示、截图导出等非热路径逻辑继续可用
4. 保持终端输出不变
   - 便于和第一轮结果直接对比

## 计划修改的文件

- `docs/features/frame-rate-optimization-pass-2.md`
- `src/render/Framebuffer.h`
- `src/render/Framebuffer.cpp`
- `src/render/PipelineStages.h`

## 风险点与兼容性影响

- 无边界检查快速接口如果被错误调用，可能直接写坏缓冲区，因此必须只在已由光栅包围盒保证合法范围的路径中使用。
- 如果片元阶段改成按 `pixelIndex` 工作，需要确认索引和当前 framebuffer 布局完全一致。
- 本轮不改变公共渲染语义，但会让 `src/render` 内部实现更偏性能优化风格，可读性会略受影响，需要保持命名清晰。
- 本次仍不改变模块职责、依赖方向或关键共享数据结构语义，原则上不应需要更新项目架构文档。

## 手动测试方案

1. 配置并编译
   - `cmake -S . -B build-mingw -DCMAKE_BUILD_TYPE=Release`
   - `cmake --build build-mingw`
2. 运行 viewer
   - `build-mingw/bin/sr_viewer.exe`
3. 记录性能数据
   - 观察 `FPS / Frame / Render / Present`
   - 与第一轮的 `65.3 FPS / 14.8 ms Render` 做对比
4. 观察画面结果
   - 6 个面编号应继续正确显示
   - 光照和遮挡关系应保持一致
   - 相机交互应正常
5. 异常观察点
   - 若出现随机像素错误或面闪烁，优先检查快速写回路径是否误用了越界索引
   - 若 Render 没明显下降，说明下一轮应转向 draw 组织或更高层次优化

## 完成标准 / 验收标准

1. 已完成第二轮需求对齐。
2. 已生成第二轮开发文档。
3. 已为 `Framebuffer` 增加渲染热路径所需的快速访问接口。
4. 已让片元热路径避免重复边界检查和重复索引计算。
5. 编译通过，离屏导出或运行验证通过。
6. `Render` 耗时相对第一轮继续下降或至少具备明确的性能验证结论。
7. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
8. 已提供清晰的手动测试方法。
9. 等待用户 review。

