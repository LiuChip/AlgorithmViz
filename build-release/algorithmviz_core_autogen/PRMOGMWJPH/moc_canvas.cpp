/****************************************************************************
** Meta object code from reading C++ file 'canvas.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/core/canvas.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'canvas.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN6CanvasE_t {};
} // unnamed namespace

template <> constexpr inline auto Canvas::qt_create_metaobjectdata<qt_meta_tag_ZN6CanvasE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Canvas",
        "toolModeChanged",
        "",
        "Canvas::ToolMode",
        "mode",
        "editModeChanged",
        "Canvas::EditMode",
        "zoomScaleChanged",
        "scale",
        "gridVisibilityChanged",
        "visible",
        "cursorScenePositionChanged",
        "QPointF",
        "pos",
        "selectionChanged",
        "setToolMode",
        "ToolMode",
        "setEditMode",
        "EditMode",
        "zoomIn",
        "zoomOut",
        "resetZoom",
        "fitToScene",
        "fitToSelection",
        "setGridVisible",
        "toggleGrid",
        "undo",
        "redo",
        "copy",
        "paste",
        "cut",
        "deleteSelected",
        "selectAll",
        "bringSelectedToFront",
        "sendSelectedToBack",
        "clearAllItems"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'toolModeChanged'
        QtMocHelpers::SignalData<void(Canvas::ToolMode)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'editModeChanged'
        QtMocHelpers::SignalData<void(Canvas::EditMode)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 4 },
        }}),
        // Signal 'zoomScaleChanged'
        QtMocHelpers::SignalData<void(qreal)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QReal, 8 },
        }}),
        // Signal 'gridVisibilityChanged'
        QtMocHelpers::SignalData<void(bool)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 10 },
        }}),
        // Signal 'cursorScenePositionChanged'
        QtMocHelpers::SignalData<void(const QPointF &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'selectionChanged'
        QtMocHelpers::SignalData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setToolMode'
        QtMocHelpers::SlotData<void(enum ToolMode)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 4 },
        }}),
        // Slot 'setEditMode'
        QtMocHelpers::SlotData<void(EditMode)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 18, 4 },
        }}),
        // Slot 'zoomIn'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomOut'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'resetZoom'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'fitToScene'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'fitToSelection'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setGridVisible'
        QtMocHelpers::SlotData<void(bool)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 10 },
        }}),
        // Slot 'toggleGrid'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'undo'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'redo'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'copy'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'paste'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'cut'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'deleteSelected'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectAll'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'bringSelectedToFront'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'sendSelectedToBack'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'clearAllItems'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Canvas, qt_meta_tag_ZN6CanvasE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Canvas::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6CanvasE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6CanvasE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6CanvasE_t>.metaTypes,
    nullptr
} };

void Canvas::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Canvas *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->toolModeChanged((*reinterpret_cast<std::add_pointer_t<Canvas::ToolMode>>(_a[1]))); break;
        case 1: _t->editModeChanged((*reinterpret_cast<std::add_pointer_t<Canvas::EditMode>>(_a[1]))); break;
        case 2: _t->zoomScaleChanged((*reinterpret_cast<std::add_pointer_t<qreal>>(_a[1]))); break;
        case 3: _t->gridVisibilityChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 4: _t->cursorScenePositionChanged((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 5: _t->selectionChanged(); break;
        case 6: _t->setToolMode((*reinterpret_cast<std::add_pointer_t<enum ToolMode>>(_a[1]))); break;
        case 7: _t->setEditMode((*reinterpret_cast<std::add_pointer_t<EditMode>>(_a[1]))); break;
        case 8: _t->zoomIn(); break;
        case 9: _t->zoomOut(); break;
        case 10: _t->resetZoom(); break;
        case 11: _t->fitToScene(); break;
        case 12: _t->fitToSelection(); break;
        case 13: _t->setGridVisible((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 14: _t->toggleGrid(); break;
        case 15: _t->undo(); break;
        case 16: _t->redo(); break;
        case 17: _t->copy(); break;
        case 18: _t->paste(); break;
        case 19: _t->cut(); break;
        case 20: _t->deleteSelected(); break;
        case 21: _t->selectAll(); break;
        case 22: _t->bringSelectedToFront(); break;
        case 23: _t->sendSelectedToBack(); break;
        case 24: _t->clearAllItems(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Canvas::*)(Canvas::ToolMode )>(_a, &Canvas::toolModeChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Canvas::*)(Canvas::EditMode )>(_a, &Canvas::editModeChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Canvas::*)(qreal )>(_a, &Canvas::zoomScaleChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Canvas::*)(bool )>(_a, &Canvas::gridVisibilityChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Canvas::*)(const QPointF & )>(_a, &Canvas::cursorScenePositionChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (Canvas::*)()>(_a, &Canvas::selectionChanged, 5))
            return;
    }
}

const QMetaObject *Canvas::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Canvas::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6CanvasE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int Canvas::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 25)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 25;
    }
    return _id;
}

// SIGNAL 0
void Canvas::toolModeChanged(Canvas::ToolMode _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void Canvas::editModeChanged(Canvas::EditMode _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void Canvas::zoomScaleChanged(qreal _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void Canvas::gridVisibilityChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void Canvas::cursorScenePositionChanged(const QPointF & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void Canvas::selectionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
