cmake_minimum_required(VERSION 3.16)
project(AlgorithmViz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Svg Test)

# 核心代码封装为静态库，方便主程序以及所有自动化测试用例共用
add_library(algorithmviz_core STATIC
    # === Core ===
    src/core/canvas.cpp
    src/core/canvas.h
    src/core/undo_manager.cpp
    src/core/undo_manager.h

    # === Core / Commands ===
    src/core/commands/undo_commands.cpp
    src/core/commands/undo_commands.h

    # === Core / Shape Controllers ===
    src/core/shape_controller/canvas_controller.cpp
    src/core/shape_controller/canvas_controller.h
    src/core/shape_controller/connector_controller.cpp
    src/core/shape_controller/connector_controller.h
    src/core/shape_controller/anchor_resolver.cpp
    src/core/shape_controller/anchor_resolver.h
    src/core/shape_controller/drawing_constraints.h
    src/core/shape_controller/control_box.h
    src/core/shape_controller/control_box.cpp

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
    src/editor/html_syntax_highlighter.cpp
    src/editor/html_syntax_highlighter.h
    src/editor/html_editor_widget.cpp
    src/editor/html_editor_widget.h
    src/editor/diagnostics_widget.cpp
    src/editor/diagnostics_widget.h
    src/editor/html_validator.cpp
    src/editor/html_validator.h

    # === Export ===
    src/export/scene_export_utils.cpp
    src/export/scene_export_utils.h
    src/export/svg_exporter.cpp
    src/export/svg_exporter.h

    # === UI ===
    src/ui/main_window.cpp
    src/ui/main_window.h
    src/ui/toolbar_widget.cpp
    src/ui/toolbar_widget.h
    src/ui/canvas_widget.cpp
    src/ui/canvas_widget.h
    src/ui/property_panel_widget.cpp
    src/ui/property_panel_widget.h
    src/ui/auto_hiding_dock_widget.cpp
    src/ui/auto_hiding_dock_widget.h
    src/ui/shape_explorer_widget.cpp
    src/ui/shape_explorer_widget.h
    src/core/layout_engine/layout_engine.h src/core/layout_engine/layout_engine.cpp
    src/core/document/diagram_document.h src/core/document/diagram_document.cpp
    src/core/document/document_serializer.cpp
    src/core/document/document_parser.cpp
    src/core/document/project_metadata.h src/core/document/project_metadata.cpp
)

target_link_libraries(algorithmviz_core PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Svg)

# 主程序入口
add_executable(AlgorithmViz src/main.cpp
    )
target_link_libraries(AlgorithmViz PRIVATE algorithmviz_core)

# 自动化单元测试框架集成
enable_testing()
add_executable(connector_test tests/connector_test.cpp)
target_link_libraries(connector_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME ConnectorTest COMMAND connector_test)

add_executable(layout_engine_test tests/layout_engine_test.cpp)
target_link_libraries(layout_engine_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME LayoutEngineTest COMMAND layout_engine_test)

add_executable(undo_commands_test tests/undo_commands_test.cpp)
target_link_libraries(undo_commands_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME UndoCommandsTest COMMAND undo_commands_test)

add_executable(parser_serializer_test tests/parser_serializer_test.cpp)
target_link_libraries(parser_serializer_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME ParserSerializerTest COMMAND parser_serializer_test)

add_executable(html_editor_ui_test tests/html_editor_ui_test.cpp)
target_link_libraries(html_editor_ui_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME HtmlEditorUITest COMMAND html_editor_ui_test)

add_executable(shape_explorer_test tests/shape_explorer_test.cpp)
target_link_libraries(shape_explorer_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME ShapeExplorerTest COMMAND shape_explorer_test)

add_executable(main_window_integration_test tests/main_window_integration_test.cpp)
target_link_libraries(main_window_integration_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME MainWindowIntegrationTest COMMAND main_window_integration_test)

add_executable(svg_exporter_test tests/svg_exporter_test.cpp)
target_link_libraries(svg_exporter_test PRIVATE algorithmviz_core Qt6::Test)
add_test(NAME SvgExporterTest COMMAND svg_exporter_test)

# QWidget/QGraphicsView tests must use Qt's headless platform in CI and local CTest runs.
set(ALGORITHMVIZ_GUI_TESTS
    ConnectorTest
    LayoutEngineTest
    UndoCommandsTest
    ParserSerializerTest
    HtmlEditorUITest
    ShapeExplorerTest
    MainWindowIntegrationTest
    SvgExporterTest
)
set_tests_properties(${ALGORITHMVIZ_GUI_TESTS}
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(html_editor_demo tests/html_editor_demo.cpp)
target_link_libraries(html_editor_demo PRIVATE algorithmviz_core)

add_executable(toolbar_demo tests/toolbar_demo.cpp)
target_link_libraries(toolbar_demo PRIVATE algorithmviz_core Qt6::Widgets)

add_executable(canvas_property_demo tests/canvas_property_demo.cpp)
target_link_libraries(canvas_property_demo PRIVATE algorithmviz_core Qt6::Widgets)

target_include_directories(algorithmviz_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
