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
- `computer-graphics-notes/lesson08-from-blinn-phong-to-pbr.md`
- `computer-graphics-notes/lesson09-ibl-precomputation-and-toolchain.md`

## 3. 这些笔记对当前架构文档的约束与启发

- `lesson01` 约束我们先从 `main -> Application -> 主循环` 画起，架构图应优先反映总调度关系。
- `lesson02` 约束窗口、输入和消息泵应聚合为平台实现的一部分，而不是继续拆成过宽的应用门面。
- `lesson03` 启发我们把数学模块视为“基础支撑层”，它为变换和相机提供统一语言。
- `lesson04` 约束 `Framebuffer` 作为 CPU 渲染结果与窗口显示之间的桥接数据结构。
- `lesson05` 启发渲染模块内部要明确区分顶点输入、统一参数、插值数据和流水线执行器。
- `lesson06` 约束运行时主线要能表达“顶点 -> 三角形 -> 像素 -> 帧缓冲 -> 窗口”的数据流。
- `lesson08` 约束材质扩展仍应沿着 `ShaderTypes / Program / shader/*` 协议演进，而不是把光照逻辑写进应用层。
- `lesson09` 约束 IBL 不是单个片元公式升级，而是“环境源 -> irradiance / prefilter / BRDF LUT -> 运行时消费”的完整资源闭环。

## 4. 当前项目架构总览

```mermaid
flowchart LR
    viewer["apps/viewer/main.cpp"]
    app["src/app<br/>Application / layer/Layer / layer/LayerContext / layer/LayerStack / layer/SceneLayer / layer/CameraLayer / layer/RenderLayer"]
    platform["src/platform\nWindow / Input"]
    resource["src/resource\nOBJ Loader / Image Loader"]
    base["src/base\nMath / StateMachine"]
    render["src/render\nCamera / Color / VertexTypes / Texture2D / EnvironmentMap / Program / shader/* / Mesh / Pipeline / PipelineStages / Framebuffer / SkyboxRenderer / IblPrecompute"]
    win32["Win32 + GDI"]
    docs["docs/architecture/project-architecture.md"]

    viewer --> app
    app --> platform
    app --> base
    app --> resource
    app --> render
    resource --> render
    platform --> win32
    render --> base
    platform -.读取像素并显示.-> render
    docs -.开发前阅读/开发后必要时更新.-> app
    docs -.约束模块边界.-> platform
    docs -.约束模块边界.-> resource
    docs -.约束模块边界.-> render
```

可以把当前仓库理解成 6 个运行时层次：

1. `apps/viewer`
   程序入口层，只负责构造应用并交出控制权。
2. `src/app`
   应用调度层，负责生命周期、主循环和 layer 编排。
3. `src/platform`
   平台实现层，负责窗口创建、消息泵、输入状态维护和最终显示。
4. `src/resource`
   资源接入层，负责把磁盘资源解析成运行时可消费的数据结构。
5. `src/render`
   软光栅核心层，负责 CPU 侧三角形处理、阶段状态机推进、skybox/IBL 预计算与帧缓冲写入。
6. `src/base`
   基础支撑层，负责向量、矩阵、变换工具和通用状态机基础设施。

## 5. 当前运行时主链路

### 5.1 启动链路

```mermaid
sequenceDiagram
    participant Main as main.cpp
    participant App as Application
    participant FB as render::Framebuffer(local)
    participant Startup as StartupState(local)
    participant Ctx as LayerContext(local)
    participant Layers as LayerStack
    participant SceneLayer as app::SceneLayer
    participant CameraLayer as app::CameraLayer
    participant RenderLayer as app::RenderLayer
    participant Window as platform::Window

    Main->>App: 构造 Application(title, width, height)
    Main->>App: Run()
    App->>FB: 创建局部 framebuffer
    App->>Startup: 创建局部 startupState
    App->>Ctx: 创建局部 context
    App->>Layers: OnAttach(context)
    Layers->>SceneLayer: OnAttach(context)
    SceneLayer->>resource: LoadLitMesh(OBJ/MTL/base color)
    SceneLayer->>render::ibl: 生成环境预设 / irradiance / prefilter / BRDF LUT
    SceneLayer->>App: 通过 context 回填 sceneView / startupState
    Layers->>CameraLayer: OnAttach(context)
    Layers->>RenderLayer: OnAttach(context)
    App->>Window: Create()
    App->>Window: Show()
```

### 5.2 逐帧链路

```text
while (Window::ProcessEvents())
-> Application 从 Window 读取输入状态
-> Application 写 context.input
-> Application 驱动 LayerStack::OnUpdate(deltaTime, context)
-> SceneLayer 读取 context.input
-> SceneLayer 根据 R 键切换环境预设并更新 sceneView.ibl
-> CameraLayer 读取 context.input
-> CameraLayer 更新 render::Camera 的 Pos / Dir / Right
-> CameraLayer 写 context.activeCamera
-> RenderLayer 读取 context.input
-> RenderLayer 根据 F/G/T 切换材质模式、skybox 模式和 prefilter 预览 lod
-> Application 驱动 LayerStack::OnRender(context)
-> RenderLayer 读取 context.activeCamera / context.sceneView / context.framebuffer
-> RenderLayer 清颜色
-> SkyboxRenderer 按当前 skybox 模式绘制背景
-> RenderLayer 清深度
-> Pipeline::Run(framebuffer, mesh, program, uniforms)
-> PipelineStageMachine
-> VertexStage
-> ClipStage
-> RasterStage
-> FragmentStage
-> sample 级覆盖判断、深度测试与颜色写入
-> Framebuffer::ResolveColor() 聚合 4x sample 颜色
-> Window::Present(framebuffer)
-> StretchDIBits 显示到 Win32 窗口
```

### 5.3 关键数据流

```text
Win32 keyboard messages
-> Window::OnMessage()
-> Window 内部 InputState
-> Application 写 context.input
-> LayerStack::OnUpdate(deltaTime, context)
-> SceneLayer::OnUpdate(deltaTime, context)
-> R 键切换 active IBL preset
-> RenderSceneIblResources(skybox / irradiance / prefilter / brdfLut / presetName)
-> context.sceneView
-> CameraLayer::OnUpdate(deltaTime, context)
-> render::Camera(Pos / Dir / Right / Up / Aspect)
-> context.activeCamera
-> RenderLayer::OnUpdate(deltaTime, context)
-> F 键切换 Blinn / 直接光 PBR / IBL-PBR
-> G 键切换 Skybox / Irradiance / Prefilter 背景视图
-> T 键切换 Prefilter 预览 lod
-> LayerStack::OnRender(context)
-> RenderLayer 读取 Camera::ViewMat4() / ProjectionMat4()

OBJ(v + vt + vn) + mtllib/usemtl
-> resource::LoadLitMesh()
-> 解析 MTL(newmtl + map_Kd)
-> 解码 PNG/JPG/JPEG 为 Texture2D
-> SceneLayer 持有 OBJ 资源、程序生成的 albedo / metallic / roughness / ao 贴图、model matrix 和场景级 IBL 资源
-> context.sceneView

程序环境预设
-> render::ibl::GenerateEnvironmentPreset()
-> render::ibl::ConvoluteDiffuseIrradiance()
-> render::ibl::PrefilterEnvironment()
-> render::ibl::IntegrateBrdfLut()
-> SceneLayer 持有多套 IBL 预设
-> RenderLayer / IblPbrShader / SkyboxRenderer 运行时消费

RenderLayer::OnRender()
-> SkyboxRenderer 按方向采样 EnvironmentMap / PrefilterEnvironmentMap
-> PipelineStageMachine 驱动固定阶段切换
-> Pipeline::Run()
-> vertexShader 输出 clipPos + worldPos + worldNormal + uv
-> ClipStage 生成 clippedTriangles
-> RasterStage 生成当前片元批次
-> FragmentStage 采样 Texture2D / EnvironmentMap / BRDF LUT
-> 计算 Blinn-Phong 或直接光 PBR 或 IBL-PBR 光照
-> Framebuffer(samplePixels + sampleDepth + pixels + depth)
-> ResolveColor() 将 sample 颜色聚合为最终像素颜色
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
- 不直接操作 `platform::Window`、`render::*`。
- 入口如果需要新增模式切换、命令行解析，也应以“配置应用”而不是“替代应用主循环”为原则。

### 6.2 `src/app`

**职责**

- 作为运行时总调度器管理生命周期。
- `src/app/layer/` 负责承载 layer 抽象、layer 容器与具体 layer 实现。
- 在 `Run()` 内创建局部 `LayerContext / Framebuffer / StartupState`。
- 直接持有 `Window` 并维护主循环、present 与退出流程。
- 负责调度各 layer，但不拥有 scene/model/render 运行状态本体。

**输入**

- 应用标题、窗口宽高。
- `src/platform` 的窗口、输入和显示能力。
- `src/app` 内部 layer 提供的逐帧更新与渲染能力。
- `src/render` 的 `Framebuffer`、相机数据和渲染协议。

**输出**

- 进程返回码。
- 对 `Window` 发出创建、事件处理与 present 请求。
- 对 `LayerStack` 发出逐帧更新与渲染分发请求。
- 向局部 `LayerContext` 提供输入、`Framebuffer` 和启动期共享状态。

**边界条件**

- 可以依赖 `platform`、`base`、`resource`、`render`，但不应下沉到 Win32 API。
- `Application` 负责“调度”，不直接持有具体 `SceneLayer`、`CameraLayer` 或 `RenderLayer` 的实现细节。
- 相机控制应放在 `CameraLayer::OnUpdate()`，场景装配和环境切换应放在 `SceneLayer::OnAttach()/OnUpdate()`，渲染提交应放在 `RenderLayer::OnRender()`。
- 不应把 GDI、窗口句柄或资源所有权泄漏到应用接口。

### 6.3 `src/platform`

**职责**

- 提供窗口创建、消息泵、输入状态维护和最终显示的具体平台实现。
- 通过 `Window` 封装当前 Windows 平台细节。
- 把 `render::Framebuffer` 的像素数据显示到本地窗口。

**输入**

- 来自 `src/app` 的窗口参数。
- 来自 `src/render` 的 `Framebuffer` 只读像素数据。
- 来自操作系统的 Win32 消息、键盘事件和 GDI 显示能力。

**输出**

- `Window::Create / Show / ProcessEvents / Present / Input` 等实例级能力。

**边界条件**

- 平台层当前不再保留额外 `Platform` 门面，`Window` 直接作为应用层可见的窗口抽象。
- 可以维护并暴露输入状态、可以读取 `Framebuffer`，但不能反向修改渲染算法或场景数据。
- 不负责三角形光栅化、矩阵运算、mesh 组织或 shader 逻辑。
- 当前实现是 Windows-only；如果后续支持多平台，应在 `platform` 目录内扩展新的窗口实现，而不是把平台分支散到 `app` 层。

### 6.4 `src/render`

**职责**

- 持有 CPU 侧渲染核心数据结构与算法。
- 提供相机、颜色、顶点类型、shader 协议、Program、mesh 容器、Texture2D、EnvironmentMap、PrefilterEnvironmentMap、无状态 Pipeline 执行器、阶段状态机与 framebuffer。
- 当前 shader 相关实现集中在 `src/render/shader/`，并保留 `Lit/Blinn-Phong`、`直接光 PBR` 与 `IBL-PBR` 三条材质路径。
- 负责最小 skybox 渲染与最小 IBL 预计算工具链，包括 `GenerateEnvironmentPreset / ConvoluteDiffuseIrradiance / PrefilterEnvironment / IntegrateBrdfLut`。
- `SceneLayer -> RenderScenePart / RenderSceneIblResources -> RenderLayer` 构成当前跨层场景协议。

**输入**

- 顶点数据与索引数据。
- `uniforms` 和 shader 函数指针。
- `src/base` 提供的向量矩阵运算。
- 来自 `src/app` 装配的对象材质纹理和场景级 IBL 资源。

**输出**

- 写入 `Framebuffer` 的颜色与深度结果。
- 面向应用层暴露的环境图采样、skybox 绘制和 IBL 预计算能力。

**边界条件**

- 可以依赖 `base`，不应依赖 `platform` 或 Win32。
- `Framebuffer` 是渲染结果容器，不负责窗口展示。
- `Pipeline` 当前是无状态执行器，不负责应用生命周期和消息循环。
- `Camera / VertexTypes / ShaderTypes / Texture2D / EnvironmentMap / Program` 共同构成当前渲染层的重要共享协议。
- shader 相关实现应优先继续收敛在 `src/render/shader/` 内，而不是把材质逻辑写进应用层。
- `PipelineStages` 以固定阶段状态机组织 `Vertex -> Clip -> Raster -> Fragment`，阶段切换参数在 render 内部按固定数据结构传递。
- IBL 预计算目前是最小 CPU 实现，属于渲染层能力的一部分，但不直接承担磁盘资源管理或 UI 配置职责。
- 当前实现仍以三角形列表和模板化 `Pipeline` 为核心；如果未来引入更多图元或阶段，也应尽量保持“渲染层负责渲染，不越界到平台显示”的原则。

### 6.5 `src/base`

**职责**

- 提供向量、矩阵、插值、裁剪辅助等基础数学能力，以及可供多模块复用的通用状态机基础设施。

**输入**

- 纯数学参数或通用状态机所有者/状态对象。

**输出**

- 无平台语义的数学结果或通用状态切换结果。

**边界条件**

- 应保持无平台、无窗口、无业务场景依赖。
- 可以被 `app` 和 `render` 复用，但不应反向依赖上层模块。
- `StateMachine` 这类基础设施如果继续扩展，也应保持通用性，不绑定具体 render 语义。
- 新增基础数学工具时，应优先保持可复用、可验证，不混入渲染流程控制。

### 6.6 `src/resource`

**职责**

- 负责从磁盘读取最小 OBJ / MTL / base color 图片资源，并转换成渲染层可消费的 `render::Mesh<render::LitVertex>` 与 `render::Texture2D`。
- 封装基础文件读取、OBJ 中 `mtllib / usemtl / v / vt / vn / f` 行解析、MTL 中 `newmtl / map_Kd` 解析和最小图片解码。

**输入**

- 来自 `src/app` 或 `SceneLayer` 的资源路径。
- 来自磁盘的 OBJ / MTL 文本内容和 PNG/JPG/JPEG 图片内容。

**输出**

- 面向应用层返回的加载结果，以及面向渲染层的数据结构 `render::Mesh<...>` 与 `render::Texture2D`。

**边界条件**

- 可以依赖 `render` 的顶点/mesh 协议，但不负责窗口显示、主循环或像素写入。
- 当前只接入最小 OBJ loader、最小 MTL(base color) 解析和 PNG/JPG/JPEG 外部纹理解码。
- 当前 IBL 资源仍由 `SceneLayer + render::ibl` 在运行时程序生成，尚未形成外部 `.hdr` / 预计算缓存的通用加载链。
- 如果后续资源种类继续增多，应优先在 `resource` 目录内扩展，而不是把解析逻辑散落到 `app` 或 `render`。

### 6.7 `assets`

**职责**

- 存放模型、纹理、测试数据等静态资产。
- 当前已提供 `assets/models/cube.obj`、`assets/models/cube.mtl` 和 `assets/textures/cube-basecolor.png` 作为最小外部纹理加载链路的测试资源。
- `assets/hdr/` 当前仍是为后续真实环境资源预留的位置，当前 IBL 演示主要依赖程序生成环境预设。

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
- 当前渲染链以 `Camera + VertexTypes + ShaderTypes + Program + Pipeline + PipelineStages + Mesh + Texture2D + EnvironmentMap + Framebuffer` 为基础协议。
- 当前 viewer 运行时保留 OBJ 模型加载路径，并在渲染层提供 `Blinn-Phong / 直接光 PBR / IBL-PBR` 三条材质路径。
- 当前 IBL 采用“程序环境预设 + 启动期 CPU 预计算 + 运行时切换”的最小闭环，而不是完整外部 HDR 资源工作流。

## 9. 什么时候必须更新这份架构图

出现以下任一情况时，功能开发完成后应同步更新本文件：

- 新增或删除一个项目级模块目录，例如新增 `src/scene`、`src/resource`。
- 改变现有模块职责，例如把窗口显示从 `platform` 移到别处。
- 改变主要调用方向，例如 `Application` 不再直接持有和调度 `Pipeline`。
- 改变关键共享数据结构，例如 `Framebuffer` 语义发生明显变化。
- 引入新的跨层协议，例如资源系统、场景系统、调试 UI 系统。
- 改变平台抽象方式，例如从单一 `Window` 扩展为多后端平台实现。

如果只是以下情况，通常不需要更新本文件：

- 单个函数重命名。
- 模块内部私有实现重构。
- 不影响职责边界的局部性能优化。
- 不改变调用方向的文件拆分。

## 10. 开发前阅读建议

每次开始新功能前，建议按下面顺序阅读：

1. 本文档，先定位会影响哪些模块、哪些依赖方向、哪些共享数据。
2. `AGENTS.md` 中映射到的相关 lesson。
3. 本次功能已有的 `docs/features/<feature-name>.md`，如果它已经存在。

如果发现“代码现状”和“本文档”不一致，应优先指出差异，并在实现或 review 迭代中补齐更新。
