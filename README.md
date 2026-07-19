# AlgorithmViz — 算法图形可视化与编辑工具

> 当前版本：`v0.2.0-alpha` (阶段一闭环 / 阶段二进行中)
> 详细技术交接与类参考规格文档，请参阅单点真实数据源：[AI_HANDOVER.md](file:///Users/liuchip/Workspace/Project%20C%20&%20C++/AlgorithmViz/AI_HANDOVER.md)

---

## 📌 项目定位

**AlgorithmViz** 是一个轻量级、面向开发者的 C++/Qt 算法图形可视化与编辑器。
它的核心产品目标是通过简洁流畅的交互，快速绘制树、图、数据结构演示步骤、流程图等算法图，并在后续将图元一键导出为 **高适配 HTML/SVG 代码片段**，方便直接插入 Markdown 学习笔记、学术博客或文档中。

> **设计准则**：轻量优先、自愈容错、极简易用——不仅够用，而且健壮。

---

## 🛠️ 技术栈

| 模块组件 | 技术 / 框架 | 关键说明 |
| :--- | :--- | :--- |
| **开发语言** | C++17 | 使用现代 C++ 标准算法与数学检查函数 (`std::isfinite` / `std::clamp`) 确保边界数值防护 |
| **图形界面** | Qt 6.11.1 | 采用 `QGraphicsView` + `QGraphicsScene` + `QGraphicsObject` 构建高性能矢量渲染画布 |
| **构建系统** | CMake 3.20+ + Ninja | 开启严格编译诊断选项 (`-Wall -Wpedantic -Wshadow`) 确保生产级编译零错误零警告 |
| **测试驱动** | QTest / ctest | 搭载 57 个自动化核心与安全测试用例，提供毫秒级高确定性闭环回归 |

---

## 📁 核心项目结构

```text
src/
├── main.cpp                       # 主运行入口
├── core/
│   ├── canvas.h / .cpp            # [已完成] 独立 Canvas 组件（QGraphicsView 缩放平移与事件路由总线）
│   ├── shape_controller/
│   │   ├── connector_controller.*  # [已完成] 连线专用控制器（吸附创建与端点重连管理）
│   │   ├── anchor_resolver.*       # [已完成] 物理空间最短投影吸附解析器
│   │   ├── canvas_controller.*     # [待实现] 通用图元选择、创建、移动与删除控制器
│   │   └── control_box.*           # [待新建] 选中控制盒图元与 8向/旋转手柄
│   ├── layout/
│   │   └── layout_engine.*         # [待新建] 布局引擎（对齐/分布/尺寸统一/紧凑排列）
│   └── undo/
│       ├── undo_manager.*          # [已完成] QUndoStack 外壳与状态暴露组件
│       └── undo_commands.*         # [待新建] 11 类具体操作命令子类
├── shapes/
│   ├── shape.*                    # [已完成] 抽象图元基类 (继承 QGraphicsObject，含三层样式与保护机制)
│   ├── connectable_shape.*        # [已完成] 封闭多边形可吸附基类与 AnchorSpec 规范
│   ├── rect_shape.*               # [已完成] 矩形
│   ├── ellipse_shape.*            # [已完成] 椭圆 / 圆形
│   ├── diamond_shape.*            # [已完成] 菱形
│   ├── line_shape.*               # [已完成] 自由线段 (重载屏蔽常规尺寸缩放与被吸附)
│   ├── arrow_shape.* / dual_.*    # [已完成] 单箭头 / 双箭头线
│   ├── text_label.*               # [已完成] 独立文本图元 (支持 AutoSize 与 FixedSize 自动防护)
│   └── connector/
│       ├── connector_anchor.*     # [已完成] 连线端点对象模型 (支持 QPointer 防空悬防线)
│       └── connector.*            # [已完成] 动态自愈连接线 (直接继承 Shape，从源头杜绝自连死循环)
├── editor/                        # [待实现] HTML 片段生成编辑器
├── export/                        # [待实现] SVG 导出模块
└── ui/
    └── main_window.*              # [已完成] 主窗口框架与顶部联动栏
```

---

## 🏗️ 架构设计与各层级职责

系统遵循**数据与视图解耦、单向事件驱动、严格防御性编程**的设计哲学，核心模块分工如下：

1. **底图渲染层 (`Shape` & `ConnectableShape`)**
   - 所有图元继承自 `QGraphicsObject` 以支持 Qt 信号/槽；内置边框 (`Border`)、填充 (`FillStyle`) 与字体 (`TextStyle`) 三级值对象。
   - 虚几何操作方法全部设计为 `virtual bool` 返回值体系，凡参数异常（如 `NaN`、负大小）或图形被锁定即予以阻断拦截。
2. **连接线与吸附引擎 (`Connector` & `AnchorResolver`)**
   - `Connector` 类**直接继承 `Shape`**（不作为 `ConnectableShape`），从类型层次杜绝任何“连接线吸附连接线”的死循环发生。
   - `AnchorResolver` 在场景物理空间计算多边形离散物理线段最短垂直距离与投影关系，配合多向二道检验 (`Double-Check`)，实现极高准度的 10px 边界/内部自动吸附。
   - `Connector` 内部对目标连接点实施 QPointer 去重保护与目标销毁后的**自动降级自愈 (`degradeToFree`)** 机制。
3. **中央导航与事件总线 (`Canvas` & Controllers)**
   - `Canvas` 作为完全独立的 `QWidget` 承载，提供带有安全阈值的 `Ctrl+滚轮` 锚点平滑缩放与双键画布平移。
   - 根据选中的工具模式 (`ToolMode`) 动态分流事件：对于各种线创建工具，通过 `AnchorResolver` 探测——若命中图元则自动由 `ConnectorController` 创建吸附连线，落在空白处则由 `CanvasController` 创建自由线段。
4. **钉子模型与连线联动**
   - 连线端点视作“钉在图形上的钉子”。拖动图元即拖动钉子；连线被动监听图元的 `geometryChanged()` 信号自动重算重绘，形成没有回调死循环的单向响应链。

---

## 🚀 构建与自动化测试

项目使用 CMake 与 C++17 跨平台构建，在 Unix / macOS 环境中，支持通过终端一条命令直接进行全量严格构建与自动化套件回归：

```bash
# 构建并运行 57 项自动化测试套件（跳过 Qt 许可证诊断提示）
export QTFRAMEWORK_BYPASS_LICENSE_CHECK=1
cmake --build build && ctest --test-dir build --output-on-failure
```

> **测试覆盖情况**：自动化套件 `tests/connector_test.cpp` 包含 **57 个全量测试用例**（涵盖无限边界防御、连线克隆降级、连线创建规则收紧 A1/A2、以及 `Canvas` 平移缩放清场回收自测），能在 `0.7s` 内极速 100% 绿色通过！

---

## 📋 Tasks 进度与蓝图总表

项目的完整实施蓝图分为两大阶段与十一大核心任务（具体的类成员 API 规格与算法公式，请见 [AI_HANDOVER.md](file:///Users/liuchip/Workspace/Project%20C%20&%20C++/AlgorithmViz/AI_HANDOVER.md)）：

### ✅ 阶段一：底层核心图形与连线控制系统 (100% 已闭环)
- [x] **Task 1: Shape → QGraphicsObject 重构与几何校验深化**
- [x] **Task 2: ConnectorAnchor 数据结构与 C++ 深度封装**
- [x] **Task 3: Connector 图形实体类与去重自愈处理**
- [x] **Task 4: AnchorResolver 物理几何吸附解析器**
- [x] **Task 5: ConnectorController 交互控制器与 UAF 防护**

### 🔄 阶段二：Canvas 独立 Widget 化与应用层集成 (当前推进中)
- [x] **Task 6: ConnectorController 行为前置收紧**（起点必吸附 A1、终点必吸附 A2、运行时单端 Free 自愈确认）
- [x] **Task 7: Canvas 独立组件化 — 镜头导航与按键/事件动态分流路由**
- [x] **Task 10a: UndoManager 构建与安全清场对接**
- [ ] **Task 8: CanvasController + ControlBox — 图形选择、拖扯创建、位移（含连线钉子联动）与手柄交互** *(进行中)*
- [ ] **Task 9: LayoutEngine — 多选图元对齐、等距分布、尺寸统一与紧凑间距排列**
- [ ] **Task 10b: UndoCommands — 11 业务操作栈命令子类**
- [ ] **Task 11: 最终集成回归与文档封包**

---

## 📝 开发交接与未来计划

本项目主要于 2026 暑假集中构建推进。
任何未来接手的 AI 助手或开发者在增加图形、扩展工具模式或更改核心流程时，**必须且仅需深入研读并遵照 [AI_HANDOVER.md](file:///Users/liuchip/Workspace/Project%20C%20&%20C++/AlgorithmViz/AI_HANDOVER.md) 第八章提供的标准检查单模板**执行开发与回归，确保原架构的高鲁棒性与零警告传统。
