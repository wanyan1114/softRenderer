# 最小法线支持与基础光照

## 功能背景

当前仓库已经具备：

- 最小 OBJ 模型加载能力
- 可编程软光栅主线
- 立方体按面调试颜色与编号显示

但模型仍主要依赖纯色或调试着色，缺少最基础的明暗层次。为了让模型从“几何可见”升级到“具有立体感”，需要把 OBJ 中的法线数据接入运行时，并在渲染层增加一个最小基础光照模型。

上一轮已经把最小 `Blinn-Phong` 光照接进来了，但从 review 结果看，当前暗部尤其是底面仍偏黑，影响调试可读性。本轮迭代的目标是在保持基础光照模型不变的前提下，重新平衡环境光与直射光，让背光面不再出现大面积死黑。

## 需求摘要

- 保留当前 `vn + 单光源基础 Blinn-Phong` 路径。
- review 迭代中需要避免背光面和底面过黑，保证调试面在暗部仍有可读性。
- 立方体不同面的底色与编号仍需保持清晰可辨。
- 本轮优先排查并修正：
  - 环境光过低
  - 光照方向过于集中
  - 暗部缺少过渡层次
- 本轮不做：
  - 纹理采样
  - 法线贴图
  - PBR
  - 多光源
  - 阴影
  - 完整线性空间 / gamma 工作流重构

## 对应阅读的项目架构图

- `docs/architecture/project-architecture.md`

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson05-vertex-uniform-varyings-and-programmable-shader-interface.md`
- `computer-graphics-notes/lesson07-model-and-texture-resource-loading.md`
- `computer-graphics-notes/lesson08-from-blinn-phong-to-pbr.md`

## 架构图与笔记对当前功能的约束/启发

- 架构图约束光照公式、顶点协议和插值数据应继续留在 `src/render`，应用层只负责装配相机、光源和材质参数。
- `lesson05` 约束暗部改善仍应通过 `uniforms / varyings / fragment shader` 路径完成，而不是在应用层直接改像素。
- `lesson08` 提醒当前阶段仍是教学版最小 Blinn-Phong，因此这次更适合优化环境项和平衡参数，而不是直接切换到更复杂模型。

## 技术方案与关键数据流

目标数据流保持不变：

```text
Application::Run()
-> resource::LoadLitMesh(path)
-> 组装 render::LitVertex
-> DebugFace shader 输出 worldPos + worldNormal
-> fragment shader 计算 ambient + diffuse + specular
-> 叠加数字掩码
-> Present
```

本轮主要调整点：

- 将常量环境光升级为更平滑的环境项，优先采用半球环境光（sky/ground ambient）来抬亮侧面和底面。
- 保留单光源 Blinn-Phong 的 diffuse/specular 逻辑。
- 必要时微调光源位置和默认参数，让当前观察角度下的 3 个可见面亮度层次更均衡。
- 编号叠加顺序保持在最终底色之上，确保数字可见性不被暗面吞掉。

## 计划修改的文件

- `docs/features/minimal-normals-and-lighting.md`
- `src/render/Vertex.h`
- `src/app/Application.cpp`

## 风险点与兼容性影响

- 为了改善当前暗部过黑的问题，本轮会重新平衡环境光和直射光参数，因此画面亮度分布会有小幅变化。
- 本轮不新增模块目录，也不改变跨层依赖方向。
- 由于尚未做完整 gamma 工作流，当前优化以“视觉更稳、更适合调试”为主，而不是严格物理正确。

## 手动测试方案

1. 运行 viewer
   - `build-mingw/bin/sr_viewer.exe`
2. 观察结果
   - 立方体不同面的底色应仍然可区分
   - 当前朝向光源的面应更亮，背光面应更暗，但不应出现大面积死黑
   - 底面和侧面上的数字仍应可见
3. 异常观察点
   - 若整体仍像纯色，优先检查环境光与 diffuse 比例
   - 若暗部还是太死黑，优先检查环境项是否只剩常量而没有方向过渡
   - 若编号变差，优先检查数字叠加顺序和颜色对比度

## 完成标准 / 验收标准

1. 已理解并对齐 review 问题。
2. 已更新开发文档。
3. 当前示例的暗部亮度分布已改善。
4. 不同面和编号的可读性仍保持稳定。
5. 已检查是否需要更新 `docs/architecture/project-architecture.md`。
6. 已提供更新后的手动测试方法。
7. 等待用户 review。
