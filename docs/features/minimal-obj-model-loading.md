# 最小 OBJ 模型加载与 Viewer 网格显示

## 功能背景

当前仓库已经具备最小可运行的软光栅主线：

- 应用层负责窗口、主循环与场景装配
- 渲染层负责顶点着色、裁剪、背面剔除、光栅化、深度测试与帧缓冲写入
- viewer 已能从磁盘读取最小 OBJ 并显示固定模型

在上一轮接入最小 OBJ loader 后，立方体已经能从 `assets/models/cube.obj` 正常显示。但仅靠单色渲染仍然不方便检查：

- 每个面是否被正确识别
- 面的朝向和可见性是否符合预期
- 观察到的面与数据里的“第几面”如何对应

本轮目标是在保持最小 OBJ 加载链路不变的前提下，为测试立方体增加“分面可视化标记”：

```text
同一个模型 -> 按面拆分为 6 组调试绘制 -> 每面不同颜色 -> 面上直接显示 1~6 号数字
```

## 需求摘要

- 保留当前最小 OBJ 模型加载能力。
- viewer 继续加载固定测试模型 `assets/models/cube.obj`。
- 对立方体的 6 个面进行单独可视化：
  - 每个面使用不同底色
  - 每个面在表面直接显示一个数字编号，便于观察和核对
- 数字显示应直接出现在面上，而不是只打印到控制台。
- 本轮编号以 1~6 为目标，不引入通用文本渲染系统。
- 本轮不做：
  - 通用字体系统
  - 纹理贴图数字
  - UI 面板编号列表
  - 非立方体模型的通用“自动编号所有面”能力

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson07-model-and-texture-resource-loading.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束资源加载逻辑仍应留在 `src/resource`，这次 review 迭代不应把 OBJ 解析回塞进 `src/app`。
- 架构图约束应用层负责“场景装配”和“调试示例组织”，因此“按面拆分立方体并设置颜色/编号”可以放在 `src/app`。
- `lesson05` 提醒这次最自然的实现方式不是引入独立文本系统，而是扩展当前 `vertex / uniforms / varyings / fragment shader` 协议来表达面内局部坐标与编号。
- `lesson06` 约束数字叠加应继续走现有主线：
  `VertexShader -> Rasterize -> FragmentShader -> Framebuffer`。

## 技术方案与关键数据流

目标数据流：

```text
Application::Run()
-> resource::LoadFlatColorMesh(path)
-> 按立方体面方向把三角形分组
-> 为每个面构建带 faceUv 的调试 mesh
-> DebugFace Pipeline::Run(framebuffer, faceMesh)
-> fragment shader 根据 faceUv + faceNumber 画出数字
-> Platform::Present(framebuffer)
```

实现要点：

- 保留 `src/resource/loaders/ObjMeshLoader.*` 不变，继续负责最小 OBJ 读取。
- 在 `src/render` 内新增一组面调试着色协议：
  - `DebugFaceVertex`
  - `DebugFaceUniforms`
  - `DebugFaceVaryings`
  - `DebugFaceVertexShader`
  - `DebugFaceFragmentShader`
- `DebugFaceVertex` 除模型空间位置外，还携带面内局部坐标 `faceUv`。
- `DebugFaceFragmentShader` 输出逻辑：
  - 默认返回该面的底色
  - 若当前片元命中数字掩码区域，则返回深色数字颜色
- 数字不通过纹理加载，而是在片元阶段使用程序化的七段数码管风格掩码绘制 1~6。
- `Application` 负责：
  - 加载立方体 mesh
  - 依据三角形法线主轴把 12 个三角形分到 6 个面
  - 为每个面计算稳定的局部 `uv`
  - 给每个面分配固定颜色和编号
  - 逐面调用 pipeline 进行绘制

## 计划修改的文件

- `docs/features/minimal-obj-model-loading.md`
- `src/render/Vertex.h`
- `src/render/Pipeline.h`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- 本轮的“面编号”是针对当前测试立方体的调试可视化，不是通用文字系统。
- 面分组逻辑依赖当前测试模型是轴对齐立方体；若换成任意 OBJ，不保证仍能自动得到稳定的 1~6 面编号。
- 本轮仅扩展渲染协议和示例装配，不改变资源层职责，也不新增模块目录。
- 因为没有引入真正字体，数字外观会偏工程化、示意化，而不是排版级文本效果。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行 viewer
   - `build-mingw/bin/sr_viewer.exe`
4. 观察结果
   - 窗口中应看到 3D 立方体
   - 当前可见的不同面应具有不同底色
   - 每个可见面上应能直接看到清晰的数字编号
5. 异常观察点
   - 若只有颜色变化没有数字，优先检查片元阶段数字掩码逻辑
   - 若数字被拉伸或翻转，优先检查每个面的局部 `uv` 映射方向
   - 若面颜色和编号错位，优先检查三角形分组与每面 draw call 装配顺序

## 完成标准 / 验收标准

1. 已理解并对齐 review 需求。
2. 已更新开发文档。
3. viewer 已继续通过最小 OBJ loader 读取固定立方体模型。
4. 立方体不同面已具有不同底色。
5. 当前可见面上已能直接看到 1~6 的编号标记。
6. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
7. 已提供更新后的手动测试方法。
8. 等待用户 review。

