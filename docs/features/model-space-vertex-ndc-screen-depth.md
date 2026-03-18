# 模型空间顶点、Renderer 内部 NDC/屏幕变换与深度缓冲

## 功能背景

当前项目的最小渲染链路已经能把一个纯色三角形显示到窗口中，但顶点数据仍然直接使用 NDC 坐标，`Renderer` 只负责做屏幕映射和颜色写入。这种实现适合最早期打通显示链路，但和后续软光栅主线还差关键一步：顶点应先表达模型空间位置，再在渲染器内部完成变换、深度计算和深度测试。

本轮目标是把当前渲染路径升级为更接近标准软件光栅主线的最小版本，为后续接入相机、更多几何体和更完整的渲染管线打基础。

在本轮 review 迭代中，MVP 接口进一步收敛为过渡版设计：`Renderer` 不再只暴露 `SetMvp()`，而是分开暴露 `SetModel()`、`SetView()`、`SetProjection()`，内部缓存组合矩阵，并在实现中留出后续迁移到 `Uniforms + Shader` 的明确标记。

## 需求摘要

- 将 `Vertex` 从直接存储 NDC 顶点改为存储模型空间顶点。
- 在 `Renderer` 内部完成顶点从模型空间到裁剪空间、NDC、屏幕空间的变换。
- 增加深度缓冲，光栅化时先做深度测试，再决定是否写颜色。
- 保留当前最小彩色三角形演示，示例数据改为模型空间顶点。
- 本轮只做最小实现，不引入完整 shader/program 系统。
- 作为过渡版，渲染器分开管理 `Model/View/Projection`，便于后续接入 `Camera` 结构与着色器统一输入。

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson04-framebuffer-depth-and-msaa.md`

## 笔记对当前功能的约束与启发

- `lesson05` 约束顶点的最低语义应是模型空间位置，不能继续把顶点格式绑定成 NDC 输入。
- `lesson05` 也启发后续着色器设计应把变换相关数据视为每次 draw 共享输入，而不是长期固化在渲染器流程里。
- `lesson06` 启发当前主线应明确为：模型空间顶点 -> 裁剪空间 -> NDC -> 屏幕空间 -> 重心插值 -> 深度测试 -> 写颜色。
- `lesson04` 约束 `Framebuffer` 不应只管理颜色，还应提供深度缓冲及清理接口。

## 技术方案与关键数据流

### 顶点数据

- 将 `Vertex` 中的 `positionNdc` 改为 `positionModel`。
- 先保持颜色属性不变，继续使用顶点色插值。

### 变换输入

- `Renderer` 分开提供 `SetModel()`、`SetView()`、`SetProjection()`。
- 渲染器内部缓存 `m_Model`、`m_View`、`m_Projection` 与组合后的 `m_Mvp`。
- 在缓存更新函数附近加入 `TODO(shader)` 标记，后续迁移到 `Uniforms + Program` 时从这里收口。
- `DrawTriangle()` 内部把每个顶点先变换到裁剪空间。

### 渲染主线

```text
Application 准备模型空间顶点和 Model / View / Projection
-> Renderer::SetModel()
-> Renderer::SetView()
-> Renderer::SetProjection()
-> Renderer 内部重建 MVP
-> Renderer::Draw(mesh)
-> DrawTriangle()
-> ModelPos 乘 MVP 得到 ClipPos
-> 透视除法得到 NDC
-> Renderer 内部映射到屏幕坐标和深度
-> 包围盒遍历 + 重心插值
-> 深度测试通过后写颜色和深度
```

### 深度缓冲

- `Framebuffer` 增加 `m_DepthBuffer`。
- 新增清深度、读深度、写深度接口。
- 清屏时颜色和深度都可重置。
- 深度值采用屏幕空间 `z`，范围按 NDC `[-1, 1]` 映射到 `[0, 1]`。

## 计划修改的文件

- `docs/features/model-space-vertex-ndc-screen-depth.md`
- `src/render/Vertex.h`
- `src/render/Framebuffer.h`
- `src/render/Framebuffer.cpp`
- `src/render/Renderer.h`
- `src/render/Renderer.cpp`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- 当前实现暂不做裁剪，若后续传入超出视锥的三角形，屏幕映射结果可能不完整或异常。
- 当前深度缓冲只做最小单样本版本，不覆盖 MSAA 和更复杂的深度函数配置。
- 当前 `SetModel / SetView / SetProjection` 仍然是过渡版状态管理；后续真正进入 shader 设计时，应把这些数据迁移为每次 draw 的 `Uniforms` 输入。

## 手动测试方案

1. 配置并编译工程。
2. 运行示例程序。
3. 观察窗口中是否仍能稳定显示两张前后遮挡的三角形。
4. 确认应用层现在不再手工组合 `MVP` 传入，而是分开设置 `Model / View / Projection`。
5. 如需验证过渡接口，分别只改 `model`、`view` 或 `projection` 中一个矩阵，确认画面仍可正常更新。

## 完成标准 / 验收标准

1. 已更新开发文档。
2. `Vertex` 已表达模型空间位置。
3. `Renderer` 已在内部完成模型空间到 NDC/屏幕的变换。
4. `Framebuffer` 已具备深度缓冲。
5. 光栅化阶段已执行深度测试并写回深度。
6. `Renderer` 已提供过渡版 `Model/View/Projection` 接口，并留出后续着色器迁移标记。
7. 已提供清晰的手动测试方法。
8. 等待用户 review。
