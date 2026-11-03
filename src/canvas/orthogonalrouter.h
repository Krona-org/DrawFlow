#pragma once

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QVector>

namespace editor {

class OrthogonalRouter {
public:
    // Построение пути с прямыми углами 90 градусов и обходом прямоугольных препятствий
    static QVector<QPointF> route(
        const QPointF &start,
        const QPointF &startNormal,
        const QPointF &end,
        const QPointF &endNormal,
        const QList<QRectF> &obstacles,
        qreal margin = 16.0
    );

private:
    static QVector<QPointF> simplifyCollinear(const QVector<QPointF> &points);
    static bool segmentIntersectsObstacles(const QPointF &p1, const QPointF &p2, const QList<QRectF> &obstacles);
};

} // namespace editor

