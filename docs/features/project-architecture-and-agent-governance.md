# 项目架构图与 Agent 维护流程补充

## 功能背景

当前仓库已经有分层代码结构，也已经有面向功能开发的 `AGENTS.md` 流程约束，但还缺少一份稳定的“项目级架构图 + 模块边界说明”作为共同上下文。这样会带来两个问题：

- 开发前虽然会读具体 lesson，但不一定先建立当前仓库的整体分层认知。
- 当后续功能改动影响模块职责或调用方向时，没有明确要求同步维护项目架构文档。

本轮目标是补齐这份项目级架构文档，并把它纳入开发前必读、开发后必要时必改的流程。

## 需求摘要

- 生成当前项目的架构图。
- 说明当前各模块的职责与边界条件。
- 将架构图文档纳入开发前必读材料。
- 更新 Agent 规范，要求如果代码开发导致模块关系、职责边界或关键数据流变化，需要同步更新架构图文档。
- 保持 `AGENTS.md` 和 `agent.md` 入口描述一致。

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/README.md`
- `computer-graphics-notes/lesson01-project-overview-and-entry.md`
- `computer-graphics-notes/lesson02-platform-window-and-input.md`
- `computer-graphics-notes/lesson03-math-and-camera.md`
- `computer-graphics-notes/lesson04-framebuffer-depth-and-msaa.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

## 笔记对当前功能的约束与启发

- `lesson01` 约束架构图必须优先表达入口、应用调度和主循环。
- `lesson02` 提醒平台层与窗口显示链路需要和应用层解耦，边界必须明确。
- `lesson03` 说明数学模块是渲染数据流的基础支撑，而不是业务调度层。
- `lesson04` 说明 `Framebuffer` 是跨渲染与显示的关键共享数据容器。
- `lesson05` 说明渲染模块内部需要区分 `Mesh / Program / Uniforms / Varyings / Pipeline` 的协议边界。
- `lesson06` 说明架构图除了目录结构，还应覆盖实际的“顶点到像素”主流程。

## 技术方案与关键数据流

本轮新增一份项目级文档：

- `docs/architecture/project-architecture.md`

文档内容包括：

- 静态模块依赖图
- 运行时逐帧数据流
- 各模块职责与边界条件
- 架构图维护规则

同时更新：

- `AGENTS.md`
- `agent.md`

让后续功能开发在阅读 lesson 之前或同时，必须先对齐当前架构图；如果开发导致架构发生变化，则在交付前同步更新该文档。

## 计划修改的文件

- `docs/features/project-architecture-and-agent-governance.md`
- `docs/architecture/project-architecture.md`
- `AGENTS.md`
- `agent.md`

## 风险点与兼容性影响

- 架构图是对当前仓库状态的抽象，若后续结构变化但未同步维护，文档会失真。
- 当前仓库仍是骨架版软光栅实现，文档需要明确“当前已有模块”和“尚未落地的未来方向”之间的边界，避免误导。
- `AGENTS.md` 约束增强后，后续功能开发流程会更严格，需要保持文字表述清晰，避免执行歧义。

## 手动测试方案

1. 打开 `docs/architecture/project-architecture.md`，确认能看到项目架构图、模块边界和维护规则。
2. 打开 `AGENTS.md`，确认新增了“开发前阅读架构图”与“开发后必要时更新架构图”的要求。
3. 打开 `agent.md`，确认入口文件也提示同时阅读架构文档。
4. 抽查当前代码目录 `apps/viewer`、`src/app`、`src/platform`、`src/render`、`src/base`，确认文档描述与现状一致。

## 完成标准 / 验收标准

1. 已生成项目级架构文档。
2. 架构文档中包含整体架构图与模块边界条件。
3. `AGENTS.md` 已要求开发前阅读架构图。
4. `AGENTS.md` 已要求开发后在必要时更新架构图。
5. `agent.md` 入口说明与主规范保持一致。
6. 已提供手动检查方式。
7. 等待用户 review。
