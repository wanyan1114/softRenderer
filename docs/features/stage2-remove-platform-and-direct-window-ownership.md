# 阶段二：删除 Platform，并继续收缩 Application 状态所有权

## 功能背景

阶段二第一轮完成后，`Platform` 已被删除，`Application` 直接持有 `Window`，输入状态也已经进入 `LayerContext`。但 review 继续指出了两个更深层的问题：

- `Application` 不应继续持有 scene / model / OBJ 资源
- `Application` 不应持有 `LayerContext` 或 `Framebuffer` 这类运行期共享状态

如果继续让 `Application` 同时持有窗口、层栈、场景资源、启动状态和渲染状态，它仍然会逐步滑回“大杂烩调度器”。本轮目标是把这些状态所有权进一步下沉，让 `Application` 回到最小外壳。

## 需求摘要

- 保持 `Platform` 已删除的结果不变
- `Application` 只持有：
  - `Window`
  - `LayerStack`
  - 标题、宽高
- `LayerContext` 改为 `Application::Run()` 内局部变量
- `Framebuffer` 改为 `Application::Run()` 内局部变量
- 新增 `SceneLayer`
- `SceneLayer` 负责：
  - OBJ 路径选择
  - OBJ 资源加载
  - scene/model 数据拥有
  - 启动信息与启动失败状态输出
  - 把 `RenderSceneView` 写入 `LayerContext`
- `CameraLayer` 继续：
  - 私有持有 `Camera`
  - 从 `context.input` 读输入
  - 写 `context.activeCamera`
- `RenderLayer` 继续：
  - 不持有 `Framebuffer`
  - 不持有 scene/model 资源
  - 只读 `context.activeCamera + context.sceneView`
  - 只写 `context.framebuffer`
- `LayerContext` 只保留共享出口，不承载大对象所有权
- 更新架构文档，加入 `SceneLayer`

本阶段不做：

- `Vertex.h` 拆分
- `Pipeline` 无状态化
- `Stage` 状态机
- `Loaders` 深度重构

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson02-platform-window-and-input.md`
- `computer-graphics-notes/lesson03-math-and-camera.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束 `Application` 应继续收敛为总调度器，而不是资源拥有者
- `lesson02` 启发窗口和输入应是运行时外壳的一部分，但不应该把 scene/render 状态一起塞回应用根对象
- `lesson03` 约束 `Camera` 仍应由 `CameraLayer` 私有持有
- `lesson10` 启发共享状态可以通过帧级上下文流动，但不应重新变成类成员杂糅

## 当前代码与目标结构差异

- 当前 `Application` 仍持有 `Framebuffer`
- 当前 `Application` 仍持有 `LayerContext`
- 当前 `Application` 仍持有 `model / modelPath / ObjLoadResult / SceneParts / SceneView / startupMessage`
- 当前项目还没有独立的 `SceneLayer`

## 技术方案与关键数据流

目标结构：

```text
Application
-> Window
-> LayerStack
   -> SceneLayer
   -> CameraLayer
   -> RenderLayer
```

`Application::Run()` 内部局部对象：

```text
Framebuffer framebuffer
StartupState startupState
LayerContext context
```

目标职责：

- `Application`
  - 只负责窗口生命周期、主循环、present、退出
  - 在 `Run()` 内创建局部 `Framebuffer / StartupState / LayerContext`
- `SceneLayer`
  - 持有场景和模型资源
  - 在 `OnAttach()` 加载 OBJ
  - 写 `context.sceneView`
  - 写 `context.startupState`
- `CameraLayer`
  - 持有相机
  - 在 `OnUpdate()` 写 `context.activeCamera`
- `RenderLayer`
  - 在 `OnRender()` 读取 `context.activeCamera / context.sceneView / context.framebuffer`
  - 执行渲染

关键数据流：

```text
Application::Run()
-> 创建 framebuffer / startupState / context
-> LayerStack::OnAttach(context)
-> SceneLayer::OnAttach(context)
-> 加载 OBJ
-> SceneLayer 写 context.sceneView + startupState
-> CameraLayer::OnAttach(context)
-> 写默认 activeCamera
-> while (Window::ProcessEvents())
-> Application 写 context.input
-> LayerStack::OnUpdate(deltaTime, context)
-> CameraLayer 写 context.activeCamera
-> LayerStack::OnRender(context)
-> RenderLayer 读 context.activeCamera / context.sceneView / context.framebuffer
-> Window::Present(framebuffer)
```

## 计划修改的文件

- `docs/features/stage2-remove-platform-and-direct-window-ownership.md`
- `src/app/Application.h`
- `src/app/Application.cpp`
- `src/app/LayerContext.h`
- `src/app/SceneLayer.h`
- `src/app/SceneLayer.cpp`
- `src/app/CameraLayer.cpp`
- `src/app/RenderLayer.cpp`
- `src/CMakeLists.txt`
- `docs/architecture/project-architecture.md`

## 风险点与兼容性影响

- `SceneLayer` 引入后，layer 顺序需要稳定
- `LayerContext` 改成局部变量后，所有回调都必须只消费当前帧上下文，不能假设长期持有
- `StartupState` 通过 context 传递后，启动失败路径需要谨慎验证
- 本轮继续改变 `src/app` 内部职责边界，需要同步更新架构文档

## 手动测试方案

1. 编译 / 运行
   - `cmake --build build-mingw --target sr_viewer`
   - `build-mingw/bin/sr_viewer.exe`
2. 启动验证
   - 默认 OBJ 场景能正常显示
   - 非 `.obj` 路径会在启动阶段直接报错并退出
3. 输入验证
   - `W / A / S / D / Q / E / Space / Left Shift` 仍能正确控制相机
4. 边界验证
   - `Application.h` 中不再持有 `Framebuffer / LayerContext / SceneView / ObjLoadResult`
   - `RenderLayer` 不持有 `Framebuffer`
   - `SceneLayer` 持有场景/model/OBJ 数据
5. 渲染验证
   - `RenderLayer` 仍能正常写入 `context.framebuffer`
   - `Window::Present()` 仍能正常显示画面
6. 如结果异常，优先检查
   - `SceneLayer::OnAttach()` 是否正确写入 `context.sceneView`
   - `CameraLayer::OnUpdate()` 是否正确写入 `context.activeCamera`
   - `Application::Run()` 是否正确创建并传递局部 `LayerContext`

## 完成标准 / 验收标准

1. 已完成需求对齐
2. 已生成开发文档
3. `Application` 已不再持有 scene/model/render 运行状态
4. `LayerContext` 已改为 `Run()` 局部变量
5. `Framebuffer` 已改为 `Run()` 局部变量
6. 已新增 `SceneLayer`
7. `RenderLayer` 已只保留渲染执行职责
8. 已检查并更新 `docs/architecture/project-architecture.md`
9. 已提供清晰的手动测试方法
10. 等待用户 review
