cmake_minimum_required(VERSION 3.16)
project(AlgorithmViz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Test)

# 核心代码封装为静态库，方便主程序以及所有自动化测试用例共用
add_library(algorithmviz_core STATIC
    # === Core ===
    src/core/canvas.cpp
    src/core/canvas.h
    src/core/undo_manager.cpp
    src/core/undo_manager.h

    # === Core / Shape Controllers ===
    src/core/shape_controller/canvas_controller.cpp
    src/core/shape_controller/canvas_controller.h
    src/core/shape_controller/connector_controller.cpp
    src/core/shape_controller/connector_controller.h

    # === Shapes ===
    src/shapes/shape.cpp
    src/shapes/shape.h
    src/shapes/connectable_shape.cpp
    src/shapes/connectable_shape.h
    src/shapes/rect_shape.cpp
    src/shapes/rect_shape.h
    src/shapes/ellipse_shape.cpp
    src/shapes/ellipse_shape.h
    src/shapes/diamond_shape.cpp
    src/shapes/diamond_shape.h
    src/shapes/line_shape.cpp
    src/shapes/line_shape.h
    src/shapes/arrow_shape.cpp
    src/shapes/arrow_shape.h
    src/shapes/dual_arrow_shape.cpp
    src/shapes/dual_arrow_shape.h
    src/shapes/text_label.cpp
    src/shapes/text_label.h

    # === Shapes / Connector ===
    src/shapes/connector/connector_anchor.cpp
    src/shapes/connector/connector_anchor.h
    src/shapes/connector/connector.cpp
    src/shapes/connector/connector.h

    # === Editor ===
    src/editor/html_editor.cpp
    src/editor/html_editor.h

    # === Export ===
    src/export/svg_exporter.cpp
    src/export/svg_exporter.h

    # === UI ===
    src/ui/main_window.cpp
    src/ui/main_window.h
)

target_link_libraries(algorithmviz_core PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui)

# 主程序入口
add_executable(AlgorithmViz src/main.cpp)
target_link_libraries(AlgorithmViz PRIVATE algorithmviz_core)

# 自动化单元测试框架集成
enable_testing()
add_executable(connector_test tests/connector_test.cpp)
target_link_libraries(connector_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME ConnectorTest COMMAND connector_test)
