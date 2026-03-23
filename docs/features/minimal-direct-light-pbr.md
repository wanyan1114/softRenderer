# 最小直接光 PBR

## 功能背景

当前 viewer 已具备：

- OBJ + MTL + `map_Kd` 的最小资源加载路径
- 基于 `vertex / uniforms / varyings / Program / Pipeline` 的可编程 CPU 软光栅主链
- 一条单光源、带 base color 纹理采样的 `Lit` / Blinn-Phong 材质路径

但当前材质表达仍停留在经验型高光模型，无法通过统一的 `metallic / roughness / ao` 参数稳定表达金属与非金属、粗糙与光滑材质差异。根据 `lesson08-from-blinn-phong-to-pbr.md`，本轮最自然的升级路径是先实现“直接光 PBR”，即在保留现有渲染主链的前提下，将片元阶段升级为 Cook-Torrance BRDF。

## 需求摘要

- 在当前 viewer 中新增一条“最小直接光 PBR”渲染路径。
- 保留现有 Blinn-Phong 路径，便于对照。
- PBR 路径至少支持：
  - `albedo`
  - `metallic`
  - `roughness`
  - `ao`
- `albedo / metallic / roughness / ao` 贴图由本次在运行时生成并接入。
- 本轮重点是看到明显的 PBR 视觉差异，不做 IBL。
- 本轮 review 迭代额外要求默认场景下的 PBR 结果不能整体过暗，并且要更容易观察到 metallic / roughness 的差异。

### 不做什么

- 不做 `IBL-PBR`
- 不接入 HDR、irradiance map、prefilter map、BRDF LUT
- 不引入法线贴图、金属度贴图自动从 MTL 解析的完整资源规范
- 不重写软件光栅主流程
- 不移除现有 Blinn-Phong 路径

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

### 架构图对当前功能的约束/启发

- 材质模型升级应主要落在 `src/render` 的共享协议和 shader 实现中。
- `src/app` 只负责根据场景与输入装配 draw call，不承载 BRDF 计算。
- 若新增共享材质数据或切换模式，应通过 `LayerContext / RenderSceneView / RenderScenePart` 等跨层协议传递，而不是让 `render` 反向依赖 `app`。

## 对应阅读的 computer-graphics-notes 模块

- `computer-graphics-notes/lesson08-from-blinn-phong-to-pbr.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

### 笔记对当前功能的约束/启发

- `lesson08` 约束本轮优先选择“直接光 PBR”，核心是将片元 BRDF 从 Blinn-Phong 升级为 Cook-Torrance，并引入 `albedo / metallic / roughness / ao`。
- `lesson05` 约束 PBR 应以新的 uniforms / varyings / shader 实现接入，而不是绕开当前 `Program` 协议。
- `lesson06` 约束本轮不重写 `Vertex -> Clip -> Raster -> Fragment` 主链，只替换材质路径与部分装配逻辑。

## 技术方案与关键数据流

## 方案概述

本轮采用“双材质路径并存”的最小升级方案：

1. 保留现有 `Lit` / Blinn-Phong shader 及其 uniforms。
2. 新增一套 `PbrUniforms / PbrVaryings / PbrShader`。
3. 在 `RenderScenePart` 中补充最小 PBR 资源引用：
   - albedo 纹理
   - metallic 纹理
   - roughness 纹理
   - ao 纹理
4. 在 `RenderLayer` 中根据当前模式选择走 Blinn 或 PBR draw path。
5. 提供一个轻量切换入口，允许在运行时切换 Blinn 与 PBR 做对照。
6. 在 `SceneLayer` 中为默认场景运行时生成一组简单但能体现材质差异的贴图资源。

## 材质表达

PBR 路径使用 metallic-roughness 工作流：

- `albedo`：基础反照率，继续承担材质主色
- `metallic`：控制介质/金属过渡与 `F0`
- `roughness`：控制 GGX 分布与高光扩散程度
- `ao`：调制环境项

本轮默认仍使用直接光，不引入 IBL；但为了避免演示画面过暗，允许对直射光强度、半球环境项和最小镜面环境近似做教学向调优。

## 渲染模式切换

为了满足“保留现有路径做对照”，预计提供轻量模式切换：

- 默认进入 PBR
- 在运行时通过一个按键切换 `Blinn-Phong / PBR`
- 启动日志中输出当前模式和切换提示

## 关键数据流

```text
SceneLayer
-> 加载 OBJ
-> 运行时生成/绑定 albedo + metallic + roughness + ao 纹理
-> 组装 RenderScenePart
-> context.sceneView

RenderLayer::OnRender()
-> 读取 activeCamera / sceneView / framebuffer / input
-> 根据当前渲染模式选择 Blinn 或 PBR Program
-> 组装对应 uniforms
-> Pipeline::Run()
-> VertexShader 产出 worldPos / worldNormal / uv
-> FragmentShader
-> Blinn-Phong 或 Cook-Torrance BRDF
-> Framebuffer
```

## 计划修改的文件

- `docs/features/minimal-direct-light-pbr.md`
- `src/app/layer/LayerContext.h`
- `src/app/layer/SceneLayer.h`
- `src/app/layer/SceneLayer.cpp`
- `src/app/layer/RenderLayer.h`
- `src/app/layer/RenderLayer.cpp`
- `src/platform/Input.h`
- `src/platform/Window.cpp`
- `src/render/shader/ShaderTypes.h`
- `src/render/shader/PbrShader.h`
- `src/render/shader/Lighting.h`

## 风险点与兼容性影响

- 当前 `RenderScenePart` 仅面向 Lit 路径，扩展为同时承载 PBR 资源后，属于跨层共享协议变更，需要确保 `SceneLayer -> RenderLayer` 装配仍清晰。
- 当前 `Input` 只有相机控制键，新增模式切换键时需避免影响现有相机操作。
- 当前 `varyings` 使用按 float 连续内存插值的实现，新 PBR varyings 仍需保持纯 float/向量字段布局。
- 当前 `ObjMeshLoader` 只负责 OBJ/MTL 最小加载；本轮选择“运行时生成贴图”，该能力应明确属于示例场景装配，而不是资源规范升级。
- 若最终新增了共享材质协议或模式切换数据流，需要在交付前检查是否应更新架构文档。

## 手动测试方案

1. 编译并运行 viewer。
2. 启动默认模型场景。
3. 观察启动日志是否输出当前渲染模式与切换提示。
4. 按 `F` 在 Blinn-Phong 与 PBR 模式之间切换，观察：
   - 默认 PBR 模式下是否仍然整体过暗
   - 高光是否从经验型高光变为随 roughness 变化的高光分布
   - metallic 区域是否更偏镜面反射、漫反射减弱
   - ao 区域是否整体更暗
5. 使用 WASD、Q/E、Space、Left Shift 移动相机，从不同视角观察材质响应是否稳定。

## 完成标准 / 验收标准

- viewer 中可运行 Blinn-Phong 与最小直接光 PBR 两条路径。
- PBR 路径使用 `albedo + metallic + roughness + ao` 纹理。
- 默认场景贴图由本次在运行时生成并绑定到场景。
- 在同一模型上可明显观察到 Blinn 与 PBR 的视觉差异。
- 不引入 IBL 相关资源和流程。
- 交付前显式检查是否需要更新 `docs/architecture/project-architecture.md`。
