# 阶段三：Render 协议拆分与 Lit-only Shader 收口

## 功能背景

当前 `src/render/Vertex.h` 同时承担了顶点类型、uniform/varyings 协议、Program、面剔除状态、shader helper、Blinn-Phong 光照实现，以及纯色和调试面编号 shader。这已经明显超过了一个“顶点相关头文件”应承担的职责。

同时，当前版本已经明确只保留 OBJ 正常渲染链路，不再需要：

- 纯色绘制相关逻辑
- 调试面编号相关逻辑

因此阶段三的目标不是改渲染算法，而是把当前真正使用的 Lit 渲染链从大而全头文件里拆出来，并删除已经不再需要的渲染分支。

## 需求摘要

- 阶段三只做 render 协议与头文件职责重构
- 当前版本只保留 Lit 渲染链
- 删除 `FlatColor` 相关逻辑
- 删除 `DebugFace` 相关逻辑
- 删除调试面编号 helper
- 删除 OBJ loader 中 flat-color 路径
- 将 shader 相关内容统一迁移到 `src/render/shader/`
- `ShaderMath` 与 `ShaderTypes` 合并为一个文件
- 收缩 `Vertex.h`，不再让它承担大而全职责
- 保持当前 OBJ 渲染行为不变

本阶段不做：

- `Pipeline` 无状态化
- `CanRun()` / `ProgramAvailable()` 去重
- `FragmentStage` 数据流改造
- `Stage` 状态机
- loader 深层架构重构

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson10-engineering-features-parallel-pipeline-imgui-and-profiling.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图要求 `src/render` 继续保持“协议、算法、数据结构”边界清晰，不能再让一个头文件成为中心杂物堆。
- `lesson05` 约束 `vertex / uniforms / varyings / program` 应明确分层。
- `lesson06` 约束当前软光栅主链保持不变，本轮不应破坏 `RenderLayer -> Pipeline -> Stages -> Framebuffer` 的执行路径。
- `lesson10` 启发这一步优先做工程化收口，而不是行为级重构。

## 当前代码与目标结构差异

当前问题：

- `src/render/Vertex.h` 混杂了过多职责
- `FlatColor` 链路仍存在，但当前版本已经不再需要
- `DebugFace` 面编号链路仍存在，但当前版本已经不再需要
- OBJ loader 仍保留 flat-color 加载路径
- shader 相关实现没有集中到单独目录

目标结构：

- `src/render/VertexTypes.h`
  - `VertexBase`
  - `LitVertex`
- `src/render/Program.h`
  - `Program`
  - `FaceCullMode`
- `src/render/shader/ShaderTypes.h`
  - `UniformsBase`
  - `VaryingsBase`
  - `LitUniforms`
  - `LitVaryings`
  - `TransformPosition`
  - `TransformDirection`
- `src/render/shader/Lighting.h`
  - `ApplyBlinnPhongLighting`
- `src/render/shader/LitShader.h`
  - `LitVertexShader`
  - `LitFragmentShader`
- `src/render/Vertex.h`
  - 过渡聚合头，只做转发

## 技术方案与关键数据流

本轮保持运行时主链不变：

```text
SceneLayer 提供 OBJ 资源
-> RenderLayer 组装 Lit Program + Lit Uniforms
-> Pipeline::Run(framebuffer, mesh)
-> VertexStage 调 LitVertexShader
-> ClipStage / RasterStage / FragmentStage
-> LitFragmentShader 调用 Blinn-Phong 光照
-> Framebuffer
```

本轮变化点：

1. 把 Lit 所需协议和 shader 从 `Vertex.h` 中拆出
2. 删除纯色和调试面编号渲染分支
3. 删除 OBJ loader 中不再使用的 flat-color 加载 API
4. 调整 `Pipeline`、`PipelineStages`、`RenderLayer`、OBJ loader 对新头文件的依赖
5. 暂时保留 `Vertex.h` 作为兼容聚合头，降低 include 迁移风险

## 计划修改的文件

- `docs/features/stage3-render-protocol-lit-only-refactor.md`
- `src/render/Vertex.h`
- `src/render/VertexTypes.h`
- `src/render/Program.h`
- `src/render/shader/ShaderTypes.h`
- `src/render/shader/Lighting.h`
- `src/render/shader/LitShader.h`
- `src/render/Pipeline.h`
- `src/render/PipelineStages.h`
- `src/app/RenderLayer.cpp`
- `src/resource/loaders/ObjMeshLoader.h`
- `src/resource/loaders/ObjMeshLoader.cpp`
- `docs/architecture/project-architecture.md`

## 风险点与兼容性影响

- 模板 include 依赖收缩后，可能暴露隐藏编译依赖
- `ObjMeshLoader` 删除 flat-color 路径后会收缩公开接口
- `Vertex.h` 如果一次性彻底删除，会放大改动面，因此先保留聚合头更稳
- 本轮虽然不改 Pipeline 行为，但 `src/render` 内部文件边界会变化，需要检查架构文档是否需要更新

## 手动测试方案

1. 编译 / 运行
   - `cmake --build build-mingw --target sr_viewer`
   - `build-mingw/bin/sr_viewer.exe`
2. 启动验证
   - 默认 OBJ 场景正常显示
   - 控制台仍只输出 `FPS`
3. 渲染验证
   - 模型法线光照和纹理采样保持正常
   - 旋转相机后明暗与高光仍随视角变化
4. 输入验证
   - `W / A / S / D / Q / E / Space / Left Shift` 仍能正确控制相机
5. 结构验证
   - `FlatColor` 与 `DebugFace` 相关类型和 shader 已删除
   - OBJ loader 不再暴露 flat-color 加载接口
   - shader 相关头文件已迁到 `src/render/shader/`
6. 如结果异常，优先检查
   - `src/render/shader/LitShader.h`
   - `src/render/Pipeline.h`
   - `src/render/PipelineStages.h`
   - `src/app/RenderLayer.cpp`
   - `src/resource/loaders/ObjMeshLoader.cpp`

## 完成标准 / 验收标准

1. 已完成需求对齐
2. 已生成开发文档
3. render 协议只保留 Lit 渲染链
4. `FlatColor` 与 `DebugFace` 相关逻辑已删除
5. OBJ loader flat-color 路径已删除
6. shader 相关内容已迁到 `src/render/shader/`
7. `Vertex.h` 不再是职责塌陷点
8. OBJ 渲染行为保持正常
9. 已检查并在需要时更新架构文档
10. 已提供手动测试方式
11. 等待用户 review
