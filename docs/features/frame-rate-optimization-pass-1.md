# 帧率优化第一轮

## 功能背景

当前 viewer 在 `1280x720` 下运行时帧率约为 `3 FPS`，已经明显低于“最小可交互软光栅 demo”应有的水平。

结合当前仓库代码路径，本次瓶颈并不主要表现为单一 shader 公式过重，而更像是渲染主线内部存在额外的大块内存扫描和临时缓存开销，同时构建参数也没有启用优化编译：

- `src/render/Pipeline.h` 中每次 `Pipeline::Run()` 都会整屏复制深度数据，并在结束时整屏回写颜色/深度。
- `src/render/PipelineStages.h` 中当前光栅化阶段会先把所有 fragment 收集到 `std::vector`，再做第二次遍历执行片元着色和深度测试。
- `src/app/Application.cpp` 中每帧会对 6 个独立 mesh 分别调用 `pipeline.Run()`，放大了上述固定成本。
- 当前 `build-mingw` 的 `flags.make` 只有 `-std=c++20`，没有 `Release` 优化标志。

本轮目标是先做一轮“高收益、低侵入”的性能优化，把这些与渲染结果无关的额外内存搬运降下来，并补上合理的默认编译优化等级。

## 需求摘要

- 优先提升当前 viewer 的实时帧率，目标是在不改变现有画面结果的前提下明显高于当前约 `3 FPS` 的水平。
- 保持现有模块边界不变，不新增新的项目级模块。
- 保持当前调试立方体面编号、基础光照、相机交互和窗口显示行为不变。
- 在当前仓库基础上补最小性能观测输出，便于后续继续定位热点。
- 为单配置生成器补一个默认优化构建类型，但不覆盖用户已经显式指定的 build type。

本次不做：

- 不重写平台显示路径
- 不引入线程池、JobSystem 或新的并行框架
- 不改资源格式、模型加载协议或相机控制协议
- 不做大规模架构重构
- 不承诺一次性把渲染器优化到最终形态

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束本次优化应主要落在 `src/render` 的流水线内部实现，以及 `src/app` 的逐帧观测逻辑中，不能把渲染性能问题扩散到 `src/platform` 之外的模块边界。
- `lesson06` 约束优化不能破坏既有的主流程顺序：
  `VertexShader -> Clip -> NDC/FragPos -> Rasterize -> FragmentShader -> Framebuffer`。
- `lesson06` 同时启发本次最值得优化的热点是包围盒遍历、重心插值、深度测试和颜色写回这一段，而不是先动应用层装配逻辑。
- `lesson10` 启发性能优化应先有最小观测能力，再做结构调整。

## 当前代码与笔记差异

- `lesson10` 中提到的 `JobSystem / Instrumentor / ImGui` 工程化能力，在当前仓库里并未完整落地。
- 当前仓库只有终端 FPS 输出，没有分阶段 profile。
- 因此本次会采用适配当前代码形态的最小方案：在现有 `Application` 主循环中输出更可定位的问题信息，同时优先优化 `Pipeline` 内部的额外内存扫描。

## 技术方案与关键数据流

当前高成本路径：

```text
Application::Run()
-> RenderFaceDebugScene()
-> 6 次 Pipeline::Run()
-> InitializeFrame() 整屏复制 depth
-> RasterizeTriangle() 收集所有 fragment 到 vector
-> FragmentStage 再次遍历 fragment
-> CommitFrame() 整屏回写颜色/深度
-> Platform::Present()
```

本次目标路径：

```text
Application::Run()
-> RenderFaceDebugScene()
-> 6 次 Pipeline::Run()
-> Vertex/Clip/Rasterize
-> 光栅化阶段逐像素直接执行深度测试与着色
-> 直接写回 Framebuffer
-> Platform::Present()
```

方案拆分：

1. 删除 `Pipeline` 内部按整屏构造的 `StagedFrame`
   - 不再在每次 `Pipeline::Run()` 开头复制整张深度缓冲
   - 不再在每次 `Pipeline::Run()` 结尾整屏扫描提交结果
2. 删除 `RasterStage` 中的 fragment 临时 `std::vector`
   - 由光栅化阶段逐像素计算后立刻交给片元阶段
   - 避免大批量 `Fragment` 对象分配与二次遍历
3. 保持 `Framebuffer` 仍然是唯一颜色/深度结果容器
   - 仅调整写入时机，不改变其职责和数据语义
4. 在 `Application` 中补最小分段耗时输出
   - 至少区分 `render` 和 `present` 两段耗时
   - 便于后续判断瓶颈是否仍在平台显示路径
5. 为单配置生成器设置默认 `Release` 构建类型
   - 仅在用户未显式指定 `CMAKE_BUILD_TYPE` 时生效
   - 不改变多配置生成器的行为

## 计划修改的文件

- `CMakeLists.txt`
- `docs/features/frame-rate-optimization-pass-1.md`
- `src/render/Pipeline.h`
- `src/render/PipelineStages.h`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- `Pipeline` 和 `PipelineStages` 都是头文件模板实现，改动后容易引入编译错误，需要完整编译验证。
- 从 staged frame 改为直接写回 `Framebuffer` 后，必须确认深度测试与颜色写回顺序仍与现有结果一致。
- 删除 fragment 临时缓存后，如果某处逻辑隐含依赖“先收集后统一提交”，需要及时修正。
- 默认切到 `Release` 只影响未指定 build type 的单配置生成器，但仍需要在交付时明确告知重新配置方式。
- 本次不改变模块职责、目录依赖方向和共享核心数据结构语义，原则上不应需要更新项目架构文档。

## 手动测试方案

1. 重新配置为优化构建
   - `cmake -S . -B build-mingw -DCMAKE_BUILD_TYPE=Release`
2. 编译
   - `cmake --build build-mingw`
3. 运行
   - `build-mingw/bin/sr_viewer.exe`
4. 观察性能输出
   - 记录终端中的 `FPS`
   - 同时观察新增的 `render` / `present` 耗时信息
5. 观察画面正确性
   - 立方体 6 个面应继续显示编号
   - 基础光照结果应与修改前保持一致
   - 相机移动与转向交互应正常
6. 异常观察点
   - 若画面闪烁或遮挡错误，优先检查直接写回 framebuffer 后的深度测试顺序
   - 若帧率无明显提升，优先检查平台显示路径是否仍然占主导
   - 若部分面缺失或编号异常，优先检查光栅化阶段改写时是否漏掉片元着色调用

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成开发文档。
3. 已去掉 `Pipeline` 中每次 draw 的整屏 staged frame 复制/提交。
4. 已去掉 `RasterStage` 中 fragment 临时 `vector` 的主要额外开销。
5. viewer 运行时能够看到比原来更可用的性能输出。
6. 单配置生成器默认构建类型已具备优化等级。
7. 当前示例画面结果不回退。
8. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
9. 已提供清晰的手动测试方法。
10. 等待用户 review。
