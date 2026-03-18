# 最小 Programmable Pipeline

## 功能背景

当前仓库已经具备最小软光栅主链路，能够把固定格式的 `Vertex` 送入可编程管线并绘制三角形。但现有实现中曾保留一层 `Renderer` 作为 `Framebuffer` 的薄封装，这层中转已经没有足够的职责价值，反而让主渲染链路多出一层无意义跳转。

本轮目标是在不推翻现有最小可运行链路的前提下，保留 `Vertex / Uniforms / Varyings / Program / VertexShader / FragmentShader / Pipeline` 这套最小 programmable pipeline，同时去掉 `Renderer`，让 `Pipeline` 直接面向 `Framebuffer` 完成渲染。

## 需求摘要

- 保留最小基础协议：`VertexBase`、`UniformsBase`、`VaryingsBase`
- 保留 `Program<vertex_t, uniforms_t, varyings_t>`，用于绑定顶点着色器和片元着色器
- 保留 `Pipeline<vertex_t, uniforms_t, varyings_t>`，负责可编程渲染流程编排
- 移除 `Renderer`
- 由 `Pipeline` 直接操作 `Framebuffer`
- `Pipeline` 对外主入口命名为 `Run`
- `Run()` 中需要直接体现“光栅化”和“片元着色”两个阶段
- `RasterizeTriangle()` 只负责生成片元，不直接调用 `Program.fragmentShader`
- `Run()` 中显式调用 `m_Program.fragmentShader(...)` 作为片元着色器
- `Run()` 末尾统一把本轮结果写入 `Framebuffer`
- `Run()` 内部的“片元处理循环”和“结果写回循环”继续提取成独立函数，保持主线简洁
- 提供一个最小示例 shader，先跑通纯色三角形
- 本轮不实现更完整的裁剪系统、MSAA、贴图、光照模型和可配置渲染状态
- 现有 viewer 入口继续可运行，并切换为通过构建 `Pipeline` 来渲染

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson01-project-overview-and-entry.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

## 笔记对当前功能的约束与启发

- `lesson01` 约束改动要贴合现有工程入口和应用主循环，优先复用 viewer 示例验证
- `lesson05` 约束可编程接口不应被额外的固定渲染封装层重新耦合，顶点、uniform、varying 和 shader 应直接服务管线主线
- `lesson06` 约束接口需要能接入现有主线：顶点阶段输出 clip 坐标，经过 NDC 和屏幕映射后参与三角形遍历、插值和片元着色

## 技术方案与关键数据流

新增的最小数据分层如下：

- `VertexBase`
  - 表示每个顶点自带的数据，最小字段为模型空间位置
- `UniformsBase`
  - 表示一次 draw call 共享的数据，最小字段为 `MVP`
- `VaryingsBase`
  - 表示顶点阶段输出并会在三角形内部插值的数据，最小字段为 `ClipPos`、`NdcPos`、`FragPos`

新增最小 `Program`：

```text
Program<vertex_t, uniforms_t, varyings_t>
  = VertexShader(varyings_t&, const vertex_t&, const uniforms_t&)
  + FragmentShader(const varyings_t&, const uniforms_t&)
```

最小渲染主线调整为：

```text
Application
-> 构造 Framebuffer
-> 构造 Mesh<vertex_t>
-> 构造 Program<vertex_t, uniforms_t, varyings_t>
-> 构造 uniforms_t
-> 构造 Pipeline<vertex_t, uniforms_t, varyings_t>
-> Framebuffer::Clear()
-> Pipeline::Run(framebuffer, mesh)
-> VertexShader 生成 varyings
-> FinalizeVertex 生成屏幕顶点
-> RasterizeTriangle 生成片元
-> ShadeFragments 显式调用 Program.fragmentShader
-> CommitFramebuffer 在 Run 末尾统一写入 framebuffer
```

职责拆分如下：

- `Framebuffer`
  - 负责颜色缓冲和深度缓冲存储
- `Pipeline`
  - 持有 `Program` 与 `Uniforms`
  - 在 `Run()` 中直接组织顶点着色、顶点整理、三角形光栅化、片元着色和结果写回
- `Application`
  - 构造 `Framebuffer`、`Mesh`、`Program`、`Uniforms`、`Pipeline`
  - 每帧清屏后调用 `pipeline.Run(framebuffer, mesh)`

本轮示例 shader 采用最小纯色路径：

- 顶点输入包含 `positionModel`
- uniforms 包含 `MVP` 和 `Color`
- varying 只使用基类字段，不额外扩展
- 顶点着色器负责生成 `ClipPos`
- 片元着色器直接返回 uniform 颜色

## 计划修改的文件

- `docs/features/minimal-programmable-pipeline.md`
- `src/render/Vertex.h`
- `src/render/Mesh.h`
- `src/render/Pipeline.h`
- `src/app/Application.cpp`
- `src/CMakeLists.txt`

## 风险点与兼容性影响

- `Mesh` 从固定顶点类型升级为模板后，调用点需要一起调整
- `Pipeline` 是模板类，主流程会放在头文件中实现
- 移除 `Renderer` 后，调用链会从 `pipeline.Run(renderer, ...)` 改为 `pipeline.Run(framebuffer, ...)`
- 为了让渲染阶段顺序更直观，`Run()` 会显式写出 `RasterizeTriangle()`、片元着色器调用和统一写回阶段
- 当前工作区中这些渲染文件已有未提交修改，实现时需要基于现状增量调整，不能回退已有内容
- 当前最小实现仍沿用现有屏幕空间包围盒和深度测试逻辑，尚未覆盖更完整的 clip 阶段

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行程序
   - `build-mingw/bin/sr_viewer.exe`
4. 观察窗口是否正常显示一个纯色三角形
5. 如结果异常，优先检查
   - 三角形是否完全消失
   - 是否只有背景色
   - 颜色是否正确
   - 关闭窗口后程序是否正常退出

## 完成标准 / 验收标准

1. 已补齐本轮开发文档
2. 已引入 `VertexBase`、`UniformsBase`、`VaryingsBase`
3. 已引入 `Program`、`VertexShader`、`FragmentShader` 最小接口
4. 已移除 `Renderer`
5. `Pipeline` 已直接通过 `Framebuffer` 完成绘制
6. `Run()` 中已显式体现光栅化和片元着色两个阶段
7. viewer 示例已切换为通过构建 `Pipeline` 来绘制
8. 已提供清晰的手动测试方法
9. 等待用户 review

