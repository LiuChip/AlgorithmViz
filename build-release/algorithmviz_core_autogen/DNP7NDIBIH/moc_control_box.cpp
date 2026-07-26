/****************************************************************************
** Meta object code from reading C++ file 'control_box.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/core/shape_controller/control_box.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'control_box.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10HandleItemE_t {};
} // unnamed namespace

template <> constexpr inline auto HandleItem::qt_create_metaobjectdata<qt_meta_tag_ZN10HandleItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "HandleItem",
        "handlePressed",
        "",
        "HandleItem*",
        "handle",
        "QPointF",
        "scenePos",
        "handleMoved",
        "Qt::KeyboardModifiers",
        "modifiers",
        "handleReleased"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'handlePressed'
        QtMocHelpers::SignalData<void(HandleItem *, const QPointF &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 5, 6 },
        }}),
        // Signal 'handleMoved'
        QtMocHelpers::SignalData<void(HandleItem *, const QPointF &, Qt::KeyboardModifiers)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 5, 6 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'handleReleased'
        QtMocHelpers::SignalData<void(HandleItem *, const QPointF &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 5, 6 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<HandleItem, qt_meta_tag_ZN10HandleItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject HandleItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10HandleItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10HandleItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10HandleItemE_t>.metaTypes,
    nullptr
} };

void HandleItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<HandleItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->handlePressed((*reinterpret_cast<std::add_pointer_t<HandleItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        case 1: _t->handleMoved((*reinterpret_cast<std::add_pointer_t<HandleItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<Qt::KeyboardModifiers>>(_a[3]))); break;
        case 2: _t->handleReleased((*reinterpret_cast<std::add_pointer_t<HandleItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< HandleItem* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< HandleItem* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< HandleItem* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (HandleItem::*)(HandleItem * , const QPointF & )>(_a, &HandleItem::handlePressed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (HandleItem::*)(HandleItem * , const QPointF & , Qt::KeyboardModifiers )>(_a, &HandleItem::handleMoved, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (HandleItem::*)(HandleItem * , const QPointF & )>(_a, &HandleItem::handleReleased, 2))
            return;
    }
}

const QMetaObject *HandleItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *HandleItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10HandleItemE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsObject::qt_metacast(_clname);
}

int HandleItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void HandleItem::handlePressed(HandleItem * _t1, const QPointF & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void HandleItem::handleMoved(HandleItem * _t1, const QPointF & _t2, Qt::KeyboardModifiers _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3);
}

// SIGNAL 2
void HandleItem::handleReleased(HandleItem * _t1, const QPointF & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}
namespace {
struct qt_meta_tag_ZN10ControlBoxE_t {};
} // unnamed namespace

template <> constexpr inline auto ControlBox::qt_create_metaobjectdata<qt_meta_tag_ZN10ControlBoxE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ControlBox",
        "resizeStarted",
        "",
        "Shape*",
        "target",
        "resizeFinished",
        "QSizeF",
        "oldSize",
        "newSize",
        "QPointF",
        "oldPos",
        "newPos",
        "rotateStarted",
        "rotateFinished",
        "oldAngle",
        "newAngle",
        "endpointMoveStarted",
        "HandleType",
        "endpoint",
        "endpointMoveFinished",
        "oldScenePos",
        "newScenePos",
        "connectorEndpointMoveFinished",
        "Connector*",
        "ConnectorAnchor",
        "oldAnchor",
        "newAnchor",
        "onTargetGeometryChanged",
        "onTargetDestroyed",
        "onTargetLockedChanged",
        "locked",
        "onHandlePressed",
        "HandleItem*",
        "handle",
        "scenePos",
        "onHandleMoved",
        "Qt::KeyboardModifiers",
        "modifiers",
        "onHandleReleased"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'resizeStarted'
        QtMocHelpers::SignalData<void(Shape *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'resizeFinished'
        QtMocHelpers::SignalData<void(Shape *, const QSizeF &, const QSizeF &, const QPointF &, const QPointF &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 6, 7 }, { 0x80000000 | 6, 8 }, { 0x80000000 | 9, 10 },
            { 0x80000000 | 9, 11 },
        }}),
        // Signal 'rotateStarted'
        QtMocHelpers::SignalData<void(Shape *)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'rotateFinished'
        QtMocHelpers::SignalData<void(Shape *, qreal, qreal)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::QReal, 14 }, { QMetaType::QReal, 15 },
        }}),
        // Signal 'endpointMoveStarted'
        QtMocHelpers::SignalData<void(Shape *, HandleType)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 17, 18 },
        }}),
        // Signal 'endpointMoveFinished'
        QtMocHelpers::SignalData<void(Shape *, HandleType, const QPointF &, const QPointF &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 17, 18 }, { 0x80000000 | 9, 20 }, { 0x80000000 | 9, 21 },
        }}),
        // Signal 'connectorEndpointMoveFinished'
        QtMocHelpers::SignalData<void(Connector *, HandleType, const ConnectorAnchor &, const ConnectorAnchor &)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 23, 4 }, { 0x80000000 | 17, 18 }, { 0x80000000 | 24, 25 }, { 0x80000000 | 24, 26 },
        }}),
        // Slot 'onTargetGeometryChanged'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTargetDestroyed'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTargetLockedChanged'
        QtMocHelpers::SlotData<void(bool)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 30 },
        }}),
        // Slot 'onHandlePressed'
        QtMocHelpers::SlotData<void(HandleItem *, const QPointF &)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 32, 33 }, { 0x80000000 | 9, 34 },
        }}),
        // Slot 'onHandleMoved'
        QtMocHelpers::SlotData<void(HandleItem *, const QPointF &, Qt::KeyboardModifiers)>(35, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 32, 33 }, { 0x80000000 | 9, 34 }, { 0x80000000 | 36, 37 },
        }}),
        // Slot 'onHandleReleased'
        QtMocHelpers::SlotData<void(HandleItem *, const QPointF &)>(38, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 32, 33 }, { 0x80000000 | 9, 34 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ControlBox, qt_meta_tag_ZN10ControlBoxE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ControlBox::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ControlBoxE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ControlBoxE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10ControlBoxE_t>.metaTypes,
    nullptr
} };

void ControlBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ControlBox *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->resizeStarted((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1]))); break;
        case 1: _t->resizeFinished((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QSizeF>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QSizeF>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[5]))); break;
        case 2: _t->rotateStarted((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1]))); break;
        case 3: _t->rotateFinished((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<qreal>>(_a[3]))); break;
        case 4: _t->endpointMoveStarted((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<HandleType>>(_a[2]))); break;
        case 5: _t->endpointMoveFinished((*reinterpret_cast<std::add_pointer_t<Shape*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<HandleType>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[4]))); break;
        case 6: _t->connectorEndpointMoveFinished((*reinterpret_cast<std::add_pointer_t<Connector*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<HandleType>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<ConnectorAnchor>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<ConnectorAnchor>>(_a[4]))); break;
        case 7: _t->onTargetGeometryChanged(); break;
        case 8: _t->onTargetDestroyed(); break;
        case 9: _t->onTargetLockedChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 10: _t->onHandlePressed((*reinterpret_cast<std::add_pointer_t<HandleItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        case 11: _t->onHandleMoved((*reinterpret_cast<std::add_pointer_t<HandleItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<Qt::KeyboardModifiers>>(_a[3]))); break;
        case 12: _t->onHandleReleased((*reinterpret_cast<std::add_pointer_t<HandleItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Shape* >(); break;
            }
            break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Connector* >(); break;
            }
            break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< HandleItem* >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< HandleItem* >(); break;
            }
            break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< HandleItem* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ControlBox::*)(Shape * )>(_a, &ControlBox::resizeStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ControlBox::*)(Shape * , const QSizeF & , const QSizeF & , const QPointF & , const QPointF & )>(_a, &ControlBox::resizeFinished, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ControlBox::*)(Shape * )>(_a, &ControlBox::rotateStarted, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ControlBox::*)(Shape * , qreal , qreal )>(_a, &ControlBox::rotateFinished, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ControlBox::*)(Shape * , HandleType )>(_a, &ControlBox::endpointMoveStarted, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ControlBox::*)(Shape * , HandleType , const QPointF & , const QPointF & )>(_a, &ControlBox::endpointMoveFinished, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ControlBox::*)(Connector * , HandleType , const ConnectorAnchor & , const ConnectorAnchor & )>(_a, &ControlBox::connectorEndpointMoveFinished, 6))
            return;
    }
}

const QMetaObject *ControlBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ControlBox::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ControlBoxE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsObject::qt_metacast(_clname);
}

int ControlBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void ControlBox::resizeStarted(Shape * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ControlBox::resizeFinished(Shape * _t1, const QSizeF & _t2, const QSizeF & _t3, const QPointF & _t4, const QPointF & _t5)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3, _t4, _t5);
}

// SIGNAL 2
void ControlBox::rotateStarted(Shape * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void ControlBox::rotateFinished(Shape * _t1, qreal _t2, qreal _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2, _t3);
}

// SIGNAL 4
void ControlBox::endpointMoveStarted(Shape * _t1, HandleType _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void ControlBox::endpointMoveFinished(Shape * _t1, HandleType _t2, const QPointF & _t3, const QPointF & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 6
void ControlBox::connectorEndpointMoveFinished(Connector * _t1, HandleType _t2, const ConnectorAnchor & _t3, const ConnectorAnchor & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3, _t4);
}
QT_WARNING_POP
