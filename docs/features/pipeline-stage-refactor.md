# Pipeline 阶段拆分重构

## 功能背景

当前 `src/render/Pipeline.h` 同时承担了两类职责：

- `Pipeline` 作为渲染主线入口，负责组织一次 draw 的执行顺序
- `Pipeline` 内部又直接承载顶点处理、裁剪、光栅化、片元着色与回写的大量细节

这导致 `Pipeline` 更像“全流程执行器”，而不是“阶段调度器”。对当前项目体量来说，最稳妥的第一步不是立即引入一批正式公共类，而是先在 `render::detail` 下把流程拆成阶段模块，让 `Pipeline` 先回到编排职责。

本轮 review 的进一步要求是：`Pipeline.h` 中只保留 `Pipeline` 类，其他阶段模块与辅助类型统一收敛到一个独立文件中，避免 `Pipeline.h` 再次膨胀。

## 需求摘要

- 保持 `Pipeline<vertex_t, uniforms_t, varyings_t>` 对外接口不变
- 保持 `Pipeline::Run(Framebuffer&, const Mesh<vertex_t>&)` 调用方式不变
- 不引入额外公共渲染模块类型，继续使用 `detail` 命名空间
- 将当前 `Pipeline` 内部逻辑分为：
  - `detail::VertexStage`
  - `detail::ClipStage`
  - `detail::RasterStage`
  - `detail::FragmentStage`
- `Pipeline.h` 中只保留 `Pipeline` 类
- 其他 `detail` 阶段与辅助类型统一放到一个独立文件中
- 保持当前裁剪、深度测试、背面剔除与着色结果不变

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束本轮重构继续限定在 `src/render` 内部，不改变 `app -> render -> framebuffer` 的调用方向
- `lesson05` 约束 `Program / Uniforms / Varyings` 仍是各阶段共享的渲染协议，不能在重构时被打散
- `lesson06` 启发当前主线天然可按“顶点 -> 裁剪 -> 光栅化 -> 片元”拆分，这正好对应本轮的 4 个 stage
- `lesson10` 启发 `Pipeline` 更适合作为阶段编排入口，而阶段细节应下沉到统一的 `detail` 文件中

## 技术方案与关键数据流

本轮将阶段模块与辅助类型统一收敛到：

- `src/render/PipelineStages.h`

其中包含：

```text
detail::ScreenPoint
detail::ScreenVertex
detail::RasterBounds
detail::Fragment
detail::StagedFrame
detail::VertexStage
detail::ClipStage
detail::RasterStage
detail::FragmentStage
```

`Pipeline.h` 只保留：

```text
Pipeline
-> CanRun
-> InitializeFrame
-> ProcessMesh
-> ProcessInputTriangle
-> CommitFrame
```

运行主线保持为：

```text
Run
-> CanRun
-> InitializeFrame
-> ProcessMesh
-> ProcessInputTriangle
   -> VertexStage
   -> ClipStage
   -> RasterStage
   -> FragmentStage
-> CommitFrame
```

实现策略：

- `PipelineStages.h` 作为唯一的阶段细节收纳点
- `Pipeline.h` 只负责调度与成员持有
- 公共数学工具继续复用 `PipelineMathHelper`
- 不改变现有算法顺序与结果

## 计划修改的文件

- `docs/features/pipeline-stage-refactor.md`
- `src/render/Pipeline.h`
- `src/render/PipelineStages.h`

## 风险点与兼容性影响

- `varyings_t` 仍依赖标准布局与 float 兼容插值，阶段搬迁时不能破坏该假设
- 裁剪后扇形拆分、透视正确插值和深度比较顺序必须保持不变
- `PipelineStages.h` 中的共享类型需要保持职责清晰，避免循环依赖
- 本轮属于 render 内部重构，不改变项目级模块依赖，通常不需要更新架构图

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行程序
   - `build-mingw/bin/sr_viewer.exe`
4. 观察结果
   - 裁剪三角形仍能正确显示
   - 前后遮挡关系与深度测试保持不变
   - 背面剔除与双面绘制表现保持不变
5. 如结果异常，重点检查
   - `VertexStage` 输出是否仍正确写入 `clipPos`
   - `ClipStage` 是否保持原裁剪公式和交点插值
   - `RasterStage` 是否保持原屏幕映射与重心插值逻辑
   - `FragmentStage` 是否保持原深度测试与颜色写入顺序

## 完成标准 / 验收标准

1. 已完成需求对齐
2. 已生成开发文档
3. `Pipeline` 内部已拆出 `detail::VertexStage / ClipStage / RasterStage / FragmentStage`
4. `Pipeline.h` 中只保留 `Pipeline` 类
5. 其他阶段与辅助类型已统一收纳到单一独立文件
6. 对外接口与运行结果保持一致
7. 已完成编译验证或明确说明未验证项
8. 已提供清晰的手动测试方法
9. 等待用户 review
