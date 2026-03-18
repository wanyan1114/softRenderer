# 可支撑项目 Demo 的数学库实现

## 功能背景

当前仓库已经具备最小应用层与平台层骨架，但 `src/base/` 仍为空。后续无论是相机、MVP 变换、顶点着色流程，还是最小可视化 demo，都需要一套统一的向量与矩阵基础设施。

本轮目标不是实现一个覆盖面很大的通用数学框架，而是落地一版足够支撑项目 demo 演进的第一版数学库。

在本轮 review 迭代中，接口组织方式进一步收敛为“以类为中心，但不把所有矩阵构造都塞进 `Mat4` 类本身”：向量和矩阵数据结构仍以类形式暴露，和对象强相关的运算保留在类内，而带明显图形学语义的相机/旋转矩阵构造函数放在类外。

## 需求摘要

- 提供基础类型：`Vec2`、`Vec3`、`Vec4`、`Mat4`
- 向量相关运算放在对应向量类中：算术、`Dot`、`Cross`、`Length`、`Normalized`、`Lerp`
- `Mat4` 内保留矩阵乘法、矩阵乘向量、下标访问、基础矩阵工厂
- `RotateX`、`RotateY`、`RotateZ`、`LookAt`、`Perspective` 改为类外函数
- 保留少量标量辅助：`Clamp`、标量 `Lerp`
- 暂不提供：角度/弧度转换、浮点比较辅助、`Mat3`、四元数、SIMD 优化、泛型模板化扩展
- 需要有一条最小验证链路，证明这些数学类型和矩阵已经能支撑 demo 级变换计算

## 对应阅读的 `computer-graphics-notes` 模块

- `computer-graphics-notes/lesson03-math-and-camera.md`
- `computer-graphics-notes/lesson01-project-overview-and-entry.md`

## 笔记对当前功能的约束与启发

- `lesson03-math-and-camera.md` 直接约束第一版数学库必须优先覆盖向量、矩阵、`LookAt`、`Perspective`，因为它们构成了从世界空间到裁剪空间的核心链路。
- `lesson03-math-and-camera.md` 强调了“位置”和“方向”的语义差异，因此需要让 `Vec4` 和矩阵接口能够自然表达 `w = 1` 的点与 `w = 0` 的方向。
- `lesson03-math-and-camera.md` 也提醒了数值稳定性问题，例如归一化与 `LookAt` 中的退化风险，因此实现时会对零长度和退化叉乘加入轻量回退，避免直接产生非法结果。
- `lesson01-project-overview-and-entry.md` 说明当前项目仍是骨架阶段，所以数学库应该放在基础层，先做干净、明确、可复用的接口，而不是过早引入复杂的高级抽象。

## 技术方案与关键数据流

## 类型设计

- 所有数学类型放在 `sr::math` 命名空间
- `Vec2`、`Vec3`、`Vec4`、`Mat4` 使用 `class` 组织
- `Vec2`、`Vec3`、`Vec4` 采用 `float` 分量
- `Mat4` 采用 `float m[4][4]` 存储

## 接口组织方式

- 向量算术运算通过成员运算符提供
- 向量点乘、长度、归一化使用成员函数，例如 `dir.Normalized()`、`a.Dot(b)`
- `Vec3::Cross` 作为三维向量专属运算放在 `Vec3` 中
- 向量插值使用对应类型的静态函数，例如 `Vec3::Lerp(a, b, t)`
- 矩阵乘法和矩阵乘向量通过 `Mat4::operator*` 提供
- `Mat4` 中只保留偏基础的矩阵能力，例如 `Identity`、`Translate`、`Scale`
- 带明确图形学构造语义的函数放在类外，包括 `RotateX`、`RotateY`、`RotateZ`、`LookAt`、`Perspective`

这样既保留了类式数据组织，也避免把过多图形学工厂函数全部塞进 `Mat4`。

## 矩阵与向量约定

- 采用 4x4 矩阵
- 采用“矩阵左乘列向量”的计算方式：`clip = projection * view * model * position`
- 变换矩阵中的平移量放在最后一列

这套约定与后续 MVP 变换链更容易对齐，也方便后续在渲染主线中直接复用。

## 相机矩阵约定

- `LookAt(eye, target, up)` 生成右手坐标系下的视图矩阵
- `Perspective(fovyRadians, aspect, nearPlane, farPlane)` 使用弧度输入
- 当前不提供角度转弧度辅助，因此调用方需要自行传入弧度

## 最小验证链路

在应用层增加一个轻量 smoke check，验证：

- 向量归一化结果长度正确
- 模型矩阵可以把点从局部空间平移到世界空间
- `LookAt` 与 `Perspective` 可组成 `projection * view * model`
- 点经过 MVP 后能得到合理的裁剪空间坐标

这条验证链路不等于正式渲染，但可以证明第一版数学库已经足够支撑 demo 主链的关键计算。

## 计划修改的文件

- `src/base/Math.h`
- `src/base/Math.cpp`
- `src/CMakeLists.txt`
- `src/app/Application.cpp`

## 风险点与兼容性影响

### 风险点

- 如果矩阵乘法约定不明确，后续接入渲染管线时容易出现“结果看起来对，但方向全反”的问题。
- `LookAt` 在 `up` 与视线方向接近平行时存在退化风险，本轮会做轻量回退，但调用方仍应尽量提供合理的 `up`。
- `Mat4` 类内工厂和类外工厂并存时，命名边界必须保持稳定，否则后续调用方会混乱。
- 当前没有单元测试框架，第一轮主要依赖最小 smoke check 与手工观察。

### 兼容性影响

- 当前 `src/base/` 仍只有这一版数学库使用者，本轮重构不会破坏其他业务模块。
- `Application.cpp` 只会增加轻量演示输出，不改变现有窗口创建流程。

## 手动测试方案

1. 配置工程
   - `cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/mingw64/bin/g++.exe"`
2. 编译工程
   - `cmake --build build-mingw`
3. 运行程序
   - `build-mingw/bin/sr_viewer.exe`
4. 观察控制台输出
   - 应看到数学库 smoke check 输出
   - 输出中应包含归一化长度、世界坐标、裁剪坐标等信息
5. 预期现象
   - 归一化后的长度接近 `1`
   - 世界坐标应体现平移矩阵对点的影响
   - 裁剪坐标应为有限数值，不应出现 `nan` 或 `inf`
6. 如果结果异常，优先观察
   - 是否使用了新的类外矩阵构造 API
   - 矩阵乘法顺序是否与文档约定一致
   - `Perspective` 的输入是否为弧度
   - `LookAt` 的 `up` 是否与视线方向过于接近

## 完成标准 / 验收标准

本轮功能被视为达到交付条件，需要满足：

1. `src/base/` 中存在可复用的 `Vec2`、`Vec3`、`Vec4`、`Mat4` 类
2. 向量和矩阵的核心运算已经收回对应类中
3. `RotateX/Y/Z`、`LookAt`、`Perspective` 已放在类外
4. 至少能完成一条从模型点到裁剪空间点的最小计算链
5. 已提供清晰的手动测试步骤
6. 代码与文档保持一致，等待用户 review
