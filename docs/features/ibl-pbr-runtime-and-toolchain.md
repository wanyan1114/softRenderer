# IBL-PBR 运行时与预计算工具链

## 功能背景

当前仓库已经具备：

- `Blinn-Phong` 与 `直接光 PBR` 两条材质路径
- 基于 `vertex / uniforms / varyings / Program / Pipeline` 的 CPU 软光栅主链
- 默认 OBJ 模型浏览与程序生成的 PBR 基础贴图

但当前环境光仍停留在直接光 PBR 的近似环境项，尚未具备：

- `IrradianceMap`
- `PrefilterMap`
- `BrdfLUT`
- skybox 环境显示
- IBL 资源切换
- IBL 预计算工具链

根据 `lesson09-ibl-precomputation-and-toolchain.md`，本轮应补齐“预计算资源生产端 + 运行时消费端”的完整闭环。

## 需求摘要

- 在现有 `Blinn-Phong / 直接光 PBR` 基础上新增 `IBL-PBR` 路径。
- 支持 skybox 显示。
- 支持 IBL 资源切换。
- 实现最小预计算工具链，生成：
  - `irradiance`
  - `prefilter`
  - `brdfLut`
- 本轮先用仓库内程序生成或占位资源跑通，不要求先接真实外部 HDR 文件。
- 保留现有旧路径用于对照。

### 不做什么

- 本轮不引入 ImGui。
- 本轮不追求高质量或高性能离线积分。
- 本轮不强制支持外部 `.hdr` 文件加载。
- 本轮不做完整资源编辑器或通用场景系统。

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

### 架构图对当前功能的约束/启发

- 预计算工具链和运行时 IBL 资源应优先落在 `src/render` / `src/app/layer` 的职责边界内。
- `src/app` 负责组织模式切换和场景装配，`src/render` 负责 skybox、环境采样与 IBL 光照。
- 如果新增环境图、prefilter 多级贴图、LUT 或跨层场景协议，交付前必须检查是否需要更新架构文档。

## 对应阅读的 computer-graphics-notes 模块

- `computer-graphics-notes/lesson09-ibl-precomputation-and-toolchain.md`
- `computer-graphics-notes/lesson08-from-blinn-phong-to-pbr.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`

### 笔记对当前功能的约束/启发

- `lesson09` 约束本轮必须同时实现“生产端”和“消费端”，不能只补 `IBLPBRShader`。
- `lesson08` 约束 IBL-PBR 应建立在现有 `albedo / metallic / roughness / ao` 工作流之上，并把环境项替换为 `irradiance + prefilter + brdfLut`。
- `lesson05` 约束 IBL-PBR 仍应通过新的 uniforms / varyings / shader 协议接入，而不是绕开当前 `Program` / `Pipeline`。

## 技术方案与关键数据流

## 方案概述

本轮采用“程序生成环境 + 启动期预计算 + 运行时切换”的最小闭环方案：

1. 新增环境图纹理抽象 `EnvironmentMap`，用方向采样代替普通 UV 采样。
2. 新增多级 prefilter 环境图抽象 `PrefilterEnvironmentMap`，用于 roughness 对应的 lod 采样。
3. 新增 IBL 预计算函数：
   - 生成原始环境图
   - diffuse irradiance 卷积
   - specular prefilter 预积分
   - BRDF LUT 数值积分
4. 在 `SceneLayer` 启动时预计算多套程序生成环境预设。
5. 在 `RenderLayer` 中新增：
   - `IBL-PBR` 模式
   - skybox 显示模式
   - prefilter 预览 lod 切换
6. 通过键位切换：
   - `F`：`Blinn-Phong / 直接光 PBR / IBL-PBR`
   - `G`：`Skybox / Irradiance / Prefilter`
   - `T`：prefilter 预览 lod
   - `R`：环境预设

## 环境资源表示

本轮新增两类运行时资源：

- `EnvironmentMap`
  - 本质是 equirectangular 2D 纹理
  - 提供 `Sample(direction)` 能力
- `PrefilterEnvironmentMap`
  - 持有多个 roughness lod 的 `EnvironmentMap`
  - 提供 `Sample(direction, lod)` 能力

`BrdfLUT` 继续使用普通 `Texture2D`，按 `(NoV, roughness)` 采样。

## 预计算工具链

本轮不引入独立 UI 工具层，而是在仓库内以最小工具链函数形式实现：

- `GenerateEnvironmentPreset(...)`
- `ConvoluteDiffuseIrradiance(...)`
- `PrefilterEnvironment(...)`
- `IntegrateBrdfLut(...)`

这些函数以 CPU 计算方式在启动时生成可用结果，优先保证链路正确和可视化成立。

## 运行时链路

```text
环境预设
-> 生成原始 skybox EnvironmentMap
-> 预计算 irradiance / prefilter / brdfLut
-> SceneLayer 组装 RenderScenePart + RenderSceneIblResources
-> context.sceneView

RenderLayer::OnUpdate()
-> F 切换材质模式
-> G 切换 skybox 可视化模式
-> T 切换 prefilter 预览 lod

SceneLayer::OnUpdate()
-> R 切换环境预设

RenderLayer::OnRender()
-> 先绘制 skybox 背景
-> 再根据当前模式绘制 Blinn / 直接光 PBR / IBL-PBR
-> IBL-PBR 读取 irradiance / prefilter / brdfLut
```

## 计划修改的文件

- `docs/features/ibl-pbr-runtime-and-toolchain.md`
- `src/app/layer/LayerContext.h`
- `src/app/layer/SceneLayer.h`
- `src/app/layer/SceneLayer.cpp`
- `src/app/layer/RenderLayer.h`
- `src/app/layer/RenderLayer.cpp`
- `src/platform/Input.h`
- `src/platform/Window.cpp`
- `src/render/Texture2D.h`
- `src/render/EnvironmentMap.h`
- `src/render/IblPrecompute.h`
- `src/render/SkyboxRenderer.h`
- `src/render/shader/ShaderTypes.h`
- `src/render/shader/Lighting.h`
- `src/render/shader/PbrShader.h`
- `src/render/shader/IblPbrShader.h`

## 风险点与兼容性影响

- 当前渲染层原本只有普通 `Texture2D`，新增方向采样环境图与多级 prefilter 资源后，属于共享协议扩展。
- 当前 `RenderSceneView / RenderScenePart` 只承载对象材质资源；本轮已新增场景级 IBL 资源与 skybox 显示依赖。
- 预计算在启动期同步执行，会带来额外启动成本。
- 无 ImGui 情况下，键位切换依赖控制台提示。
- 本轮新增环境资源链和 skybox 渲染链，交付前需要更新架构文档。

## 手动测试方案

1. 编译并运行 viewer。
2. 启动默认 OBJ 场景。
3. 观察控制台是否输出：
   - 当前环境预设
   - 材质模式切换提示
   - skybox 模式切换提示
   - prefilter lod 切换提示
4. 按 `F` 切换 `Blinn-Phong / 直接光 PBR / IBL-PBR`，观察材质差异。
5. 按 `G` 切换 skybox 显示内容，确认可看到：
   - 原始环境
   - irradiance
   - prefilter
6. 按 `T` 切换 prefilter 预览 lod，确认 prefilter 背景模糊程度变化。
7. 按 `R` 切换不同环境预设，确认：
   - skybox 跟着变化
   - IBL-PBR 材质外观跟着变化

## 完成标准 / 验收标准

- viewer 中可切换 `Blinn-Phong / 直接光 PBR / IBL-PBR`。
- viewer 中可显示 skybox，并能切换 `skybox / irradiance / prefilter` 可视化。
- 环境资源可切换，并能驱动 IBL-PBR 外观变化。
- 仓库内存在可运行的最小预计算工具链，能生成 `irradiance / prefilter / brdfLut`。
- IBL-PBR 运行时正确消费三类预计算资源。
- 交付前显式检查并更新 `docs/architecture/project-architecture.md`。
