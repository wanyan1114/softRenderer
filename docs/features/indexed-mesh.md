# Indexed Mesh

## 功能背景

当前项目中的 `Mesh<vertex_t>` 直接保存三角形列表，适合最小可运行版本，但不适合表达更常见的“顶点缓冲 + 索引缓冲”数据组织方式。

为了让渲染输入更接近真实图形管线，也为后续模型加载与顶点复用打基础，本轮将 `Mesh` 调整为索引式表示：单独保存顶点数组与索引数组，由渲染阶段按三个索引装配出一个三角形。

## 需求摘要

- `Mesh<vertex_t>` 改为保存 `vertices` 与 `indices`。
- 约定每 3 个索引组成一个三角形。
- `Pipeline::Run()` 根据索引装配三角形，再复用现有顶点着色、插值、光栅化与片元着色流程。
- 应用层示例改为使用索引式 Mesh，最终画面效果保持不回退。
- 不保留旧的 `Triangle` 容器式兼容接口。

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson07-model-and-texture-resource-loading.md`
- `computer-graphics-notes/README.md`

## 笔记对当前功能的约束与启发

- `lesson06` 约束这次改动属于图元装配阶段，索引需要先被还原成三角形输入，再进入现有光栅化主线。
- `lesson05` 约束 `Mesh` 仍需保持模板化顶点格式，不能把索引结构绑定到某个固定顶点类型。
- `lesson07` 启发资源输入更适合采用 `vertices + indices` 形式，后续模型加载可以更自然地对接这一表示。
- `README` 说明找不到完全匹配单一模块时，应采用主模块加相关模块联合阅读；本次主模块为 `lesson06`，相关模块为 `lesson05` 和 `lesson07`。

## 技术方案与关键数据流

新的数据流：

```text
Application
-> 构造 Mesh.vertices 与 Mesh.indices
-> Pipeline::Run(framebuffer, mesh)
-> 每 3 个索引装配一次三角形
-> VertexShader
-> 顶点整理 / 光栅化 / 片元着色
-> Framebuffer
```

实现要点：

- 删除 `Triangle<vertex_t>` 类型与基于三角形列表的 `Mesh` 接口。
- `Mesh<vertex_t>` 提供顶点与索引构造方式，以及访问器。
- `Pipeline::Run()` 先检查：
  - `vertices` 非空
  - `indices` 数量至少为 3
  - `indices.size() % 3 == 0`
- 若遇到越界索引，跳过对应三角形，避免非法访问。
- 片元阶段、插值逻辑、深度写回逻辑保持不变。

## 计划修改的文件

- `docs/features/indexed-mesh.md`
- `src/render/Mesh.h`
- `src/render/Pipeline.h`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- 这是一次接口切换，旧的三角形构造方式将被移除，现有调用方需要同步修改。
- 当前仅支持三角形列表语义，不支持线段、点或其他 primitive topology。
- 当前索引类型计划使用 `std::uint32_t`，后续如果需要更小索引类型，再考虑模板化或别名扩展。
- 对非法索引采用“跳过该三角形”策略，能保证运行安全，但不会主动报错中断。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=\"C:/msys64/mingw64/bin/g++.exe\"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行程序
   - `build-mingw/apps/viewer/sr_viewer.exe`
4. 观察结果
   - 窗口应继续显示当前四个有前后遮挡关系的彩色三角形。
5. 重点验证
   - 应用层已改为传入 `vertices + indices`
   - `Pipeline::Run()` 不再依赖旧三角形容器
6. 异常观察点
   - 若画面缺失三角形，优先检查索引顺序、索引是否越界，以及 `indices` 数量是否为 3 的倍数。

## 完成标准 / 验收标准

1. 已生成开发文档。
2. `Mesh<vertex_t>` 已切换为索引式表示。
3. `Pipeline::Run()` 已按每 3 个索引装配三角形。
4. 应用示例已改为使用 `vertices + indices`。
5. 已完成编译验证或明确说明未验证项。
6. 已提供清晰的手动测试方法。
7. 等待用户 review。
