# 终端 FPS 输出

## 功能背景

当前 viewer 已经能持续渲染并通过平台层显示到窗口，但运行时缺少最基础的性能可观测信息。为了便于观察渲染循环是否稳定、交互时帧率是否明显波动，本次先补一个最小版本的实时 FPS 输出能力。

结合当前仓库现状，项目没有现成的调试 UI，也没有通用文字叠加系统，因此首版采用终端输出方案，在不改动软光栅主线的前提下提供运行时帧率反馈。

## 需求摘要

- 运行 viewer 时，终端能够持续看到实时 FPS。
- 终端输出应包含 FPS 和单帧耗时毫秒值。
- 输出频率应受控，避免每帧刷屏。
- 统计逻辑应接入现有 `Application::Run()` 主循环。

本次不做：

- 窗口标题栏 FPS
- 画面内 HUD 文字叠加
- 详细 profiling 分项
- 日志文件落盘
- 可配置输出频率或开关参数

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`
- `computer-graphics-notes/lesson01-project-overview-and-entry.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束逐帧时间统计和运行时调度应继续留在 `src/app`，不应把 FPS 统计塞进 `src/render` 的光栅化实现。
- 架构图约束平台层只负责显示和系统窗口，不应承担应用级统计策略。
- `lesson01` 启发本次应复用 `Application::Run()` 中已有的逐帧 `deltaTime` 入口。
- `lesson10` 启发 FPS 输出属于最小性能观测能力，应作为工程化辅助信息接入主循环，而不是侵入渲染算法。

## 当前代码与笔记差异

- lesson10 中的独立调试 UI 和 profiling 体系在当前仓库尚未完整落地。
- 当前仓库已有逐帧 `deltaTime` 计算，但没有 FPS 平滑统计或展示输出。
- 因此本次采用适配当前仓库的最小实现：在 `Application` 主循环中聚合帧统计并输出到终端。

## 技术方案与关键数据流

目标数据流：

```text
Application::Run()
-> 每帧计算 deltaTime
-> 累加帧数与累计时长
-> 达到固定统计窗口后计算 average FPS / frame time
-> 输出到 std::cout
-> 继续渲染与 Present
```

方案拆分：

1. 在 `src/app/Application.cpp` 中增加一个局部 FPS 统计器。
2. 每帧累加：
   - `frameCount`
   - `accumulatedSeconds`
3. 当累计时长达到固定窗口时：
   - 计算平均 FPS
   - 计算平均帧时长毫秒值
   - 输出一行终端信息
   - 清空窗口统计，开始下一轮累加
4. 输出内容采用单行覆盖或定期刷新，尽量减少终端刷屏噪音。

## 计划修改的文件

- `docs/features/console-fps-output.md`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- 终端输出过于频繁会影响可读性，因此需要固定刷新间隔。
- 若只输出瞬时 `deltaTime`，数值可能抖动较大，因此更适合输出统计窗口内平均值。
- 本次不改平台层、渲染层协议和 Framebuffer 结构，不预期影响当前渲染结果。
- 终端单行覆盖依赖控制字符表现；若终端行为不一致，需要退化为普通逐行输出。

## 手动测试方案

1. 编译 / 运行
   - 生成并运行 `sr_viewer`
2. 观察输出
   - 程序启动后，终端除已有启动日志外，应持续刷新 FPS 信息
   - 输出应包含 `FPS` 和 `Frame` 毫秒值
3. 运行时验证
   - 静止观察时，FPS 数值应稳定在一个区间内
   - 按 `W/A/S/D/Q/E/Space/LeftShift` 移动相机时，FPS 应继续刷新
4. 退出验证
   - 关闭窗口后程序应正常退出，不应卡在终端输出状态
5. 异常观察点
   - 若输出过快刷屏，优先检查统计窗口是否正确重置
   - 若 FPS 始终为 0 或异常大，优先检查 `deltaTime` 累加和除法分母

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成开发文档。
3. viewer 运行时终端能够持续看到 FPS 和帧时间输出。
4. 输出频率受控，不是每帧刷一整屏。
5. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
6. 已提供清晰的手动测试方法。
7. 等待用户 review。
