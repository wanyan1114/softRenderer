# STL 目录模型加载与 Viewer 渲染

## 功能背景

当前仓库的 viewer 已经具备：

- `OBJ + MTL + base color 纹理` 的最小资源加载链路
- 基础光照、相机控制和帧缓冲显示
- 软光栅主线与窗口显示能力

但当前资源层仍只支持 OBJ，应用入口也固定加载 `assets/models/cube.obj`。这意味着像本次提供的：

- `assets/models/《斩服少女》缠流子_乐图网tuei.cn/`
- 目录下多个二进制 `STL` 分片

还不能直接进入现有 viewer 渲染链路。

本轮目标是在不改动软光栅主线职责的前提下，为项目补上一条最小 STL 资源接入链，使 viewer 能把该目录中的多个 STL 一起正确渲染出来，并且在移动相机时不再重复读盘。

## 需求摘要

- viewer 需要能够渲染目录 `assets/models/《斩服少女》缠流子_乐图网tuei.cn` 下的模型。
- 目录中的多个 `STL` 文件应一起加载，并作为一个完整模型集合显示。
- 这次 review 迭代以“正确渲染优先”为主，不再默认使用会破坏表面的抽稀预览。
- STL 数据应在启动时一次性读入内存缓存；后续相机变化只重新渲染，不再重新流式读盘。
- 首版不要求纹理、材质文件或动画，只要求稳定显示几何和基础光照。
- 为了方便区分不同分件，按 STL 文件维度赋予不同颜色。
- 继续使用当前 viewer 的相机控制、主循环和软光栅主线。

本轮不做：

- OBJ 与 STL 的通用场景系统
- STL 材质、纹理、法线贴图支持
- 模型导入 UI 或运行时文件浏览器
- 自动修复破损网格、焊接顶点、重建拓扑
- 多目录批量切换

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson07-model-and-texture-resource-loading.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束文件格式解析仍应留在 `src/resource`，不能把 STL 二进制解析写进 `src/app`。
- 架构图约束 `src/app` 继续只负责“选择资源路径、组装场景、调度渲染”，而不是承担模型文件语义。
- `lesson07` 直接对应本轮要补的资源链：`磁盘 STL -> CPU 三角形缓存 -> render pipeline`。
- `lesson06` 说明 STL 只要能以顶点三角形的形式进入 `Pipeline`，就可以复用现有软光栅主线，无需重写裁剪或光栅化阶段。

## 技术方案与关键数据流

目标数据流：

```text
Application::Run()
-> resource::LoadCachedLitStlScene(directory)
-> 启动时一次性读取所有 *.stl 到内存缓存
-> 统计 part 列表、总三角形数和整体包围盒
-> app 层根据包围盒做居中/缩放
-> render 时按 part 遍历缓存中的三角形数据
-> 每个三角形直接送入 Pipeline::RunTriangle(...)
-> Framebuffer
-> Platform::Present(framebuffer)
```

实现要点：

- 在 `src/resource/loaders` 下提供 STL 缓存加载能力：
  - 扫描目录中的 `.stl` 文件
  - 读取 binary STL 面片法线和三个顶点
  - 返回 scene bounds、part 列表和总三角形数
  - 启动时一次性把三角形数据保存在内存中
- 在 `src/render/Pipeline` 中补一个单三角形入口，复用现有主线处理每个 STL 三角形。
- app 层负责：
  - 选择默认 STL 目录
  - 基于包围盒把模型居中并缩放到当前相机视野
  - 为每个 STL 分件指定稳定的展示颜色
  - 仅在相机发生变化时重新触发 STL 全量重渲染，避免静止时重复绘制
  - STL 数据缓存后，后续相机变化只重渲染，不再重新读盘
- 保留 OBJ 路径现状，不回归已有 OBJ 功能。
- 若目录为空、文件损坏、不是二进制 STL 或三角形数异常，资源层应返回明确错误。

## 计划修改的文件

- `docs/features/stl-directory-model-rendering.md`
- `src/app/Application.cpp`
- `src/render/Pipeline.h`
- `src/resource/loaders/StlMeshLoader.h`
- `src/resource/loaders/StlMeshLoader.cpp`

## 风险点与兼容性影响

- 本轮优先支持当前目录中的二进制 STL；若后续遇到 ASCII STL，需要再补格式分支。
- STL 没有材质与 UV，本轮会以分件颜色 + 基础光照作为可视化方案，外观不是最终材质效果。
- 启动时一次性缓存全部 STL 三角形会增加内存占用，但能换来移动相机时不再重复读盘。
- 全量重渲染仍然会显著降低 STL 场景的交互帧率，但能保证模型表面正确，不再出现点云化/爆面式预览结果。
- 本轮保持 `app -> resource -> render` 的依赖方向，不新增项目级模块目录。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行 viewer
   - `build-mingw/bin/sr_viewer.exe`
4. 观察结果
   - viewer 应成功打开窗口并显示缠流子模型
   - 模型表面应连续，不再呈现大面积点云感或膨胀的爆裂三角形
   - 不同 STL 分件应能看到明显色差
   - 首次启动会有一次加载成本，但后续移动相机时不应再次出现 STL 流式读盘行为
5. 异常观察点
   - 若启动即失败，优先查看控制台中的 STL 解析错误
   - 若窗口打开但模型全黑，优先检查法线和光照输入
   - 若移动相机后仍明显出现“重新加载 STL”现象，优先检查 app 是否仍在调用逐文件访问逻辑而不是缓存逻辑
   - 若内存占用明显升高，这属于全量缓存 STL 的预期代价，重点先确认结果是否正确

## 完成标准 / 验收标准

1. 已完成需求对齐。
2. 已生成并更新开发文档。
3. viewer 已能读取目标目录下的多个 STL 文件。
4. STL 已在启动时一次性加载到内存缓存。
5. 相机移动时不再重新流式读盘。
6. viewer 已能正确显示该模型集合，不再出现上一版的点云化预览结果。
7. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
8. 已提供清晰的手动测试方法。
9. 等待用户 review。
