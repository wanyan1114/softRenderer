# 阶段四：PipelineStages 状态机化与通用状态机基础设施

## 功能背景

阶段四的目标是把当前 `Pipeline + PipelineStages` 从松散的阶段串联，整理成更明确的状态机式执行模型，同时保持 `Pipeline` 为无状态执行器。

本轮采用的是一版轻量通用状态机基础设施：

- `src/base/StateMachine.h` 提供通用状态基类与管理类
- `src/render/PipelineStages.h` 在这套基础设施上组织 `Vertex / Clip / Raster / Fragment`
- 阶段之间通过固定交接数据结构传递参数

## 需求摘要

- `src/base` 中实现通用状态机基类与管理类
- `PipelineStages.h` 继续作为 render 阶段状态与交接数据的承载文件
- `VertexStage / ClipStage / RasterStage / FragmentStage` 改为状态机中的具体阶段
- `Pipeline` 继续保持无状态执行器
- 阶段交接数据仍定义在 `PipelineStages.h` 中，不新增 `PipelineStageData.h`
- 保持当前 OBJ Lit 渲染结果不变

本阶段不做：

- 新 shader 类型
- loader 重构
- 并行化 / JobSystem
- command buffer / render command 系统
- MSAA 扩展

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束 `src/base` 可以承载可复用基础能力，因此通用状态机应落在 `base`。
- `lesson05` 约束 `Program / uniforms / varyings` 仍是渲染基础协议，本轮不应重写 shader 协议。
- `lesson06` 约束 `Vertex -> Clip -> Raster -> Fragment` 仍是当前渲染主线，状态机只是显式化执行模型。
- `lesson10` 启发执行模型应更工程化、更易管理，且基础设施要可复用。

## 当前代码与目标结构差异

当前问题：

- `Pipeline` 虽然已经无状态，但阶段切换仍然不够显式
- `PipelineStages` 之前更像工具集合，而不是明确的状态节点
- 阶段之间的参数流动没有通过统一状态机抽象来表达

目标结构：

- `src/base/StateMachine.h`
  - `State<owner_t, state_id_t>`
  - `StateMachine<owner_t, state_id_t>`
- `src/render/PipelineStages.h`
  - `PipelineStageId`
  - 阶段交接数据结构
  - `VertexStage / ClipStage / RasterStage / FragmentStage`
  - 基于 `StateMachine` 的 render 专用阶段机
- `src/render/Pipeline.h`
  - 无状态执行器，负责创建并驱动 render 状态机

## 技术方案与关键数据流

### base 通用状态机

在 `src/base/StateMachine.h` 中提供：

- `State<owner_t, state_id_t>`
  - `GetID()`
  - `OnEnter()`
  - `OnExit()`
  - `OnExcute()`
- `StateMachine<owner_t, state_id_t>`
  - `SetInitialState()`
  - `ChangeState()`
  - `ClearState()`
  - `UpdateCurrentState()`

### render 阶段状态机

在 `src/render/PipelineStages.h` 中：

- 定义 `PipelineStageId`
- 保留固定交接数据结构
- 让四个 stage 继承 `base::State<machine_t, PipelineStageId>`
- `PipelineStageMachine` 负责状态切换与运行依赖管理

### 执行流

```text
Pipeline::Run(..., program, uniforms)
-> 创建 render 专用 stage machine
-> SetInitialState(VertexStage)
-> VertexStage 写 VertexStageData 并切到 ClipStage
-> ClipStage 写 ClipStageData 并切到 RasterStage
-> RasterStage 生成当前片元批次并切到 FragmentStage
-> FragmentStage 完成片元写入后切回 RasterStage 或进入完成态
```

说明：

- `program / uniforms / framebuffer` 属于本次运行依赖，由 render stage machine 持有
- 阶段之间的固定交接数据仍定义在 `PipelineStages.h`
- 不引入大而全 `PipelineContext`

## 计划修改的文件

- `docs/features/stage4-pipeline-state-machine-refactor.md`
- `src/base/StateMachine.h`
- `src/render/Pipeline.h`
- `src/render/PipelineStages.h`
- `docs/architecture/project-architecture.md`

## 风险点与兼容性影响

- 状态切换逻辑如果组织不清楚，容易把 `PipelineStages.h` 再次写回工具箱风格
- 深度测试、裁剪后多三角形遍历和 fragment 阶段切换必须保持现有行为
- 本轮会改变 `src/base` 和 `src/render` 的职责边界，需要检查并同步更新架构文档

## 手动测试方案

1. 编译 / 运行
   - `cmake --build build-mingw --target sr_viewer`
   - `build-mingw/bin/sr_viewer.exe`
2. 启动验证
   - 默认 OBJ 场景正常显示
   - 控制台仍只输出 `FPS`
3. 渲染验证
   - 纹理采样正常
   - 法线光照与高光正常
   - 无缺面、闪烁或深度错误
4. 结构验证
   - `src/base/StateMachine.h` 已提供轻量通用状态机
   - `PipelineStages.h` 已基于该状态机管理固定阶段切换
   - `Pipeline` 仍然无状态
5. 如结果异常，优先检查
   - `src/base/StateMachine.h`
   - `src/render/PipelineStages.h`
   - `src/render/Pipeline.h`

## 完成标准 / 验收标准

1. 已完成需求对齐
2. 已生成开发文档
3. `src/base` 已提供通用状态机基类与管理类
4. `PipelineStages` 已基于通用状态机管理固定阶段切换
5. `Pipeline` 仍保持无状态执行器
6. 当前 OBJ Lit 渲染结果保持正常
7. 已检查并在需要时更新架构文档
8. 已提供手动测试方式
9. 等待用户 review
