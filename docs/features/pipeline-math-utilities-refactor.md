# Pipeline 数学工具抽取

## 功能背景

当前 `src/render/Pipeline.h` 在承担渲染主线之外，还内聚了大量纯数学/几何辅助函数，例如边函数、面积计算、采样点计算、裁剪距离、NDC 计算、全量 varyings 线性插值等。

其中一部分函数本身不直接表达渲染阶段职责，更适合作为可复用的底层工具能力存在。把它们继续留在 `Pipeline` 类内部，会让后续按阶段拆分时，主线代码仍被大量低层数学细节打断。

因此本轮作为重构第一步，先将类内最基础的几何工具函数抽到独立工具类中，并同步把 `Run()` 整理为更清晰的阶段编排入口，同时保持当前行为完全不变。

## 需求摘要

- 保持 `Pipeline<vertex_t, uniforms_t, varyings_t>` 对外接口不变。
- 保持 `Pipeline::Run(Framebuffer&, const Mesh<vertex_t>&)` 行为不变。
- 只抽取类内纯数学/几何辅助函数，不进行完整阶段拆分。
- 新增一个独立工具类，但只承接最基础、最通用的几何 helper。
- 裁剪阶段专属概念与函数仍保留在 `Pipeline` 中。
- `Mesh` 负责索引装配，不让 `Run()` 直接处理索引数组。
- `Run()` 只保留少量阶段函数调用，遍历函数内部逻辑继续下沉。
- 不修改 viewer 场景、不新增功能、不调整模块依赖方向。

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束本轮改动仍然应局限在 `src/render` 内部，`Pipeline` 仍是渲染主线执行器。
- `lesson05` 约束 `varyings_t` 的标准布局与 float 兼容插值假设不能改变。
- `lesson06` 启发先抽出数学工具层，但裁剪阶段语义应继续跟随渲染主线，而不是被误归类为通用数学工具。
- `lesson06` 同时约束 `Run()` 应尽量直接对应渲染主线阶段，而不是混杂索引装配和多层细节循环。

## 技术方案与关键数据流

计划保留独立头文件：

- `src/render/PipelineMathHelper.h`

其中提供工具类：

```text
PipelineMathHelper
```

该工具类只负责承接 `Pipeline` 当前内部最基础的几何辅助能力：

- 边函数
- 三角形面积计算
- 退化三角形判断
- 像素中心采样点计算
- 采样覆盖判断

`Mesh` 新增三角形遍历接口，负责从 `vertices + indices` 装配输入三角形，再把合法三角形交给回调处理。

`Pipeline` 自身继续保留：

- 状态持有
- 主线调度
- 裁剪阶段枚举与裁剪距离判断
- `ClipPos` / `NdcPos` 校验与空间变换
- 整套 `varyings_t` 的线性插值
- 裁剪交点插值
- 顶点整理
- 裁剪流程组织
- 光栅化流程组织
- 着色与回写流程组织

`Run()` 目标收敛为：

```text
CanRun
-> InitializeFrame
-> ForEachTriangle
-> CommitFrame
```

其中：

```text
ForEachTriangle
-> mesh.ForEachTriangle(...)
-> ProcessInputTriangle(...)
-> ProcessClippedPolygon(...)
-> ProcessScreenTriangle(...)
```

## 计划修改的文件

- `docs/features/pipeline-math-utilities-refactor.md`
- `src/render/Mesh.h`
- `src/render/Pipeline.h`
- `src/render/PipelineMathHelper.h`

## 风险点与兼容性影响

- `Mesh` 新增遍历接口后，要确保非法索引仍沿用“跳过该三角形”的原行为。
- `PipelineMathHelper` 收窄后，职责边界更清晰，但要确保迁回 `Pipeline` 的裁剪函数仍保持原公式和 epsilon。
- `Run()` 虽然变短，但阶段函数拆分不能改变裁剪、扇形拆分、深度测试和着色顺序。
- 工具类拆出后要避免引入循环包含。
- 本轮不改变架构边界，通常不需要更新项目架构图。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行程序
   - `build-mingw/bin/sr_viewer.exe`
4. 观察结果
   - 当前裁剪、背面剔除、深度遮挡与双面绘制表现应保持不变
5. 如结果异常，重点检查
   - `Mesh` 的三角形遍历是否漏掉合法三角形
   - `varyings` 插值和 `clipPos.w` 处理是否一致
   - 扇形拆分后是否仍逐个进入光栅化和着色

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成开发文档。
3. `Pipeline` 类内最基础几何工具函数已迁移到独立工具类。
4. 裁剪阶段专属代码已保留或迁回 `Pipeline`。
5. `Mesh` 已负责索引装配与三角形遍历。
6. `Run()` 已收敛为少量阶段函数调用。
7. `Pipeline` 对外接口与行为保持不变。
8. 已完成编译验证或明确说明未验证项。
9. 已提供清晰的手动测试方法。
10. 等待用户 review。
