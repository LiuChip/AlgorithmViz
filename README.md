# AlgorithmViz — 算法图形可视化与 HTML 双向编辑器

AlgorithmViz 是一个使用 **C++17 / Qt 6** 开发的桌面图形编辑器，面向流程图、数据结构示意图和算法演示图的快速制作。项目以 `QGraphicsScene` 为画布模型，同时把工程保存为可读、可编辑的 HTML；用户既可以直接操作图形，也可以编辑代码并显式执行回画布。

## 当前能力

### 图形与连接

- 矩形、椭圆、菱形、文本标签。
- 直线、单向箭头、双向箭头。
- 可吸附连接线：边界角度锚点、内部归一化锚点和运行时 Free 端点。
- 图形移动、缩放、旋转、框选、多选、复制、粘贴、删除。
- 几何锁定：阻止位置、尺寸、旋转和缩放变化，但仍允许编辑样式与文本。
- 控制盒及屏幕尺寸稳定的缩放/旋转手柄。

### 编辑器与工程文件

- HTML 语法高亮、行号、当前行高亮、补全和诊断标记。
- 画布修改自动序列化到编辑器。
- 编辑器修改仅在点击右下角 `▶` 或按 `Ctrl+Enter` 后应用到画布。
- 工程元数据保存在 HTML 的 JSON `<script>` 块中；解析器兼容旧元数据标识。
- 打开、保存、另存为、最近文件、未保存更改确认。
- PNG、JPEG、SVG 和完整 HTML 导出。

### UI

- 自动隐藏标题栏的可停靠面板。
- 四套布局预设：
  - Classic 2x3（默认）
  - Dual-Core Split
  - Zen / Pure Canvas
  - Code Focused
- 图形列表（Shape Explorer）：
  - 按 Z 值降序显示顶层图形；
  - 图标、名称、可见性和锁定状态；
  - 画布与列表双向选择同步；
  - 名称、可见性、锁定和层级修改进入撤销栈；
  - 单项拖拽到条目上合并层级，拖拽到条目间隙或列表底部重排层级。
- 属性面板：位置、尺寸、旋转、图层、边框、填充、文本和 `TextLabel` 独有布局模式。
- 诊断面板：显示级别、行号和消息，双击可跳转到编辑器行。

## 操作说明

### 鼠标交互
- **左键单击**：在选择模式下，选中单个图形。在其他工具模式下，在画布上创建对应的图形。
- **左键拖拽**：
  - 在选择模式下，点击空白处拖拽可拉出橡皮筋框选多个图形。
  - 在选择模式下，按住选中的图形拖拽可移动它们。
  - 拖拽图形上的控制手柄可以缩放或旋转图形。
  - 在连线模式下，按住左键从一个锚点拖拽到另一个锚点，即可创建连接线。
- **右键拖拽**：在画布任意位置按住鼠标右键并拖动，可以平移视角。
- **滚轮**：向上滚动放大画布，向下滚动缩小画布。

### 键盘快捷键
- `Delete` / `Backspace`：删除当前选中的图形。
- `Ctrl + Z` / `Cmd + Z`：撤销上一步操作。
- `Ctrl + Y` / `Cmd + Shift + Z`：重做上一步操作。
- `Ctrl + S` / `Cmd + S`：保存当前工程。
- `Ctrl + O` / `Cmd + O`：打开工程文件。
- `W` / `A` / `S` / `D` 或 **方向键**：微调选中的图形位置（长按可加速移动）。
- `Ctrl + Enter` / `Cmd + Enter`：在 HTML 编辑器中强制将代码变更解析并应用回画布。
## 核心设计约束

- `Shape` 继承 `QGraphicsObject`，不允许无参构造，创建时必须给出位置和尺寸。
- `ConnectableShape` 只表示可被连接的封闭图形；自由线和 `Connector` 不作为吸附目标。
- `LineShape` 系列以端点为几何真源，宽高是只读派生结果。
- `Connector` 直接继承 `Shape`，避免连接线吸附连接线形成递归关系。
- 工程加载采用两阶段恢复：先创建所有普通图形并建立 ID 映射，再解析连接线。
- 文件中的显式 ID 可按任意顺序恢复；加载器会推进全局 ID 计数器，后续新建图形不会与已加载对象冲突。
- 用户可见的画布操作统一通过 `UndoManager` / `QUndoCommand` 提交。
- 布局快照按事务应用；任一图形执行失败时逆序回滚已应用修改。

## 项目结构

```text
src/
├── main.cpp
├── core/
│   ├── canvas.*                    # QGraphicsView、镜头控制和输入路由
│   ├── undo_manager.*              # QUndoStack 外壳
│   ├── commands/undo_commands.*    # 创建、删除、移动、属性、布局等命令
│   ├── document/                   # 场景快照、HTML 解析/序列化、项目元数据
│   ├── layout_engine/              # 对齐、网格、尺寸匹配和事务快照
│   └── shape_controller/           # 通用交互、控制盒、连接线和锚点解析
├── shapes/                         # Shape 层次、具体图形和 Connector
├── editor/                         # HTML 编辑器、高亮器、校验器和诊断面板
├── export/                         # 场景导出公共逻辑与 SVG 导出
└── ui/                             # 主窗口、工具栏、画布容器、属性/图形列表和 Dock

tests/
├── connector_test.cpp
├── layout_engine_test.cpp
├── undo_commands_test.cpp
├── parser_serializer_test.cpp
├── html_editor_ui_test.cpp
├── shape_explorer_test.cpp
├── main_window_integration_test.cpp
└── svg_exporter_test.cpp
```

## 构建

### 依赖

- CMake 3.16+
- 支持 C++17 的编译器
- Qt 6，组件：`Core`、`Gui`、`Widgets`、`Svg`、`Test`
- Ninja（推荐，但不是必须）

### 配置与编译

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/macos
cmake --build build --parallel 2
```

如果 Qt 已在 CMake 的默认搜索路径中，可以省略 `CMAKE_PREFIX_PATH`。

### 运行

```bash
./build/AlgorithmViz
```

macOS 使用 app bundle 时也可以运行：

```bash
./build/AlgorithmViz.app/Contents/MacOS/AlgorithmViz
```

## 测试

项目注册了 8 个 CTest 测试目标，共 134 个具名 QTest 回归用例：

```bash
ctest --test-dir build --output-on-failure
```

需要检查更严格的编译诊断时：

```bash
cmake -S . -B build-strict -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/macos \
  -DCMAKE_CXX_FLAGS='-Wall -Wextra -Wpedantic -Wshadow -Wconversion'
cmake --build build-strict --parallel 2
ctest --test-dir build-strict --output-on-failure
```

GUI 测试由 CMake 自动设置 `QT_QPA_PLATFORM=offscreen`，可在无显示环境中运行。

## Demo

构建还会生成三个独立 UI 演示程序：

```bash
./build/html_editor_demo
./build/toolbar_demo
./build/canvas_property_demo
```

## 开发交接

详细架构决议、模块职责、同步流程、布局拓扑和扩展检查清单记录在本地 `AI_HANDOVER.md`。该文件按项目约定加入 `.gitignore`，用于 AI/开发者之间的本地交接，不作为发布产物。
