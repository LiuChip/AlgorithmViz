QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = AlgorithmViz
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/core/canvas.cpp \
    src/core/shape_base.cpp \
    src/shapes/rect_shape.cpp \
    src/shapes/ellipse_shape.cpp \
    src/shapes/diamond_shape.cpp \
    src/shapes/arrow_shape.cpp \
    src/shapes/text_label.cpp \
    src/connectors/connector.cpp \
    src/editor/html_editor.cpp \
    src/export/svg_exporter.cpp \
    src/ui/main_window.cpp

HEADERS += \
    src/core/canvas.h \
    src/core/shape_base.h \
    src/shapes/rect_shape.h \
    src/shapes/ellipse_shape.h \
    src/shapes/diamond_shape.h \
    src/shapes/arrow_shape.h \
    src/shapes/text_label.h \
    src/connectors/connector.h \
    src/editor/html_editor.h \
    src/export/svg_exporter.h \
    src/ui/main_window.h
