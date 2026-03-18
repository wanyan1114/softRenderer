# 最小可渲染流程与窗口三角形显示

## 功能背景

当前仓库已经具备最小应用层、平台层和基础数学库，但还没有真正的渲染闭环。程序现在只能创建一个隐藏窗口并输出数学 smoke check，距离“在窗口上看到一个三角形”还缺少最小颜色缓冲、渲染主循环、NDC 到屏幕映射和窗口显示链路。

本轮目标是补齐一条最短但可扩展的渲染路径，让项目第一次真正具备“可见渲染结果”。

在本轮 review 迭代中，平台层架构进一步收敛为“Platform 持有私有 Window 实例，对应用层提供统一平台门面”。也就是说，`Application` 不再直接操作 `Window`，而是只通过 `platform::Platform` 与窗口和消息泵交互。

## 需求摘要

- 启动程序后创建一个可见窗口并维持主循环。
- 在窗口中稳定显示一个纯色三角形。
- 三角形顶点使用 NDC 坐标定义，再映射到屏幕空间。
- 本轮只实现最小颜色缓冲与三角形光栅化。
- `Application` 不能直接依赖或调用 `Window`，必须通过 `Platform` 操作平台窗口。
- `Platform` 应持有私有 `Window` 实例，对外暴露平台级门面接口。
- 本轮暂不实现深度缓冲、MSAA、相机与 MVP 变换、模型与纹理加载、光照与 shader 系统。
- 代码结构需要具备后续扩展为更完整 renderer 的基础。

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/README.md`
- `computer-graphics-notes/lesson01-project-overview-and-entry.md`
- `computer-graphics-notes/lesson02-platform-window-and-input.md`
- `computer-graphics-notes/lesson04-framebuffer-depth-and-msaa.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

## 笔记对当前功能的约束与启发

- `lesson01` 约束最小渲染也要走 `main -> Application -> Run()` 的主循环。
- `lesson02` 约束平台细节和窗口实现应被平台层收口，而不是散落在应用层。
- `lesson04` 启发我们先实现最小 `Framebuffer`，只做颜色缓冲、清屏和像素写入，把深度/MSAA 留到后续迭代。
- `lesson05` 启发 renderer 骨架要有基础数据结构分层，哪怕本轮还不引入完整 programmable shader。
- `lesson06` 明确了当前最小主线应是：NDC 顶点 -> 屏幕映射 -> 包围盒遍历 -> inside test -> 写颜色 -> 显示。

## 技术方案与关键数据流

新增最小 `render` 模块，包含：

- `Color`
- `Framebuffer`
- `Vertex`
- `Renderer`

关键数据流为：

```text
Application
-> Platform::Initialize(title, width, height)
-> Platform 私有创建 Window
-> Renderer 写入 Framebuffer
-> Platform::Present(framebuffer)
-> Platform 内部调用 Window::Present(framebuffer)
```

- `Framebuffer` 负责颜色缓冲管理。
- `Renderer` 负责清屏、NDC 到屏幕映射、三角形光栅化。
- `Window` 负责 Win32 具体窗口与显示实现，但仅作为 `Platform` 的私有后端。
- `Platform` 作为应用层唯一可见的平台门面，统一管理窗口生命周期、消息泵和 present。
- `Application` 负责主循环和渲染调度，但不直接接触 `Window`。

本轮先让顶点只包含 `math::Vec2 positionNdc` 与 `Color color`，并保留 `Renderer::DrawTriangle()` 接口，方便后续扩展到更多属性与更多图元。

## 计划修改的文件

- `docs/features/minimal-renderable-pipeline-triangle.md`
- `src/app/Application.cpp`
- `src/platform/Platform.h`
- `src/platform/Platform.cpp`
- `src/platform/Window.h`
- `src/platform/Window.cpp`

## 风险点与兼容性影响

- 当前仓库已有未提交修改，本轮实现需要避免覆盖用户已有改动。
- `Platform` 接口会从轻量静态工具函数升级为持有状态的静态门面，调用方式会发生变化。
- Win32 像素格式如果和内部缓冲不一致，可能出现颜色通道错误或上下翻转问题。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行程序
   - `build-mingw/bin/sr_viewer.exe`
4. 观察窗口是否可见，并确认窗口中稳定显示一个三角形。
5. 尝试关闭窗口，确认程序正常退出。

## 完成标准 / 验收标准

1. 已更新开发文档。
2. 程序启动后能显示可见窗口。
3. 窗口中能渲染一个三角形。
4. `Application` 不直接依赖 `Window`，平台窗口交互全部通过 `Platform` 完成。
5. 已提供清晰的手动测试说明。
6. 等待用户 review。
