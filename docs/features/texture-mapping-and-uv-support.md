# 纹理贴图与 UV 支持

## 功能背景

当前仓库已经具备：

- 最小 OBJ 模型加载能力
- 可编程软光栅主线
- 基础法线与 Blinn-Phong 光照
- 基于 `faceUv` 的局部调试绘制示例

但正式渲染链路里仍缺少“模型自带 UV + 纹理采样”能力：

- `LoadLitMesh()` 目前只消费 `v + vn`
- `LitVertex` / `LitVaryings` 还没有正式 `uv`
- viewer 主场景仍以统一底色着色，而不是从纹理取样

因此本轮目标是在保持当前项目层次不变的前提下，把 OBJ 的 `vt`、纹理坐标插值和最小 2D 纹理采样接入主链路，并先用程序生成的测试纹理完成验证。

## 需求摘要

- 支持从 OBJ 中读取 `v / vt / vn`。
- 在正式主渲染链路中接入 UV：
  - 顶点携带 `uv`
  - 顶点 shader 传递 `uv`
  - 光栅化阶段对 `uv` 做透视正确插值
  - 片元 shader 基于 `uv` 采样 2D 纹理
- 先使用程序生成的测试纹理，不引入 `.mtl` 或外部图片解码库。
- viewer 运行后应能看到“纹理颜色 × 基础光照”的效果。
- 本轮不做：
  - `.mtl` 材质解析
  - 多材质 / 多纹理切换
  - 法线贴图 / 高光贴图
  - PBR
  - 通用场景文件纹理绑定系统

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson07-model-and-texture-resource-loading.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束 OBJ 解析仍应留在 `src/resource`，应用层不直接解析模型文本。
- 架构图约束纹理采样与 shader 协议应落在 `src/render`，应用层只负责装配纹理对象和 draw call 参数。
- `lesson05` 约束 `uv` 应作为顶点输入和 varyings 扩展字段进入渲染协议，纹理对象应通过 uniforms 传入。
- `lesson06` 约束纹理贴图必须沿着 `VertexShader -> 裁剪 -> 插值 -> FragmentShader` 主链路完成，不能绕过流水线直接写像素。
- `lesson07` 启发资源链路应表达为：
  `OBJ(v/vt/vn) -> Mesh<vertex_t> -> Program/Uniforms -> Texture Sample -> Framebuffer`。

## 技术方案与关键数据流

目标数据流：

```text
assets/models/cube.obj
-> resource::LoadLitMesh()
-> 解析 v / vt / vn 并组装 render::LitVertex(position, normal, uv)
-> Application 创建程序生成的 render::Texture2D
-> Lit uniforms 持有纹理指针
-> Lit vertex shader 传递 uv
-> Pipeline 光栅化时对 uv 做透视正确插值
-> Lit fragment shader 采样纹理并叠加基础光照
-> Framebuffer
-> Platform::Present()
```

实现要点：

- 在 `src/render` 内新增最小 `Texture2D` 数据结构与 `Sample()` 接口。
- 提供程序生成 UV 调试纹理的工厂函数，作为本轮固定测试纹理。
- 扩展 `LitVertex / LitVaryings / LitUniforms`：
  - `LitVertex` 新增 `uv`
  - `LitVaryings` 新增 `uv`
  - `LitUniforms` 新增纹理指针与“是否启用纹理”开关
- `LitFragmentShader` 优先采样纹理颜色，再走当前基础光照。
- `ObjMeshLoader` 扩展 `vt` 解析，并要求 lit 路径中的面至少具备 `position + uv + normal`。
- `Application` 继续加载固定 `cube.obj`，但主显示路径改为正式纹理贴图场景。
- 面编号调试链路保留，仅继续服务截图导出，不作为本轮主要显示路径。

## 计划修改的文件

- `docs/features/texture-mapping-and-uv-support.md`
- `docs/architecture/project-architecture.md`
- `assets/models/cube.obj`
- `src/render/Texture2D.h`
- `src/render/Vertex.h`
- `src/resource/loaders/ObjMeshLoader.cpp`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- 本轮为最小实现，只保证单纹理、单材质的教学链路，不覆盖通用 DCC 导出场景。
- OBJ lit 加载路径会从“必须有 `vn`”升级为“必须有 `vt + vn`”，旧的无 UV lit 模型将不再满足该路径。
- 新增 `Texture2D` 后，`render` 层的共享协议从“相机/mesh/program/framebuffer”为主扩展到“包含 2D 纹理对象”，因此本轮同步更新了架构文档。
- 程序生成纹理便于验证，但视觉效果以“确认 UV/采样正确”为主，不追求最终美术表现。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行 viewer
   - `build-mingw/bin/sr_viewer.exe`
4. 观察结果
   - 立方体表面应出现清晰的程序生成棋盘 / 分块纹理
   - 纹理应随模型旋转保持稳定，不应整面变成纯色
   - 不同三角形拼接处不应出现明显 UV 错乱
   - 光照仍应保留明暗层次，而不是完全覆盖纹理
5. 异常观察点
   - 若画面仍是纯色，优先检查 uniforms 是否绑定纹理及 fragment shader 是否实际采样
   - 若纹理拉伸或翻转，优先检查 OBJ `vt` 解析和 `uv` 方向约定
   - 若三角形接缝错位，优先检查索引展开时 position / uv / normal 是否按同一 face token 配对

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成开发文档。
3. OBJ lit 路径已支持 `vt`。
4. 正式主渲染链路已支持 UV 插值和 2D 纹理采样。
5. viewer 已能显示程序生成纹理的贴图效果。
6. 已检查并更新 `docs/architecture/project-architecture.md`。
7. 已提供清晰的手动测试方法。
8. 等待用户 review。
