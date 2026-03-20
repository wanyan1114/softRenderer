# lesson03 相机架构对齐改造

## 功能背景

当前仓库已经具备最小窗口、资源加载、软光栅渲染和键盘交互能力，但相机逻辑仍直接散落在 `Application::Run()` 中：

- `Application` 直接读取平台输入
- `Application` 直接维护相机位置与朝向
- `Application` 直接拼装 `LookAt` 视图矩阵

这与最新 `lesson03-math-and-camera.md` 中给出的相机架构不一致。根据新的参考文档，相机相关逻辑应拆成：

```text
Application
-> CameraLayer
-> Renderer::Camera
-> 渲染逻辑消费相机矩阵与相机位置
```

本次目标是让当前项目的相机代码结构对齐这条主线，而不是继续把相机细节堆在 `Application.cpp` 里。

## 需求摘要

- 引入 `Renderer::Camera`，作为“纯数据 + 矩阵生成逻辑”的相机数据结构。
- 引入 `CameraLayer`，作为相机状态拥有者，负责：
  - `OnAttach()` 初始化相机
  - `OnUpdate()` 根据输入更新相机状态
  - 通过 `Get()` / `Camera()` 对外暴露当前相机
- `Application` 不再直接维护零散相机状态，而是：
  - 持有并驱动 `CameraLayer`
  - 在渲染前读取相机的 `ViewMat4()` / `ProjectionMat4()` / `Pos`
- 保留当前键盘交互方案：
  - `W / S` 前进 / 后退
  - `A / D` 左移 / 右移
  - `Space / Left Shift` 上移 / 下移
  - `Q / E` 绕世界 Y 轴旋转

本次不做：

- 完整 `LayerStack` 系统
- 鼠标视角
- ImGui 调试面板
- 多相机
- 场景系统重构

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson03-math-and-camera.md`
- `computer-graphics-notes/lesson02-platform-window-and-input.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束平台层只负责输入状态和窗口显示，不能把 Win32 细节泄漏到相机层或渲染层。
- `lesson03` 明确要求相机职责拆成 `CameraLayer` 和 `Renderer::Camera` 两层：
  - `CameraLayer` 负责维护状态
  - `Renderer::Camera` 负责生成矩阵
- `lesson03` 还约束了实现主线：
  - `OnAttach()` 初始化 `Aspect / Pos`
  - `OnUpdate()` 更新 `Pos / Dir / Right`
  - `ViewMat4()` / `ProjectionMat4()` 负责矩阵生成
  - 渲染逻辑只消费相机结果
- `lesson02` 约束输入仍通过 `Platform/Window` 提供状态查询，不直接在 `CameraLayer` 中接入 Win32 API。

## 当前代码与参考架构差异

- 当前仓库没有 `Renderer::Camera`。
- 当前仓库没有 `CameraLayer`。
- 当前仓库没有 `LayerStack`，因此本次只能做到“对齐相机架构核心分层”，而不是完整复制参考工程的层系统。
- 当前仓库的相机逻辑在 `Application.cpp` 中以匿名辅助结构和函数存在，这次需要把它迁移出去。

## 技术方案与关键数据流

目标数据流：

```text
Platform::ProcessEvents()
-> Window 维护按键状态
-> CameraLayer::OnUpdate(deltaTime)
-> Renderer::Camera 更新 Pos / Dir / Right / Up
-> Renderer::Camera::ViewMat4() / ProjectionMat4()
-> Application 读取当前相机并组装 uniform
-> Pipeline::Run(...)
```

方案拆分：

1. 新增 `src/render/Camera.h/.cpp`
   - 定义 `render::Camera`
   - 保存 `Pos / Right / Up / Dir / Aspect`
   - 提供 `ViewMat4()`、`ProjectionMat4()`
2. 新增 `src/app/CameraLayer.h/.cpp`
   - 拥有 `m_Camera`
   - 提供 `OnAttach()`、`OnUpdate()`、`Camera()`、`Get()`
   - 内部维护背景色和相机控制参数
3. 修改 `Application`
   - 持有 `CameraLayer`
   - 创建 `Framebuffer` 后调用 `OnAttach()`
   - 每帧调用 `OnUpdate()`
   - 渲染时通过 `CameraLayer` 获取 `camera.Pos`、`camera.ViewMat4()`、`camera.ProjectionMat4()`
4. 修改工程文件
   - 把新的 `CameraLayer.cpp`、`Camera.cpp` 加入 `sr_engine`
5. 视情况同步架构文档
   - 如果改动后关键数据流已显式经过 `CameraLayer / Renderer::Camera`，则更新架构文档描述

## 计划修改的文件

- `docs/features/camera-architecture-alignment.md`
- `src/CMakeLists.txt`
- `src/app/Application.h`
- `src/app/Application.cpp`
- `src/app/CameraLayer.h`
- `src/app/CameraLayer.cpp`
- `src/render/Camera.h`
- `src/render/Camera.cpp`
- `docs/architecture/project-architecture.md`（如需同步）

## 风险点与兼容性影响

- 本次属于结构重构，核心风险是迁移过程中把当前可运行的相机交互打坏。
- 由于没有完整 `LayerStack`，本次 `CameraLayer` 是“对齐 lesson03 核心思想”的轻量落地版，后续如果真要引入层系统，还需要进一步演进。
- 如果 `CameraLayer::Get()` 设计不稳，后续扩展多实例时会有限制，因此本次默认只服务单主相机场景。
- 这次重构不应改变平台输入边界，也不应让渲染层反向依赖 `app`。

## 手动测试方案

1. 编译 / 运行
   - `cmake --build build-mingw --target sr_viewer`
   - `build-mingw/bin/sr_viewer.exe`
2. 相机初始化
   - 启动后应能正常看到当前立方体场景
   - 初始视角应与本次改造前大体一致
3. 相机交互
   - `W / S`：前进 / 后退
   - `A / D`：左移 / 右移
   - `Space / Left Shift`：上移 / 下移
   - `Q / E`：左右转向
4. 结构验证
   - 若渲染正常、相机可动，说明 `Application -> CameraLayer -> Renderer::Camera -> 渲染消费` 链路已通
5. 异常观察点
   - 若相机不动，优先检查 `CameraLayer::OnUpdate()` 是否被逐帧调用
   - 若视图异常，优先检查 `Renderer::Camera::ViewMat4()` 和 `ProjectionMat4()`
   - 若移动方向异常，优先检查 `Dir / Right / Up` 的更新与归一化

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成新的开发文档。
3. 当前项目已引入 `CameraLayer`。
4. 当前项目已引入 `Renderer::Camera`。
5. `Application` 不再直接维护零散相机状态。
6. 编译通过，运行时相机交互仍可使用。
7. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
8. 已提供清晰的手动测试方法。
9. 等待用户 review。
