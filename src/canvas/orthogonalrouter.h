#pragma once

#include <QLineF>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QVector>

namespace editor {

class OrthogonalRouter {
public:
    // Построение пути с прямыми углами 90 градусов, обходом препятствий и разведением параллельных линий
    static QVector<QPointF> route(
        const QPointF &start,
        const QPointF &startNormal,
        const QPointF &end,
        const QPointF &endNormal,
        const QList<QRectF> &obstacles,
        qreal margin = 20.0,
        qreal laneOffset = 0.0,
        const QList<QLineF> &existingSegments = {}
    );

    // Резиновая адаптация маршрута с ручной настройкой при перемещении блоков (Rubber-banding)
    static QVector<QPointF> adaptManualRoute(
        const QVector<QPointF> &currentPoints,
        const QPointF &newStart,
        const QPointF &startNormal,
        const QPointF &newEnd,
        const QPointF &endNormal,
        const QList<QRectF> &obstacles,
        qreal minStub = 12.0
    );

    // Проверка пересечения ломаной линии с препятствиями
    static bool pathIntersectsObstacles(const QVector<QPointF> &pts, const QList<QRectF> &obstacles);
    static bool segmentIntersectsObstacles(const QPointF &p1, const QPointF &p2, const QList<QRectF> &obstacles, qreal inflation = -1.5);

    // Проверка наложения (слияния) сегментов разных коннекторов
    static bool segmentsOverlapCollinear(const QPointF &a1, const QPointF &a2,
                                         const QPointF &b1, const QPointF &b2,
                                         qreal minSpacing = 13.0);
    static bool segmentsCrossPerpendicular(const QPointF &a1, const QPointF &a2,
                                           const QPointF &b1, const QPointF &b2);
    static bool pathCollidesWithSegments(const QVector<QPointF> &pts,
                                         const QList<QLineF> &existingSegments,
                                         qreal minSpacing = 13.0);

    static QVector<QPointF> simplifyCollinear(const QVector<QPointF> &points);

private:
    static QVector<QPointF> tryIdealFormulaRoute(
        const QPointF &start,
        const QPointF &startNormal,
        const QPointF &end,
        const QPointF &endNormal,
        const QList<QRectF> &obstacles,
        qreal margin,
        qreal laneOffset,
        const QList<QLineF> &existingSegments
    );

    static QVector<QPointF> nudgeCollidingSegments(
        const QVector<QPointF> &pts,
        const QList<QRectF> &obstacles,
        const QList<QLineF> &existingSegments,
        qreal margin
    );
};

} // namespace editor
