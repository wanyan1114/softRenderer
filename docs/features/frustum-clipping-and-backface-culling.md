# 视锥裁剪与背面剔除

## 功能背景

当前仓库中的最小 `Pipeline` 已经具备顶点着色、透视除法、屏幕映射、包围盒光栅化、片元着色与深度测试能力，但还缺少两段关键的图形学主线：

- 视锥裁剪
- 背面剔除

这会带来两个直接问题：

- 三角形一旦跨出标准裁剪体，当前实现不会沿视锥边界切割，只会在 `clipPos.w <= 0` 时整体跳过，无法正确处理“部分可见”的图元。
- 三角形无论绕序如何都会继续进入光栅化，缺少最基础的背面剔除优化与行为约束。

本轮目标是在保持现有模块边界不变的前提下，把这两段能力补进 `src/render/Pipeline.h` 的主线中，并更新 viewer 示例来提供可观察的手动验证场景。

## 需求摘要

- 在 `clipPos` 阶段实现标准齐次裁剪空间 6 平面视锥裁剪。
- 裁剪发生在顶点着色之后、`NDC -> 屏幕空间` 映射之前。
- 裁剪后如果得到凸多边形，需要通过扇形拆三角继续进入现有光栅化流程。
- 背面剔除默认开启，按屏幕空间绕序判断，约定逆时针 `CCW` 为正面。
- 需要保留关闭背面剔除的能力，以支持双面绘制。
- 本轮只覆盖当前最小 `Pipeline + FlatColor viewer` 路径，不扩展到 MSAA、线框模式、其他图元类型或完整材质系统。
- 现有深度测试与颜色写回逻辑不回退。

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`
- `computer-graphics-notes/lesson03-math-and-camera.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束这次改动必须局限在 `src/render` 渲染主线内部，不能把裁剪或剔除逻辑泄漏到 `src/app` 或 `src/platform`。
- `lesson06` 约束正确顺序必须是：
  `VertexShader -> Clip -> NDC -> FragPos -> Rasterize -> FragmentShader`。
- `lesson06` 同时说明背面剔除应发生在三角形光栅化入口，而不是应用层提前过滤。
- `lesson03` 提醒裁剪使用的是投影后的 `clipPos`，背后依赖的是当前 `MVP` 与齐次坐标约定。

## 技术方案与关键数据流

目标数据流调整为：

```text
Pipeline::Run(framebuffer, mesh)
-> 每 3 个索引装配原始三角形
-> VertexShader 输出 clipPos
-> ClipTriangleAgainstFrustum()
-> 裁剪后凸多边形扇形拆三角
-> FinalizeVertex() 生成 ndcPos / fragPos / screen point
-> RasterizeTriangle()
-> 背面剔除
-> 包围盒遍历与透视正确插值
-> FragmentShader + 深度测试 + 写回 Framebuffer
```

实现要点：

- 在 `Program<...>` 中加入最小渲染状态，用于表达“是否开启背面剔除”。
- 在 `Pipeline` 内新增裁剪辅助结构与函数：
  - 视锥平面枚举
  - 顶点是否在平面内判断
  - 线段与裁剪平面交点比例计算
  - 多边形按单平面裁剪
  - 对 6 个平面连续裁剪
- 裁剪时要同时插值整套 `varyings_t`，不能只更新 `clipPos`。
- 裁剪完成后按扇形方式重新组三角形，每个结果三角形再执行现有顶点整理与光栅化。
- 背面剔除使用屏幕空间有符号面积：
  - 默认 `CCW` 为正面
  - 若启用背面剔除，则面积小于等于 0 的三角形不进入包围盒遍历
- viewer 示例中会显式区分正面绕序、背面绕序和双面模式，避免手测样例本身与剔除约定冲突
- `Application` 中更新示例场景：
  - 至少包含一个跨出视锥边界但应被裁剪后显示的三角形
  - 至少包含一个绕序为背面的三角形，用于验证剔除
  - 保留一组有前后遮挡关系的三角形，防止深度测试回退

## 计划修改的文件

- `docs/features/frustum-clipping-and-backface-culling.md`
- `src/render/Vertex.h`
- `src/render/Pipeline.h`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- `Pipeline` 是模板头文件实现，本轮新增的裁剪逻辑也需要留在头文件中，代码体量会增加。
- 当前 `varyings_t` 依赖“标准布局 + float 兼容字段”来做插值，裁剪阶段会复用这一前提。
- 裁剪会让单个输入三角形展开成多个输出三角形，后续如果引入性能统计，计数口径要注意区分“输入图元”和“裁剪后三角形”。
- viewer 示例画面会发生变化，但这是为了让功能具备可观察验证场景，不属于架构变更。
- 本轮不新增独立模块，也不改变依赖方向，因此通常不需要更新项目架构图文档。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行程序
   - `build-mingw/bin/sr_viewer.exe`
4. 观察结果
   - 一个跨出边界的三角形应只显示落在视锥内的部分，不应整块消失。
   - 一个背面三角形应被剔除，不应出现在窗口中。
   - 一组前后遮挡的三角形应继续保持正确深度关系。
5. 异常观察点
   - 若跨边界三角形整块消失，优先检查裁剪平面测试和交点插值。
   - 若背面三角形仍然可见，优先检查屏幕空间绕序判断和 culling 开关默认值。
   - 若裁剪后颜色或深度异常，优先检查裁剪阶段对 `varyings` 的插值与 `clipPos.w` 处理。

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成开发文档。
3. `Pipeline` 已支持标准视锥裁剪。
4. `Pipeline` 已支持默认开启的背面剔除，并可切换为双面绘制。
5. 裁剪后的图元能继续进入现有光栅化与深度测试主线。
6. viewer 已提供可观察的裁剪与剔除手动验证场景。
7. 已完成编译验证或明确说明未验证项。
8. 已提供清晰的手动测试方法。
9. 等待用户 review。

