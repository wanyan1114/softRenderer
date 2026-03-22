# 阶段一：Application / LayerStack / CameraLayer / RenderLayer 结构重构

## 功能背景

本轮 review 已确认当前项目在应用层和相机层存在多处结构问题：

- `Application` 不应直接持有具体 `Layer`
- `Application::Run()` 同时承担启动、资源装配、输入驱动、渲染分发、统计输出与退出清理
- 当前项目需要一个公共 `Layer` 抽象类和 `LayerStack`
- `CameraLayer` 与 `Camera` 的边界需要继续拉开
- `OnUpdate` 与 `OnRender` 的阶段职责需要明确
- 当前版本只保留 OBJ 渲染路径，其它格式支持移除
- `Application` 不应随着 layer 增长不断新增 `FindXxxLayer()`
- 当前版本不再保留 BMP 导出分支
- `RenderLayer` 不应承担资源加载、启动流程控制和相机依赖获取
- 层之间的共享数据不希望通过服务接口协作，而应通过统一 context 传递

本阶段不是一次性重构全部模块，而是先把应用层主干结构理顺，为后续 `Platform`、`Vertex.h`、`Pipeline` 重构打基础。

## 需求摘要

- 引入公共 `Layer` 抽象类
- 引入 `LayerStack`，由 `Application` 持有并统一管理 layer 生命周期与逐帧分发
- `Application` 不再直接持有或缓存任何具体 `Layer` 实例
- `CameraLayer` 继承自 `Layer`，以私有成员形式持有 `render::Camera`
- `Application` 负责 OBJ 场景资源加载、场景资源拥有、启动信息构造、启动异常处理
- `RenderLayer` 只负责渲染执行：读取 `Camera`、读取已准备好的场景视图、写入 `Framebuffer`
- 引入 `LayerContext`，由 `Application` 创建并维护，作为各层共享数据的唯一通道
- 明确逐帧边界：
  - `OnUpdate`：游戏逻辑 / 相机控制 / 动画推进 / 物理推进
  - `OnRender`：读取已经准备好的渲染数据并执行渲染提交
- `Application::Run()` 拆分成更清晰的调度流程，并且只保留 FPS 输出
- 提升 `Camera::ViewMat4()` 的可读性
- 删除 STL 与其它非 OBJ 渲染路径
- 删除渲染耗时统计逻辑，只保留 FPS
- 删除 BMP 导出逻辑与 `SR_EXPORT_DEMO_IMAGES` 分支
- 删除 `LayerServices.h`，不再通过服务接口查询层能力

本阶段不做：

- `Platform` 职责收缩
- `Vertex.h` 拆分
- `Pipeline` 无状态化
- `Stage` 状态机
- `Loaders` 深度重构

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson03-math-and-camera.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束 `Application` 继续承担主循环调度职责，`render` 继续承担渲染职责，`platform` 继续承担窗口显示职责
- `lesson03` 约束 `Camera` 是纯相机数据与矩阵逻辑，`CameraLayer` 是维护相机状态的层
- `lesson05` 启发渲染提交应消费已经准备好的相机与场景数据，而不是在渲染阶段反向做状态更新
- `lesson10` 启发 `Layer` 与逐帧阶段分发适合作为应用层的组织骨架

## 当前代码与目标结构差异

- 旧版本 `Application` 直接持有具体相机层
- 旧版本没有 `Layer` 抽象类和 `LayerStack`
- 旧版本没有独立的 `RenderLayer`
- 旧版本 `Application::Run()` 承担过多职责
- 旧版本保留了 STL 等非 OBJ 渲染路径
- 旧版本包含多项渲染耗时统计输出
- 旧版本保留 BMP 导出分支
- 旧版本通过 `FindRenderLayer()` / `FindCamera()` 感知具体 layer 类型
- 旧版本把资源加载、启动控制、相机依赖获取混在 `RenderLayer`
- 旧版本通过服务接口查询层能力，而不是通过共享 context 协作
- 旧版本 `Camera::ViewMat4()` 可读性较弱

## 技术方案与关键数据流

目标结构：

```text
main
-> Application
-> LayerStack
   -> CameraLayer
   -> RenderLayer
```

目标职责：

- `Application`
  - 持有 `LayerStack`
  - 管主循环、时间、平台事件、present 和退出
  - 不直接持有或缓存任何具体 `Layer` 实例
  - 负责加载 OBJ、拥有场景资源、构造启动信息、处理启动失败
  - 创建并维护 `LayerContext`
- `Layer`
  - 作为公共抽象接口，提供带 `LayerContext` 的 `OnAttach / OnDetach / OnUpdate / OnRender`
- `LayerStack`
  - 持有 layer 容器
  - 顺序分发 layer 回调
  - 不承担场景数据管理，只负责把 `LayerContext` 传递给各层
- `LayerContext`
  - 持有层间共享数据，例如当前 `Camera`、场景视图、`Framebuffer`
- `CameraLayer`
  - 在 `OnUpdate()` 内更新相机状态
  - 内部私有持有 `render::Camera`
  - 把当前相机写入 `LayerContext`
  - 不负责真正绘制
- `RenderLayer`
  - 在 `OnRender()` 内只执行渲染
  - 从 `LayerContext` 读取当前相机和场景视图
  - 清屏、组装 uniforms、调用 pipeline、写入 `Framebuffer`
  - 不负责资源加载、启动信息和启动控制

关键数据流：

```text
Application::PrepareScene()
-> resource::LoadLitMesh()
-> Application 拥有 OBJ mesh / texture / model
-> Application 写入 LayerContext.sceneView
-> Application 写入 LayerContext.framebuffer

Platform::ProcessEvents()
-> Application 主循环
-> LayerStack::OnUpdate(deltaTime, context)
-> CameraLayer::OnUpdate(deltaTime, context)
-> CameraLayer 写 context.activeCamera
-> LayerStack::OnRender(context)
-> RenderLayer::OnRender(context)
-> RenderLayer 读 context.activeCamera + context.sceneView
-> Pipeline::Run(...)
-> context.framebuffer
-> Platform::Present(framebuffer)
```

本阶段采用的共享方式：

- 不引入额外 `AppContext / FrameContext`
- 直接以 `LayerContext` 作为 layer 系统内部共享数据结构
- `Application`、`CameraLayer`、`RenderLayer` 只通过 `LayerContext` 协作
- 删除 `LayerServices.h`

## 计划修改的文件

- `docs/features/stage1-layer-stack-application-refactor.md`
- `src/CMakeLists.txt`
- `src/app/Application.h`
- `src/app/Application.cpp`
- `src/app/CameraLayer.h`
- `src/app/CameraLayer.cpp`
- `src/app/Layer.h`
- `src/app/LayerContext.h`
- `src/app/LayerStack.h`
- `src/app/RenderLayer.h`
- `src/app/RenderLayer.cpp`
- `src/render/Camera.cpp`
- `docs/architecture/project-architecture.md`
- 删除 `src/app/LayerServices.h`
- 删除 `src/resource/loaders/StlMeshLoader.h`
- 删除 `src/resource/loaders/StlMeshLoader.cpp`

## 风险点与兼容性影响

- `Layer` 回调签名变化会波及整个 `src/app` 主链
- `Application` 接管资源加载后，应用层会暂时承担更多场景装配代码
- `LayerContext` 如果塞入过多无关状态，后续会变成新的大杂烩
- `CameraLayer` 和 `RenderLayer` 的更新、渲染顺序需要稳定
- 本阶段变更会影响 `src/app` 的职责描述，完成后需要同步更新架构文档
- 删除 STL 文件后，本版本 viewer 不再接受 `.stl` 输入
- 删除 BMP 导出后，不再支持截图式 demo 导出流程

## 手动测试方案

1. 编译 / 运行
   - `cmake --build build-mingw --target sr_viewer`
   - `build-mingw/bin/sr_viewer.exe`
2. 启动验证
   - 默认 OBJ 场景仍能正常启动并显示
   - 如设置 `SR_MODEL_PATH`，只接受 `.obj` 路径
3. FPS 输出验证
   - 控制台只输出 `FPS`
4. 相机控制验证
   - `W / S / A / D / Space / Left Shift / Q / E` 仍能正常更新视角
5. 渲染职责验证
   - `CameraLayer` 只负责更新相机并写入 `LayerContext`
   - `RenderLayer` 只负责从 `LayerContext` 读取数据并写入 `Framebuffer`
   - 资源加载与启动日志都由 `Application` 负责
6. 如结果异常，优先检查
   - `Application::PrepareScene()` 是否正确准备 `LayerContext.sceneView`
   - `LayerStack` 是否正确分发带 context 的回调
   - `CameraLayer` 是否在渲染前正确更新 `context.activeCamera`
   - `Camera::ViewMat4()` 是否仍生成正确观察矩阵

## 完成标准 / 验收标准

1. 已完成需求对齐
2. 已生成开发文档
3. 已引入 `Layer` 抽象类和 `LayerStack`
4. `Application` 不再直接持有或缓存任何具体 `Layer` 实例
5. 已引入 `RenderLayer`
6. `OnUpdate` 与 `OnRender` 边界已按约定落实
7. `Application::Run()` 已完成第一轮职责拆分
8. `Camera::ViewMat4()` 可读性已提升
9. 已检查并在必要时更新 `docs/architecture/project-architecture.md`
10. 已删除非 OBJ 渲染路径、渲染耗时统计与 BMP 导出逻辑
11. 已把资源加载、启动控制和相机获取上提回 `Application`
12. 已切换到 `LayerContext` 协作模式
13. 已提供清晰的手动测试方法
14. 等待用户 review
