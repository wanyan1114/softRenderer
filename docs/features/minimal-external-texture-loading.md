# 最小外部纹理加载：OBJ + MTL + PNG/JPG Base Color

## 功能背景

当前仓库已经具备：

- 最小 OBJ 几何加载能力
- 正式渲染链路中的 UV 插值与 `Texture2D` 采样能力
- viewer 主场景的基础光照与纹理贴图显示

但当前主显示路径仍依赖程序生成的调试纹理，尚未打通：

- OBJ 到 MTL 的材质引用
- MTL 到外部 base color 贴图的引用
- `.png/.jpg/.jpeg` 图片文件到 `render::Texture2D` 的解码

因此本轮目标是在保持现有项目层次不变的前提下，完成“单个 OBJ 文件及其关联 MTL/base color 贴图”的最小外部纹理加载链路。

## 需求摘要

- viewer 继续只加载一个固定 OBJ 文件。
- OBJ 允许通过 `mtllib` 指向一个 `.mtl` 文件。
- OBJ 允许通过 `usemtl` 选择材质。
- MTL 最小支持：
  - `newmtl`
  - `map_Kd`
- 外部图片最小支持：
  - `.png`
  - `.jpg`
  - `.jpeg`
- 图片路径先只支持相对路径。
- viewer 运行后应能显示 OBJ 关联的外部 base color 纹理，不再使用程序生成的默认纹理。

本轮不做：

- 多个 OBJ 一起加载
- 多材质子网格拆分与分别绘制
- 法线贴图、roughness、metallic、specular 等更多材质通道
- 通用场景文件系统
- 完整 MTL 语义兼容

额外范围约束：

- 单个 OBJ 文件允许出现 `usemtl`，但最小实现只支持整份模型最终落到同一个 base color 材质。
- 如果检测到多个不同材质都被实际使用，则显式报错，不做隐式降级。

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson07-model-and-texture-resource-loading.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束 OBJ/MTL/图片文件的解析与解码应留在 `src/resource`，应用层不直接处理文件格式细节。
- 架构图约束 `Texture2D` 仍属于 `src/render` 的共享渲染协议，资源层只负责构造与返回。
- `lesson07` 给出的主线正适合本轮目标：
  `磁盘资源 -> CPU 数据结构 -> shader 采样`。
- 因为本轮没有引入新的项目级模块目录，也不改变 `app -> resource -> render` 的依赖方向，所以更可能属于架构内扩展，而不是架构重组。

## 技术方案与关键数据流

目标数据流：

```text
assets/models/cube.obj
-> 解析 mtllib / usemtl / v / vt / vn / f
-> src/resource 读取 cube.mtl
-> 解析 newmtl / map_Kd
-> src/resource 解码 assets/textures/*.png|jpg|jpeg
-> 构造 render::Texture2D
-> Application 拿到 mesh + 可选 baseColorTexture
-> Lit fragment shader 按 uv 采样外部纹理
-> Framebuffer
-> Platform::Present()
```

实现要点：

- 在 `src/resource` 中扩展 OBJ 加载结果，使其除了 `mesh` 之外还能返回最小材质信息。
- 新增最小 MTL 解析逻辑：
  - 收集 `newmtl`
  - 为材质记录 `map_Kd`
  - 根据 OBJ 中实际使用的 `usemtl` 决定最终 base color 贴图
- 新增最小图片加载器：
  - 使用 Windows WIC 解码 PNG/JPG/JPEG
  - 输出统一的 RGBA 像素，再构造成 `render::Texture2D`
- `Application` 不再强制创建程序生成纹理，而是优先使用资源层返回的外部纹理；若资源缺失或解码失败，直接报错退出，不做静默回退。

## 计划修改的文件

- `docs/features/minimal-external-texture-loading.md`
- `assets/models/cube.obj`
- `assets/models/cube.mtl`
- `assets/textures/cube-basecolor.png`
- `src/CMakeLists.txt`
- `src/app/Application.cpp`
- `src/resource/loaders/ObjMeshLoader.h`
- `src/resource/loaders/ObjMeshLoader.cpp`
- `src/resource/loaders/ImageTextureLoader.h`
- `src/resource/loaders/ImageTextureLoader.cpp`

## 风险点与兼容性影响

- 最小实现只支持单材质主路径；多材质 OBJ 会被显式拒绝。
- WIC 是 Windows 平台能力，符合当前仓库的 Windows-only 前提，但后续若跨平台，需要重新抽象图片解码层。
- 若纹理路径中包含本轮未处理的特殊 MTL 语法（例如选项参数、空格转义、非相对路径），可能需要后续补齐。
- 本轮会让 `src/resource` 同时负责几何、材质引用和图片解码，但依赖方向仍保持在 `resource -> render`，不引入跨层反向依赖。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行 viewer
   - `build-mingw/bin/sr_viewer.exe`
4. 观察结果
   - viewer 应成功加载固定 OBJ
   - 模型表面应显示 `cube.mtl` 指向的外部 PNG/JPG 纹理
   - 不应再出现原来的程序生成 UV 调试纹理样式
5. 异常观察点
   - 若启动即失败，优先查看控制台中的 OBJ/MTL/图片路径错误
   - 若模型有光照但无纹理，优先检查 `mtllib`、`usemtl`、`map_Kd` 是否都被正确解析
   - 若纹理倒置或错位，优先检查图片解码后的行方向与 `Texture2D::Sample()` 的 `v` 方向约定

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成开发文档。
3. OBJ 最小链路已支持 `mtllib` / `usemtl`。
4. MTL 最小链路已支持 `newmtl` / `map_Kd`。
5. 已支持从 PNG/JPG/JPEG 解码到 `render::Texture2D`。
6. viewer 已显示外部 base color 纹理。
7. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
8. 已提供清晰的手动测试方法。
9. 等待用户 review。
