# 帧率优化第三轮

## 功能背景

前两轮优化已经把 viewer 从极低帧率提升到可交互水平，但当前仓库又出现了两个新的现实问题：

- 运行时只剩下 `FPS` 输出，缺少 `Render / Present` 分段耗时，后续继续定位瓶颈会比较盲。
- `src/render/PipelineStages.h` 里仍保留“光栅化阶段先收集整批 fragment，再由片元阶段二次遍历处理”的路径，这会在像素密集区域产生额外的 `std::vector` 扩容、对象写入和二次遍历成本。

结合当前代码，渲染主链仍是：

```text
RenderLayer
-> Pipeline::Run()
-> VertexStage
-> ClipStage
-> RasterStage
-> FragmentStage
-> Framebuffer
-> Window::Present()
```

本轮目标是在不改变对外模块边界和渲染结果语义的前提下，补回最小可用的分段观测能力，并去掉 `RasterStage -> FragmentStage` 之间的大批量中间缓存开销。

## 需求摘要

- 恢复终端中的最小分段耗时输出，至少包含 `FPS / Frame / Render / Present`。
- 在保持画面结果不变的前提下，去掉 `PipelineStages` 中按三角形批量缓存 fragment 的主要额外开销。
- 保持 `app -> render -> platform` 的调用方向不变。
- 保持 `Framebuffer` 作为唯一渲染结果容器的职责不变。
- 不改变窗口显示、模型加载、相机交互和材质协议。

本次不做：

- 不引入并行化、JobSystem 或 SIMD
- 不改 `Window::Present()` 的显示实现
- 不改 OBJ/MTL/纹理加载协议
- 不做新的项目级模块拆分
- 不把固定阶段流水线改成完全不同的架构

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson04-framebuffer-depth-and-msaa.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束本轮优化主要应落在 `src/app` 的运行时观测逻辑与 `src/render` 的主链内部实现，不能把性能问题扩散到平台层职责之外。
- `lesson06` 约束我们不能破坏 `Vertex -> Clip -> Raster -> Fragment -> Framebuffer` 的主流程语义，深度测试与颜色写回顺序也不能回退。
- `lesson04` 启发这轮热点仍在颜色/深度写回相关路径，`Framebuffer` 只能继续作为结果容器，而不是引入新的 staging 容器。
- `lesson10` 启发性能优化应先有最小观测能力，再继续迭代热点，因此先补回分段耗时是必要前置条件。

## 当前代码与文档差异

- `docs/features/frame-rate-optimization-pass-1.md` 和 `docs/features/frame-rate-optimization-pass-2.md` 中都记录过更细的性能判断思路，但当前 `src/app/Application.cpp` 实际只输出 `FPS`。
- `docs/features/frame-rate-optimization-pass-1.md` 里已经把“去掉 fragment 临时 `vector`”列为优化方向，但当前 `src/render/PipelineStages.h` 仍保留 `RasterStageData::fragments`。
- 当前代码没有违背架构图的模块边界，但仍存在尚未完成的内部热路径优化。

## 技术方案与关键数据流

当前路径：

```text
Application::RunMainLoop()
-> LayerStack::OnUpdate()
-> LayerStack::OnRender()
-> RasterStage::BuildFragments()
-> std::vector<FragmentData>
-> FragmentStage::OnExcute()
-> Framebuffer
-> Window::Present()
```

目标路径：

```text
Application::RunMainLoop()
-> 记录 update/render/present 时间片段
-> LayerStack::OnUpdate()
-> LayerStack::OnRender()
-> RasterStage 在命中像素后直接执行片元处理
-> 直接写回 Framebuffer
-> Window::Present()
```

方案拆分：

1. 在 `Application` 主循环中增加分段计时
   - 记录单帧总耗时
   - 记录 `OnUpdate + OnRender` 作为 `Render`
   - 记录 `Window::Present()` 作为 `Present`
   - 按现有半秒窗口刷新终端状态行
2. 重构 `PipelineStages`
   - 保留现有固定阶段语义和状态机框架
   - 去掉 `RasterStageData` 中按三角形批量缓存的 `fragments`
   - 让 `RasterStage` 在像素命中后直接完成深度测试与片元着色写回
3. 保持 `Framebuffer` 和 `Program` 对外协议不变
   - 只优化内部执行路径
   - 不影响 `RenderLayer` 的调用方式

## 计划修改的文件

- `docs/features/frame-rate-optimization-pass-3.md`
- `src/app/Application.cpp`
- `src/render/PipelineStages.h`

## 风险点与兼容性影响

- `PipelineStages.h` 是模板头文件，修改热路径后如果漏掉某个状态转换或辅助结构，容易直接出现编译错误。
- 从“先收集再统一处理”改成“命中后直接处理”后，必须确认深度测试、颜色写回和背面剔除结果与当前一致。
- 如果 `Render` 时间中继续混入 `OnUpdate`，它更准确地说是“逐帧主渲染段”而不是纯光栅化耗时，需要在交付时说明口径。
- 本次不改变模块职责、依赖方向、共享核心数据结构语义和项目级目录结构，原则上不应需要更新架构文档。

## 手动测试方案

1. 以优化构建重新配置并编译
   - `cmake -S . -B build-mingw -DCMAKE_BUILD_TYPE=Release`
   - `cmake --build build-mingw`
2. 运行 viewer
   - `build-mingw/bin/sr_viewer.exe`
3. 观察终端输出
   - 确认能看到 `FPS / Frame / Render / Present`
   - 记录静止和移动相机时的数值变化
4. 观察画面结果
   - 立方体各面编号应保持正确
   - 光照和遮挡关系应与修改前一致
   - 相机移动和转向应继续正常
5. 异常观察点
   - 若出现闪烁或遮挡错误，优先检查深度测试和片元写回顺序
   - 若程序编译通过但帧率无明显变化，优先比对 `Render` 与 `Present` 的耗时占比
   - 若输出数值异常抖动或为零，优先检查统计窗口累加和时间片段口径

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成本轮开发文档。
3. 终端已恢复 `FPS / Frame / Render / Present` 输出。
4. `PipelineStages` 已去掉按三角形批量缓存 fragment 的主要额外开销。
5. 代码编译通过，viewer 可运行或至少具备明确的本地验证结论。
6. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
7. 已提供清晰的手动测试方法。
8. 等待用户 review。
