# Mesh 与 Draw() 骨架

## 功能背景

当前仓库已经具备最小 `Framebuffer`、`Vertex` 与 `Renderer::DrawTriangle()`，应用层可以直接提交一个裸三角形完成显示。但从渲染器分层来看，现阶段还缺少一层“渲染输入容器”和“draw call 入口”抽象。

如果继续让应用层手动逐个提交三角形，后续扩展到模型加载、批量绘制和更复杂的渲染数据流时，接口会过早耦合到 `DrawTriangle()` 这一细粒度函数。

本轮目标是先补齐最小 `Mesh + Draw()` 骨架，让项目具备“应用层提交 mesh，Renderer 负责遍历并绘制其中三角形”的基础能力。

## 需求摘要

- 新增最小渲染输入结构 `Triangle` 与 `Mesh`。
- 新增 `Renderer::Draw(const Mesh&)` 作为 draw call 级入口。
- `Renderer::Draw()` 内部遍历 mesh 中的三角形，并复用已有 `DrawTriangle()`。
- 应用层示例改为构造一个 `Mesh` 并调用 `Renderer::Draw()`。
- 本轮只实现最小非索引 mesh 骨架。
- 本轮不实现 OBJ 加载、索引缓冲、通用 shader 接口、深度缓冲或材质系统。

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson07-model-and-texture-resource-loading.md`
- `computer-graphics-notes/summary-notes-rgs.md`

## 笔记对当前功能的约束与启发

- `lesson05` 启发渲染输入结构应独立于 `Renderer` 内部光栅化细节，便于后续扩展更丰富的顶点格式。
- `lesson06` 约束 `Draw()` 应作为 draw call 入口，主要负责遍历 mesh 并驱动单三角形绘制，而不是取代 `DrawTriangle()` 的职责。
- `lesson07` 启发 `Mesh` 应先作为“渲染级输入容器”存在，未来再承接更上层的模型资源加载结果。
- `summary-notes-rgs` 进一步确认了推荐数据流是 `Mesh / Triangle -> Renderer -> Framebuffer -> Window`。

## 技术方案与关键数据流

新增最小渲染输入类型：

- `Triangle`
  - 持有 3 个 `Vertex`
- `Mesh`
  - 持有 `std::vector<Triangle>`

新增批量绘制入口：

```text
Application
-> 构造 Mesh
-> Renderer::Draw(mesh)
-> 遍历 Mesh::Triangles
-> Renderer::DrawTriangle(v0, v1, v2)
-> RasterizeTriangle(...)
-> Framebuffer
```

设计原则：

- `Mesh` 只负责表达渲染输入，不承担加载逻辑。
- `DrawTriangle()` 保留为更底层的最小绘制单元。
- `Draw()` 作为更高层入口，先把 draw call 语义建立起来。
- 当前 `Triangle` 与 `Mesh` 使用非索引布局，保证骨架简单可运行。

## 计划修改的文件

- `docs/features/mesh-and-draw-skeleton.md`
- `src/render/Mesh.h`
- `src/render/Renderer.h`
- `src/render/Renderer.cpp`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- 当前 `Mesh` 为非索引结构，后续接 OBJ 或索引缓冲时需要再扩展数据组织方式。
- 如果后续引入模板化 vertex 格式，`Mesh` 结构可能从当前固定 `Vertex` 版本演进为模板版本。
- 现有 `DrawTriangle()` 仍然保留，因此新旧调用方式会短暂并存。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行程序
   - `build-mingw/bin/sr_viewer.exe`
4. 观察窗口
   - 确认程序仍能正常显示一个三角形
5. 验证骨架调用路径
   - 应用层不再直接调用 `DrawTriangle()`，而是构造 `Mesh` 后调用 `Renderer::Draw()`
6. 异常观察点
   - 若窗口无三角形，优先检查 `Mesh` 是否包含三角形数据，以及 `Renderer::Draw()` 是否正确遍历提交

## 完成标准 / 验收标准

1. 已更新开发文档。
2. 仓库新增最小 `Mesh` 与 `Triangle` 渲染输入骨架。
3. `Renderer` 提供 `Draw(const Mesh&)` 入口。
4. 应用层示例已切换到 `Mesh -> Draw()` 调用路径。
5. 已给出可执行的手动测试说明。
6. 等待用户 review。
