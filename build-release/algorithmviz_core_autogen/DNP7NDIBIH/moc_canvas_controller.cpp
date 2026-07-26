/****************************************************************************
** Meta object code from reading C++ file 'canvas_controller.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/core/shape_controller/canvas_controller.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'canvas_controller.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14RubberBandItemE_t {};
} // unnamed namespace

template <> constexpr inline auto RubberBandItem::qt_create_metaobjectdata<qt_meta_tag_ZN14RubberBandItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "RubberBandItem"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<RubberBandItem, qt_meta_tag_ZN14RubberBandItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject RubberBandItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14RubberBandItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14RubberBandItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14RubberBandItemE_t>.metaTypes,
    nullptr
} };

void RubberBandItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<RubberBandItem *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *RubberBandItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RubberBandItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14RubberBandItemE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsObject::qt_metacast(_clname);
}

int RubberBandItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_tag_ZN16CanvasControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto CanvasController::qt_create_metaobjectdata<qt_meta_tag_ZN16CanvasControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "CanvasController",
        "copy",
        "",
        "paste",
        "cut",
        "deleteSelected",
        "selectAll",
        "bringSelectedToFront",
        "sendSelectedToBack",
        "clearAllItems",
        "onKeyMoveTick",
        "onShapeDestroyed",
        "object",
        "onResizeFinished",
        "Shape*",
        "target",
        "QSizeF",
        "oldSize",
        "newSize",
        "QPointF",
        "oldPos",
        "newPos",
        "onRotateFinished",
        "oldRotation",
        "newRotation",
        "onEndpointMoveFinished",
        "HandleType",
        "type",
        "oldScenePos",
        "newScenePos",
        "onConnectorEndpointMoveFinished",
        "Connector*",
        "endpoint",
        "ConnectorAnchor",
        "oldAnchor",
        "newAnchor"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'copy'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'paste'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'cut'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'deleteSelected'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectAll'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'bringSelectedToFront'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'sendSelectedToBack'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'clearAllItems'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onKeyMoveTick'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onShapeDestroyed'
        QtMocHelpers::SlotData<void(QObject *)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QObjectStar, 12 },
        }}),
        // Slot 'onResizeFinished'
        QtMocHelpers::SlotData<void(Shape *, QSizeF, QSizeF, QPointF, QPointF)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 }, { 0x80000000 | 16, 17 }, { 0x80000000 | 16, 18 }, { 0x80000000 | 19, 20 },
            { 0x80000000 | 19, 21 },
        }}),
        // Slot 'onRotateFinished'
        QtMocHelpers::SlotData<void(Shape *, qreal, qreal)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 }, { QMetaType::QReal, 23 }, { QMetaType::QReal, 24 },
        }}),
        // Slot 'onEndpointMoveFinished'
        QtMocHelpers::SlotData<void(Shape *, HandleType, QPointF, QPointF)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 }, { 0x80000000 | 26, 27 }, { 0x80000000 | 19, 28 }, { 0x80000000 | 19, 29 },
        }}),
        // Slot 'onConnectorEndpointMoveFinished'
        QtMocHelpers::SlotData<void(Connector *, HandleType, const ConnectorAnchor &, const ConnectorAnchor &)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 31, 15 }, { 0x80000000 | 26, 32 }, { 0x80000000 | 33, 34 }, { 0x80000000 | 33, 35 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CanvasController, qt_meta_tag_ZN16CanvasControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject CanvasController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16CanvasControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16CanvasControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16CanvasControllerE_t>.metaTypes,
    nullptr
} };

void CanvasController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CanvasController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->copy(); break;
        case 1: _t->paste(); break;
        case 2: _t->cut(); break;
        case 3: _t->deleteSelected(); break;
        case 4: _t->selectAll(); break;
        case 5: _t->bringSelectedToFront(); break;
        case 6: _t->sendSelectedToBack(); break;
        case 7: _t->clearAllItems(); break;
        case 8: _t->onKeyMoveTick(); break;
        case 9: _t->onShapeDestroyed((*reinterpret_cast<std::add_pointer_t<QObject*>>(_a[1]))); break;
        case 10: _t->onResizeFinished((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QSizeF>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QSizeF>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[5]))); break;
        case 11: _t->onRotateFinished((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<qreal>>(_a[3]))); break;
        case 12: _t->onEndpointMoveFinished((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<HandleType>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[4]))); break;
        case 13: _t->onConnectorEndpointMoveFinished((*reinterpret_cast<std::add_pointer_t<Connector*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<HandleType>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<ConnectorAnchor>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<ConnectorAnchor>>(_a[4]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Connector* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *CanvasController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CanvasController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16CanvasControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CanvasController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}
QT_WARNING_POP
