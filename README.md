# AlgorithmViz — 算法图形可视化工具

## 📌 项目定位

**AlgorithmViz** 是一个轻量级的算法图形可视化工具，核心目标是将算法图（树、图、排序步骤、流程图等）转成 **HTML 代码片段**，方便嵌入 Markdown 笔记/博客中。

不是 Visio 替代品，也不是完整的 SVG 编辑器——**够用就好，轻量优先**。

## 🏗️ 技术栈

| 技术 | 用途 |
|:----|:----|
| **C++17** | 核心语言 |
| **Qt 6.11.1** (Widgets + Core + Gui) | GUI 框架 |
| **CMake** | 构建系统 |
| **QGraphicsView / QGraphicsScene** | 画布与图形项管理 |
| **QPainter** | 自定义形状绘制 |

## 📁 项目结构

```
src/
├── main.cpp                    # 入口
├── core/
│   ├── canvas.h / .cpp         # 画布（QGraphicsView + Scene）
│   ├── undo_manager.h / .cpp   # 撤销/重做（待实现）
│   └── shape_controller/
│       ├── canvas_controller.*  # 鼠标交互控制（待实现）
│       └── connector_controller.* # 连线控制（待实现）
├── shapes/
│   ├── shape.h / .cpp          # 形状基类（抽象类，继承 QGraphicsItem）
│   ├── connectable_shape.h / .cpp # 可连接图形基类与锚点数据
│   ├── rect_shape.h / .cpp     # 矩形
│   ├── ellipse_shape.h / .cpp  # 椭圆
│   ├── diamond_shape.h / .cpp  # 菱形
│   ├── text_label.h / .cpp     # 文本标签（填充/边框 + 自动或固定尺寸）
│   ├── line_shape.h / .cpp     # 线段
│   ├── arrow_shape.h / .cpp    # 单箭头（继承 LineShape）
│   └── dual_arrow_shape.h / .cpp # 双箭头（继承 LineShape）
├── editor/
│   └── html_editor.h / .cpp    # HTML 编辑器（待实现）
├── export/
│   └── svg_exporter.h / .cpp   # SVG 导出（待实现）
└── ui/
    └── main_window.h / .cpp    # 主窗口
```

## ✅ 已完成模块

### Shape 基类（完整）
- 继承 `QGraphicsItem`，纯虚 `boundingRect()` + `paint()`
- 三个结构体：`Border`、`FillStyle`、`TextStyle`（各带无参/带参构造）
- 属性：位置(x,y,width,height)、旋转、边框/填充/文本样式、ID、name、visible、layer、lock_stat
- 可连接图形统一提供 `localGeometryPath()`、边框命中判断和 `boundaryPointAtAngle()` 接口；`TextLabel` 不支持锚点
- 当前锚点描述类型为 `ConnectableShape::AnchorSpec`，边界锚点按中心角度保存，内部锚点按归一化坐标保存
- 静态自增 ID 和 Layer 计数器
- 所有 getter/setter 已实现
- `changeVisibility()` / `changeLockStat()` / `setLayer()` 等控制方法

### 形状子类（全部完成）
| 子类 | 说明 |
|:----|:------|
| `RectShape` | 矩形，支持边框+填充+文字 |
| `EllipseShape` | 椭圆/圆形，同上 |
| `DiamondShape` | 菱形（四点多边形），同上 |
| `TextLabel` | 带填充和边框的文本框，默认自动计算尺寸，也支持固定尺寸 |
| `LineShape` | 线段，使用本地端点 + 场景坐标接口管理 |
| `ArrowShape` | 单箭头，终点三角箭头（可配置 arrowSize） |
| `DualArrowShape` | 双箭头，两端都有三角箭头 |

### Canvas 画布
- QGraphicsView + Scene 基础架构
- 抗锯齿渲染、ScrollHandDrag 拖拽、AnchorUnderMouse 缩放

### MainWindow 主窗口
- 1200×800 窗口，Canvas 作为 centralWidget

## 📋 待完成模块

### 🔴 高优先级
| 模块 | 说明 |
|:----|:------|
| **CanvasController** | 鼠标交互（选中/拖拽/Shift约束画正圆正方形） |
| **Connector** | 形状间连线系统（直线/曲线/虚线） |

### 🟡 中优先级
| 模块 | 说明 |
|:----|:------|
| **UndoManager** | 撤销/重做（Command 模式） |
| **MainWindow 快捷键** | Ctrl+C/V/Z、Delete 等 |
| **PropertyPanel** | 属性编辑面板 |
| **SVG Exporter** | SVG 导出 |

### 🟢 低优先级
| 模块 | 说明 |
|:----|:------|
| **HTML Editor** | 算法可视化 HTML 代码片段生成（核心目标） |

## 🎯 设计决策

### 项目定位
- **Lite 版**，不做大而全，够用就行
- 形状库当前种类已够（矩形/椭圆/菱形/箭头/文字）
- 连线系统支持：直线、曲线、虚线
- 颜色/样式等细节后续迭代再丰富

### 核心目标
- **将算法图转成 HTML 代码片段**，便于嵌入 Markdown 笔记
- 不是导出整页 HTML，而是片段

### 技术决策
- Shift 约束画正圆/正方形 → 在 Canvas 层处理，不在 Shape 里
- 复制粘贴 → MainWindow 快捷键绑定 Canvas 方法
- ID 自动分配，不对外提供 setter

## 💻 开发环境

| 环境 | 路径 |
|:----|:----|
| **Mac 本地** | `~/Workspace/Project C & C++/AlgorithmViz/` |
| **Workspace symlink** | `projects/algorithmviz/` |
| **Windows 本地** | `D:\Qt_Project\AlgorithmViz\` |
| **GitHub** | `LiuChip/AlgorithmViz` |

### Qt 安装
- **位置**：`~/Qt/6.11.1/macos`（官方 DMG 安装，非 Homebrew）
- **qmake**：`~/Qt/6.11.1/macos/bin/qmake`
- **CMake**：`CMAKE_PREFIX_PATH` 指向 Qt 安装目录

## 📝 暑假计划（2026）

AlgorithmViz 作为暑假四大任务之一，与 Java 复健、LeetCode 刷题、CSAPP 补坑并行推进。MVP 完成后即可投入使用，后续按需迭代。

### 项目状态
- ✅ 核心 Shape 体系 — 完成
- ✅ 锚点数据模型、几何锁定、TextLabel 自动/固定尺寸 — 已完成
- 🔄 交互与控制 — 待实现
- ❌ 导出与编辑器 — 待实现
