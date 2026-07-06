cmake_minimum_required(VERSION 3.16)
project(AlgorithmViz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

add_executable(AlgorithmViz
    src/main.cpp
    src/core/canvas.cpp
    src/core/canvas.h
    src/core/shape_base.cpp
    src/core/shape_base.h
    src/shapes/rect_shape.cpp
    src/shapes/rect_shape.h
    src/shapes/ellipse_shape.cpp
    src/shapes/ellipse_shape.h
    src/shapes/diamond_shape.cpp
    src/shapes/diamond_shape.h
    src/shapes/arrow_shape.cpp
    src/shapes/arrow_shape.h
    src/shapes/text_label.cpp
    src/shapes/text_label.h
    src/connectors/connector.cpp
    src/connectors/connector.h
    src/editor/html_editor.cpp
    src/editor/html_editor.h
    src/export/svg_exporter.cpp
    src/export/svg_exporter.h
    src/ui/main_window.cpp
    src/ui/main_window.h
)

target_link_libraries(AlgorithmViz PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui)
