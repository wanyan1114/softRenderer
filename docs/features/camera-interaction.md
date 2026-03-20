# 相机交互

## 功能背景

当前 viewer 已经具备最小模型加载、基础光照和窗口显示能力，但相机参数仍固定写死在 `Application::Run()` 中。运行时无法移动或转动视角，导致观察模型、验证投影和调试不同面的可见性都不够方便。

结合当前项目所处阶段，这次迭代先补上“最小可用”的相机交互链路，让用户可以通过键盘控制观察位置和朝向，并保持现有软光栅主线不变。

## 需求摘要

- 为当前 viewer 增加最小键盘相机交互。
- 支持以下控制：
  - `W / S`：沿相机前向前进 / 后退
  - `A / D`：沿相机右向左移 / 右移
  - `Space / Left Shift`：沿世界上方向上移 / 下移
  - `Q / E`：绕世界 Y 轴左转 / 右转
- 每帧根据相机状态重新生成 `view` 矩阵，并参与现有渲染流程。
- 需要保证窗口失焦后不会出现卡键。

本次不做：

- 鼠标视角控制
- 相机俯仰
- FOV 调节
- 相机惯性 / 平滑阻尼
- 调试 UI
- 场景系统或独立相机模块大重构

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson02-platform-window-and-input.md`
- `computer-graphics-notes/lesson03-math-and-camera.md`
- `computer-graphics-notes/README.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束输入采集仍应停留在 `src/platform`，应用层只消费抽象后的输入状态，不直接依赖 Win32 细节。
- 架构图约束相机状态更新和视图矩阵组装应由 `src/app` 调度，`src/render` 继续只消费最终矩阵结果。
- `lesson02` 启发本次应补齐“系统消息 -> 窗口内部状态 -> 应用层查询”的最小输入链路，而不是直接在应用层轮询 Win32。
- `lesson03` 启发本次相机交互优先采用“位置 + 前向 + LookAt”的实现方式，先保持世界上方向固定，再做绕世界 Y 轴旋转。
- `README` 映射说明当前仓库的学习主题与完整 RGS 工程并不完全同构，因此本次以“对齐当前仓库结构的最小实现”为准，不照搬 `CameraLayer`。

## 当前代码与笔记差异

- 笔记中提到的 `CameraLayer`、`Renderer::Camera`、`m_Keys[]` 结构在当前仓库尚未落地。
- 当前仓库的相机参数直接写死在 `src/app/Application.cpp` 中，平台层也只提供窗口创建、事件处理和 Present，没有键盘状态查询接口。
- 因此本次实现会在不引入完整 Layer 系统的前提下，补齐一条适配当前仓库的最小相机交互链路。

## 技术方案与关键数据流

目标数据流：

```text
Win32 KeyDown / KeyUp / KillFocus
-> platform::Window 更新内部按键状态
-> platform::Platform 暴露键盘查询接口
-> Application 每帧读取输入并更新 camera position / forward
-> LookAt(cameraPos, cameraPos + forward, worldUp)
-> Pipeline::Run(...)
-> Framebuffer
-> Platform::Present(...)
```

方案拆分：

1. 在 `src/platform` 增加最小键盘输入枚举与查询接口。
2. `Window` 维护按键按下状态，并在失焦时清空状态。
3. `Application` 引入轻量相机状态：
   - `position`
   - `forward`
   - `moveSpeed`
   - `turnSpeed`
4. 每帧基于 `deltaTime` 更新相机状态，再重新计算 `view` 矩阵。
5. 相机平移中的上下移动使用世界上方向 `(0, 1, 0)`，水平转向通过 `RotateY` 更新前向。

## 计划修改的文件

- `docs/features/camera-interaction.md`
- `src/platform/Input.h`
- `src/platform/Window.h`
- `src/platform/Window.cpp`
- `src/platform/Platform.h`
- `src/platform/Platform.cpp`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- 这次会改变 viewer 的运行时行为：画面不再固定，而是可交互移动。
- 如果按键状态与窗口生命周期处理不好，容易出现失焦卡键或关闭窗口后状态未清理的问题。
- 当前只做绕世界 Y 轴转向，不做俯仰，因此不会出现完整自由视角，但也能降低首版实现复杂度和数值不稳定风险。
- review 迭代发现固定 `sleep_for(16ms)` 会让交互观感偏“一帧帧跳动”，本轮需要把主循环改成更细粒度的逐帧更新，降低固定休眠造成的节奏量化。
- 本次不改变 `render` 模块协议与渲染主线，不预期影响现有着色和光栅化逻辑。

## 手动测试方案

1. 编译 / 运行
   - 生成并运行 `sr_viewer`
2. 基础交互
   - 启动后观察立方体初始视角应与修改前接近
   - 按 `W / S`，相机应前进 / 后退
   - 按 `A / D`，相机应左移 / 右移
   - 按 `Space / Left Shift`，相机应上移 / 下移
   - 按 `Q / E`，相机应绕世界 Y 轴左转 / 右转
3. 边界验证
   - 松开按键后，相机应停止运动
   - 将窗口切走再切回，不应持续发生移动
   - 关闭窗口后程序应正常退出
4. 异常观察点
   - 若移动方向异常，优先检查 `forward/right/worldUp` 的构造与归一化
   - 若转向后速度不稳定，优先检查旋转后前向向量是否归一化
   - 若出现卡键，优先检查 `WM_KILLFOCUS` 和按键状态清空逻辑

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成开发文档。
3. viewer 能在运行时响应约定键盘输入。
4. 每帧 `view` 矩阵随相机状态变化而更新。
5. 窗口失焦后不会卡键。
6. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
7. 已提供清晰的手动测试方法。
8. 等待用户 review。

## Review 迭代记录

### 迭代 1：相机变换观感跳动

review 反馈：

- 当前按住按键时，相机不是足够连续地变换，而是有明显按帧跳动感。

修正思路：

- 保持现有输入状态链路不变，问题优先定位为主循环固定 `16ms` 休眠带来的更新节奏量化。
- 取消固定 `sleep_for(16ms)` 帧限速，改为每帧完成后只做很轻的 `sleep_for(1ms)` 让步，让相机更新随实际帧节奏更细地推进。
- 保留 `deltaTime` 驱动，这样交互速度仍与机器性能解耦。


### 迭代 2：固定睡眠优化后仍有明显跳动

review 反馈：

- 去掉固定 `16ms` 帧睡眠后，连续性有所改善，但仍有明显跳动。

修正思路：

- 保持 `platform` 输入状态链路不变，继续在 `src/app` 解决观感问题。
- 将相机拆成“目标相机状态”和“实际渲染相机状态”：
  - 输入只更新目标相机
  - 渲染相机按指数平滑逐帧追近目标相机
- 平滑位置和朝向，减少在当前渲染帧率下的离散跳变感，同时保留 `deltaTime` 驱动和松键后可停止的行为。
