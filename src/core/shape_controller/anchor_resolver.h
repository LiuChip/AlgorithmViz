#ifndef ANCHOR_RESOLVER_H
#define ANCHOR_RESOLVER_H

#include "shapes/connector/connector_anchor.h"

#include <QPointF>
#include <QGraphicsScene>
#include <QList>

class AnchorResolver {
public:
    struct ResolveOptions {
        QPointF scenePoint;
        QGraphicsScene* scene = nullptr;
        bool ctrlPressed = false;
        qreal screenTolerancePx = 12.0;
        qreal viewScale = 1.0;
        ConnectableShape* ignoreShape = nullptr;
    };
    static ConnectorAnchor resolve(const ResolveOptions& options);
    static bool isNearBoundary(ConnectableShape* target,
                               const QPointF& scenePoint,
                               qreal sceneTolerance,
                               qreal* outAngle = nullptr);
};

#endif // ANCHOR_RESOLVER_H
