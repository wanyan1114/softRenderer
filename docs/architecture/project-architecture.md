# 项目架构图与模块边界

## 1. 文档目的

这份文档描述当前 `softRanderer` 仓库已经落地的项目结构，而不是理想中的完整引擎形态。

它服务两个目标：

- 在开始新功能前，先建立“入口、调度、资源、平台、渲染、数学”的整体地图。
- 在功能开发影响模块职责、依赖方向或关键数据流时，作为必须同步维护的架构基线。

## 2. 本次已阅读的笔记文件

- `computer-graphics-notes/README.md`
- `computer-graphics-notes/lesson01-project-overview-and-entry.md`
- `computer-graphics-notes/lesson02-platform-window-and-input.md`
- `computer-graphics-notes/lesson03-math-and-camera.md`
- `computer-graphics-notes/lesson04-framebuffer-depth-and-msaa.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

## 3. 这些笔记对当前架构文档的约束与启发

- `lesson01` 约束我们先从 `main -> Application -> 主循环` 画起，架构图应优先反映总调度关系。
- `lesson02` 约束平台层必须封装窗口与系统显示细节，应用层不应直接依赖 Win32 API。
- `lesson03` 启发我们把数学模块视为“基础支撑层”，它为变换和相机提供统一语言。
- `lesson04` 约束 `Framebuffer` 作为 CPU 渲染结果与窗口显示之间的桥接数据结构。
- `lesson05` 启发渲染模块内部要明确区分顶点输入、统一参数、插值数据和流水线执行器。
- `lesson06` 约束运行时主线要能表达“顶点 -> 三角形 -> 像素 -> 帧缓冲 -> 窗口”的数据流。

## 4. 当前项目架构总览

```mermaid
flowchart LR
    viewer["apps/viewer/main.cpp"]
    app["src/app/Application + CameraLayer"]
    resource["src/resource\nOBJ Loader"]
    base["src/base\nMath"]
    render["src/render\nCamera / Color / Vertex / Mesh / Texture2D / Pipeline / Framebuffer"]
    platform["src/platform/Platform"]
    window["src/platform/Window"]
    win32["Win32 + GDI"]
    docs["docs/architecture/project-architecture.md"]

    viewer --> app
    app --> base
    app --> resource
    app --> render
    app --> platform
    resource --> render
    platform --> window
    window --> win32
    render --> base
    window -.读取像素并显示.-> render
    docs -.开发前阅读/开发后必要时更新.-> app
    docs -.约束模块边界.-> resource
    docs -.约束模块边界.-> platform
    docs -.约束模块边界.-> render
```

可以把当前仓库理解成 6 个运行时层次：

1. `apps/viewer`
   程序入口层，只负责构造应用并交出控制权。
2. `src/app`
   应用调度层，负责生命周期、主循环、资源装配与渲染调用编排。
3. `src/resource`
   资源接入层，负责把磁盘资源解析成运行时可消费的数据结构。
4. `src/platform`
   平台适配层，负责窗口创建、消息泵、输入状态维护和最终显示。
5. `src/render`
   软光栅核心层，负责 CPU 侧三角形处理与帧缓冲写入。
6. `src/base`
   基础数学层，负责向量、矩阵与变换工具。

## 5. 当前运行时主链路

### 5.1 启动链路

```mermaid
sequenceDiagram
    participant Main as main.cpp
    participant App as Application
    participant Platform as platform::Platform
    participant Window as platform::Window
    participant Loader as resource::LoadLitMesh
    participant FB as render::Framebuffer

    Main->>App: 构造 Application(title, width, height)
    Main->>App: Run()
    App->>Platform: Initialize(title, width, height)
    Platform->>Window: Create()
    App->>Loader: 读取 assets/models/cube.obj + cube.mtl + map_Kd 纹理
    Loader-->>App: render::Mesh<LitVertex> + Texture2D
    App->>FB: 构造 Framebuffer(width, height)
    App->>Platform: Show()
```

### 5.2 逐帧链路

```text
while (Platform::ProcessEvents())
-> Application 驱动 CameraLayer::OnUpdate()
-> CameraLayer 更新 Renderer::Camera 的 Pos / Dir / Right
-> Application 读取 camera.ViewMat4() / ProjectionMat4()
-> Application 清颜色/清深度
-> Pipeline::Run(framebuffer, mesh)
-> 顶点着色
-> NDC/屏幕映射
-> 三角形光栅化
-> 深度测试与颜色写入
-> Platform::Present(framebuffer)
-> Window::Present(framebuffer)
-> StretchDIBits 显示到 Win32 窗口
```

### 5.3 关键数据流

```text
Win32 keyboard messages
-> Window::OnMessage()
-> Window 内部按键状态
-> Platform::IsKeyDown()
-> CameraLayer::OnUpdate(deltaTime)
-> render::Camera(Pos / Dir / Right / Up / Aspect)
-> Camera::ViewMat4() / ProjectionMat4()

OBJ(v + vt + vn) + mtllib/usemtl
-> resource::LoadLitMesh()
-> 解析 MTL(newmtl + map_Kd)
-> 解码 PNG/JPG/JPEG 为 Texture2D
-> LitVertex / DebugFaceVertex / Mesh
-> Pipeline::Run()
-> vertexShader 输出 clipPos + worldPos + worldNormal + uv
-> 屏幕空间三角形遍历
-> fragmentShader 采样 Texture2D 并计算基础光照
-> Framebuffer(pixels + depth)
-> Window::Present()
-> Win32 GDI
```

## 6. 模块职责与边界条件

下面的“边界条件”重点回答四件事：

- 这个模块负责什么
- 它可以依赖谁
- 它向外暴露什么
- 它明确不该做什么

### 6.1 `apps/viewer`

**职责**

- 提供可执行程序入口。
- 构造 `sr::Application` 并调用 `Run()`。

**输入**

- 启动参数当前未接入，固定使用标题和窗口尺寸。

**输出**

- 进程级返回码。

**边界条件**

- 只做入口装配，不承载渲染、平台或数学逻辑。
- 不直接操作 `platform::Platform`、`platform::Window`、`render::*`。
- 入口如果需要新增模式切换、命令行解析，也应以“配置应用”而不是“替代应用主循环”为原则。

### 6.2 `src/app`

**职责**

- 作为运行时总调度器管理生命周期。
- 创建平台窗口与帧缓冲。
- 组织每帧输入读取、驱动 `CameraLayer`、清屏、渲染、显示和退出流程。
- 组装当前示例场景的模型矩阵、uniforms、pipeline、mesh 与 base color 纹理，并在渲染前消费 `CameraLayer` 提供的相机结果。
- 解析固定示例资源路径，调用资源层加载 OBJ/MTL/外部图片，并把返回的纹理对象装配到示例场景。

**输入**

- 应用标题、窗口宽高。
- `src/base` 的数学工具。
- `src/render` 的相机、帧缓冲、mesh、Texture2D、program、pipeline。
- `src/resource` 的模型加载能力。
- `src/platform` 的初始化、事件处理、键盘查询和 present 能力。

**输出**

- 进程返回码。
- 对平台层发出事件处理与 present 请求。
- 对渲染层发出逐帧执行请求。

**边界条件**

- 可以依赖 `base`、`resource`、`render`、`platform`，但不应下沉到 Win32 API。
- 负责“调度”和“场景装配”，不负责具体像素写入算法。
- 不应把平台状态、窗口句柄或 GDI 细节泄漏到应用接口。
- 如果后续引入场景管理、相机层、资源系统，应优先继续放在调度层或更高层，而不是塞回入口层。

### 6.3 `src/platform`

**职责**

- 封装当前 Windows 平台的窗口生命周期、消息泵与最小键盘输入状态。
- 把 `render::Framebuffer` 的像素数据显示到本地窗口。
- 作为应用层唯一可见的平台门面。

**输入**

- 来自 `src/app` 的窗口初始化参数。
- 来自 `src/render` 的 `Framebuffer` 只读像素数据。
- 来自操作系统的 Win32 消息、键盘事件和 GDI 显示能力。

**输出**

- `Initialize / Show / ProcessEvents / Present / IsKeyDown / Close / Name` 等平台级接口。

**边界条件**

- `Platform` 是门面，`Window` 是具体平台窗口对象；应用层应优先依赖 `Platform`。
- 可以维护并暴露输入状态、可以读取 `Framebuffer`，但不能反向修改渲染算法或业务场景数据。
- 不负责三角形光栅化、矩阵运算、mesh 组织或 shader 逻辑。
- 当前实现是 Windows-only；如果后续支持多平台，应在 `platform` 目录内扩展，而不是让 `app` 层出现平台分支。

### 6.4 `src/render`

**职责**

- 持有 CPU 侧渲染核心数据结构与算法。
- 提供相机、颜色、顶点协议、mesh 容器、Texture2D、program、pipeline 与 framebuffer。
- 提供基础光照与最小 2D 纹理采样所需的顶点/统一参数/插值协议，并完成从顶点到像素的主要软光栅流程。

**输入**

- 顶点数据与索引数据。
- `uniforms` 和 shader 函数指针。
- `src/base` 提供的向量矩阵运算。

**输出**

- 写入 `Framebuffer` 的颜色与深度结果。

**边界条件**

- 可以依赖 `base`，不应依赖 `platform` 或 Win32。
- `Framebuffer` 是渲染结果容器，不负责窗口展示。
- `Pipeline` 负责主线执行，不负责应用生命周期和消息循环。
- `Camera / Vertex / Uniforms / Varyings / Texture2D / Program` 共同构成当前渲染层的重要共享协议；其中 `Camera` 负责纯数据和矩阵生成，新增 shader 能力时应尽量继续在这些协议内扩展，而不是把材质逻辑写进应用层。
- 当前实现以三角形列表和模板化 `Pipeline` 为核心；如果未来引入更多图元或阶段，也应尽量保持“渲染层负责渲染，不越界到平台显示”的原则。

### 6.5 `src/base`

**职责**

- 提供向量、矩阵、插值、裁剪辅助等基础数学能力。

**输入**

- 纯数学参数。

**输出**

- 无平台语义的数学结果。

**边界条件**

- 应保持无平台、无窗口、无业务场景依赖。
- 可以被 `app` 和 `render` 复用，但不应反向依赖上层模块。
- 新增基础数学工具时，应优先保持可复用、可验证，不混入渲染流程控制。

### 6.6 `src/resource`

**职责**

- 负责从磁盘读取最小 OBJ / MTL / base color 图片资源，并转换成渲染层可消费的 `render::Mesh<render::FlatColorVertex>`、`render::Mesh<render::LitVertex>` 与 `render::Texture2D`。
- 封装基础文件读取、OBJ 中 `mtllib / usemtl / v / vt / vn / f` 行解析、MTL 中 `newmtl / map_Kd` 解析和最小图片解码。

**输入**

- 来自 `src/app` 的资源路径。
- 来自磁盘的 OBJ / MTL 文本内容和 PNG/JPG/JPEG 图片内容。

**输出**

- 面向应用层返回的加载结果，以及面向渲染层的数据结构 `render::Mesh<...>` 与 `render::Texture2D`。

**边界条件**

- 可以依赖 `render` 的顶点/mesh 协议，但不负责窗口显示、主循环或像素写入。
- 当前已接入最小 OBJ loader、最小 MTL(base color) 解析和 PNG/JPG/JPEG 外部纹理解码，但仍未扩展到多材质子网格、更多材质通道和通用场景解析。
- 如果后续资源种类继续增多，应优先在 `resource` 目录内扩展，而不是把解析逻辑散落到 `app` 或 `render`。

### 6.7 `assets`

**职责**

- 存放模型、纹理、测试数据等静态资产。
- 当前已提供 `assets/models/cube.obj`、`assets/models/cube.mtl` 和 `assets/textures/cube-basecolor.png` 作为最小外部纹理加载链路的测试资源。

**边界条件**

- `assets` 本身只存数据，不承担解析逻辑。
- 资源格式解析仍应属于 `src/resource` 或未来的专门资源模块，不应放入平台层。

### 6.8 `docs`

**职责**

- 维护功能设计文档、架构文档与开发约束。

**边界条件**

- `docs/features/` 面向单次功能迭代。
- `docs/architecture/project-architecture.md` 面向项目级长期结构。
- 当代码改动影响模块职责、依赖方向、关键数据流、主要目录分层时，应优先更新架构文档，再交付功能。

## 7. 目录依赖规则

为避免后续结构失控，当前建议遵守以下依赖方向：

```text
apps/viewer -> src/app
src/app -> src/platform, src/resource, src/render, src/base
src/resource -> src/render
src/render -> src/base
src/platform -> src/render(仅 Framebuffer 显示接口), Win32/GDI
src/base -> 无上层依赖
```

不建议出现的依赖方向：

- `src/render -> src/platform`
- `src/resource -> src/platform`
- `src/base -> src/render` 或 `src/base -> src/app`
- `apps/viewer -> src/platform` 或 `apps/viewer -> src/render`
- 在 `src/app` 中直接写 Win32 API 调用

## 8. 当前架构的关键不变量

- 主控制流始终从 `main` 进入 `Application::Run()`。
- `Framebuffer` 是渲染结果的唯一主容器，窗口显示只消费它，不替代它。
- 平台显示和软光栅算法分层明确，平台层不实现渲染主线，渲染层不处理消息泵。
- 数学层保持纯工具属性，为应用装配和渲染执行同时服务。
- 当前渲染链以 `Camera + Program + Pipeline + Mesh + Texture2D + Framebuffer` 为基础协议。

## 9. 什么时候必须更新这份架构图

出现以下任一情况时，功能开发完成后应同步更新本文件：

- 新增或删除一个项目级模块目录，例如新增 `src/scene`、`src/resource`。
- 改变现有模块职责，例如把窗口显示从 `platform` 移到别处。
- 改变主要调用方向，例如 `Application` 不再直接持有和调度 `Pipeline`。
- 改变关键共享数据结构，例如 `Framebuffer` 语义发生明显变化。
- 引入新的跨层协议，例如资源系统、场景系统、调试 UI 系统。
- 改变平台抽象方式，例如从单一 `Window` 扩展为多后端平台实现。

如果只是以下情况，通常不需要更新本文件：

- 单个函数重命名
- 模块内部私有实现重构
- 不影响职责边界的局部性能优化
- 不改变调用方向的文件拆分

## 10. 开发前阅读建议

每次开始新功能前，建议按下面顺序阅读：

1. 本文档，先定位会影响哪些模块、哪些依赖方向、哪些共享数据。
2. `AGENTS.md` 中映射到的相关 lesson。
3. 本次功能已有的 `docs/features/<feature-name>.md`，如果它已经存在。

如果发现“代码现状”和“本文档”不一致，应优先指出差异，并在实现或 review 迭代中补齐更新。











