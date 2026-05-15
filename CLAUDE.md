# CLAUDE.md

液体火箭发动机管路设计软件 — C++20 / Qt6 / CMake 项目

## 构建命令

```bash
# 配置（Debug）
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=<qt-install-dir>

# 编译
cmake --build build --config Debug

# 运行测试
cmake --build build --config Debug --target lrep_tests
cd build && ctest --output-on-failure
```

## 关键路径

| 路径 | 说明 |
|------|------|
| Qt 安装 | `C:/Qt/6.8.0/msvc2022_64` |
| vcpkg root | `E:/vcpkg` |
| vcpkg toolchain | `E:/vcpkg/scripts/buildsystems/vcpkg.cmake` |

## 架构

- **构建系统**: 顶层 CMake → `src/` 子目录 → 静态库 `lrep_core` / `lrep_components` / `lrep_utils` / `lrep_ui`
- **GUI**: Qt 6 Widgets（非 QML），`QGraphicsScene` 块图编辑器
- **插件系统**: `IPlugin` 接口 → `PluginManager` 动态发现 → `ComponentFactory` 注册
- **计算引擎**: 所有物理模型在 `src/utils/` 中以 header-only 或 `.h/.cpp` 对实现
- **测试**: Catch2 v3，`tests/test_main.cpp` 初始化 `QApplication`（Qt 6 必须）

## 编码规范

- C++20 标准，传统 `#include` 头文件（非 C++20 Modules）
- 命名空间: `FluidDynamics`, `PropellantProperties`, `GasDynamics`, `NumericalMethods`, `HeatTransfer`, `FluidStructureInteraction`
- 组件类型 ID 格式: `pipe.straight`, `valve.ball`, `pump.centrifugal` 等
- 工厂模式: `ComponentFactory::instance()` 单例，`createInstance(typeId)` 创建组件
- 序列化: JSON 格式 → `toJson()` / `fromJson()`

## 测试

```bash
# 运行全部测试
cmake --build build --target lrep_tests && ./build/tests/Debug/lrep_tests

# 运行特定测试
./build/tests/Debug/lrep_tests "ComponentFactory"
```

## 注意事项

- Qt 6 的 `QGraphicsScene` 需要 `QApplication` 存在，测试中必须创建
- 命名空间冲突：`FluidDynamics` 内部的 `PropellantProperties` 引用需用 `::` 全局前缀
- `BlockScene::toJson()` 中 `descriptor()` 按值返回迭代器需要显式局部变量避免 UB
- 文件编码：所有源文件 UTF-8；CMake 文件 LF 换行
