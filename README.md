# 液体火箭发动机管路设计软件

**Liquid Rocket Engine Piping Design Software (LREPDS)**

基于 C++20 / Qt6 / CMake 的液体火箭发动机管路系统设计与仿真平台。

## 功能概览

- **核心计算引擎** — 流体力学模型（MOC 瞬态、SST k-ω 湍流）、气体动力学函数、推进剂物性计算（Wagner / Rackett / Joback / Squires / Pitzer / Nicola / Sastri-Rao）、甲烷 22 系数热状态方程
- **管路网络求解** — Hardy-Cross 迭代法 + 全节点-边矩阵直接求解
- **组件库** — 15 种管路组件（直管/弯头/三通/阀门/泵/传感器/贮箱），工厂模式 + 插件系统动态扩展
- **可视化编辑器** — 基于 QGraphicsScene 的块图编辑器，支持拖放、连线、序列化
- **数值方法** — Lax-Wendroff / RK2 / MUSCL / TVD、混合精度计算、网格无关性 GCI 验证
- **多物理场** — 对流换热（Dittus-Boelter / Sieder-Tate / Gnielinski / Bartz）、流固耦合（Korteweg 波速）
- **脚本引擎** — ExprTk 表达式引擎，支持符号表、微积分、流程控制
- **工程数据** — Crane TP-410 管件阻力系数

## 构建

### 依赖

| 依赖 | 用途 | 来源 |
|------|------|------|
| Qt 6.2+ | GUI（Widgets, Svg） | [aqt](https://github.com/miurahr/aqtinstall) 或官方安装器 |
| Eigen3 | 线性代数 | vcpkg |
| ExprTk | 数学表达式解析 | vcpkg |
| Catch2 3.x | 单元测试 | vcpkg |

### 构建步骤

```bash
# 1. 安装依赖
vcpkg install eigen3 exprtk catch2

# 2. 安装 Qt 6（推荐 aqt）
aqt install-qt windows desktop 6.8.0 win64_msvc2022_64 --outputdir C:/Qt

# 3. 配置 CMake
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.0/msvc2022_64

# 4. 编译
cmake --build build --config Release

# 5. 运行测试
cd build && ctest --output-on-failure
```

## 项目结构

```
├── cmake/                  # CMake 模块（编译器警告配置）
├── resources/              # Qt 资源文件（图标、qrc）
├── src/
│   ├── app/                # 应用入口（Application 类）
│   ├── components/         # 组件系统（描述符、工厂、实例、端口）
│   ├── core/               # 核心类型、常量、插件接口、插件管理器
│   ├── ui/                 # UI 层
│   │   ├── actions/        # 动作管理、撤销命令
│   │   ├── graphics/       # 图形场景（块、端口、连线）
│   │   ├── library/        # 组件库面板
│   │   └── properties/     # 属性编辑器
│   └── utils/              # 计算引擎
│       ├── FluidDynamics   # 流体力学
│       ├── NetworkSolver   # 网络求解
│       ├── TransientSolver # 瞬态求解（MOC）
│       ├── HeatTransfer    # 传热分析
│       ├── PropellantProperties  # 推进剂物性
│       ├── MethaneEOS      # 甲烷状态方程
│       ├── ExpressionEngine # 表达式引擎
│       └── ...             # 数值方法、基准、湍流模型等
├── tests/                  # Catch2 单元测试
├── .clang-format           # 代码格式配置
├── .clang-tidy             # 静态分析配置
├── .githooks/              # Git hooks（pre-commit 格式检查）
├── CMakeLists.txt          # 顶层 CMake
└── vcpkg.json              # vcpkg 清单
```

## 许可

内部项目，暂未公开许可。
