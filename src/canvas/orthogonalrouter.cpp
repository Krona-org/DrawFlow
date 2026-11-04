#include "orthogonalrouter.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <vector>

namespace editor {

bool OrthogonalRouter::segmentIntersectsObstacles(
    const QPointF &p1,
    const QPointF &p2,
    const QList<QRectF> &obstacles,
    qreal inflation
) {
    qreal xMin = std::min(p1.x(), p2.x());
    qreal xMax = std::max(p1.x(), p2.x());
    qreal yMin = std::min(p1.y(), p2.y());
    qreal yMax = std::max(p1.y(), p2.y());

    bool isHorizontal = std::abs(p1.y() - p2.y()) < 1e-4;
    bool isVertical = std::abs(p1.x() - p2.x()) < 1e-4;

    for (const QRectF &obs : obstacles) {
        // Проверяем, начинается или заканчивается ли сегмент непосредственно на порту этого блока
        QRectF portZone = obs.adjusted(-2.5, -2.5, 2.5, 2.5);
        bool touchesPort = portZone.contains(p1) || portZone.contains(p2);

        // Для крайних сегментов у своего блока слегка сжимаем рамку (-1.5px),
        // а для всех промежуточных сегментов мимо чужих блоков и плашек [ 1 ] даем положительный зазор
        qreal effInflation = touchesPort ? -1.5 : std::max(inflation, 4.0);
        QRectF core = obs.adjusted(-effInflation, -effInflation, effInflation, effInflation);
        if (core.width() <= 0.0 || core.height() <= 0.0) continue;

        if (isHorizontal) {
            if (p1.y() >= core.top() && p1.y() <= core.bottom()) {
                if (std::max(xMin, core.left()) <= std::min(xMax, core.right())) {
                    return true;
                }
            }
        } else if (isVertical) {
            if (p1.x() >= core.left() && p1.x() <= core.right()) {
                if (std::max(yMin, core.top()) <= std::min(yMax, core.bottom())) {
                    return true;
                }
            }
        } else {
            if (core.intersects(QRectF(xMin, yMin, qMax(1.0, xMax - xMin), qMax(1.0, yMax - yMin)))) {
                return true;
            }
        }
    }
    return false;
}

bool OrthogonalRouter::pathIntersectsObstacles(const QVector<QPointF> &pts, const QList<QRectF> &obstacles) {
    for (int i = 0; i < pts.size() - 1; ++i) {
        if (segmentIntersectsObstacles(pts[i], pts[i + 1], obstacles)) {
            return true;
        }
    }
    return false;
}

bool OrthogonalRouter::segmentsOverlapCollinear(
    const QPointF &a1,
    const QPointF &a2,
    const QPointF &b1,
    const QPointF &b2,
    qreal minSpacing
) {
    qreal lenA = std::hypot(a2.x() - a1.x(), a2.y() - a1.y());
    qreal lenB = std::hypot(b2.x() - b1.x(), b2.y() - b1.y());
    if (lenA < 1.0 || lenB < 1.0) return false;

    bool aHoriz = std::abs(a1.y() - a2.y()) < 1.0;
    bool bHoriz = std::abs(b1.y() - b2.y()) < 1.0;
    if (aHoriz && bHoriz) {
        if (std::abs(a1.y() - b1.y()) < minSpacing) {
            qreal startX = std::max(std::min(a1.x(), a2.x()), std::min(b1.x(), b2.x()));
            qreal endX = std::min(std::max(a1.x(), a2.x()), std::max(b1.x(), b2.x()));
            if (endX - startX > 2.0) {
                return true;
            }
        }
        return false;
    }

    bool aVert = std::abs(a1.x() - a2.x()) < 1.0;
    bool bVert = std::abs(b1.x() - b2.x()) < 1.0;
    if (aVert && bVert) {
        if (std::abs(a1.x() - b1.x()) < minSpacing) {
            qreal startY = std::max(std::min(a1.y(), a2.y()), std::min(b1.y(), b2.y()));
            qreal endY = std::min(std::max(a1.y(), a2.y()), std::max(b1.y(), b2.y()));
            if (endY - startY > 2.0) {
                return true;
            }
        }
        return false;
    }

    return false;
}

bool OrthogonalRouter::segmentsCrossPerpendicular(
    const QPointF &a1,
    const QPointF &a2,
    const QPointF &b1,
    const QPointF &b2
) {
    bool aHoriz = std::abs(a1.y() - a2.y()) < 1.0;
    bool bVert = std::abs(b1.x() - b2.x()) < 1.0;
    if (aHoriz && bVert) {
        qreal axMin = std::min(a1.x(), a2.x()) + 1.5;
        qreal axMax = std::max(a1.x(), a2.x()) - 1.5;
        qreal byMin = std::min(b1.y(), b2.y()) + 1.5;
        qreal byMax = std::max(b1.y(), b2.y()) - 1.5;
        return (b1.x() > axMin && b1.x() < axMax && a1.y() > byMin && a1.y() < byMax);
    }

    bool aVert = std::abs(a1.x() - a2.x()) < 1.0;
    bool bHoriz = std::abs(b1.y() - b2.y()) < 1.0;
    if (aVert && bHoriz) {
        qreal ayMin = std::min(a1.y(), a2.y()) + 1.5;
        qreal ayMax = std::max(a1.y(), a2.y()) - 1.5;
        qreal bxMin = std::min(b1.x(), b2.x()) + 1.5;
        qreal bxMax = std::max(b1.x(), b2.x()) - 1.5;
        return (a1.x() > bxMin && a1.x() < bxMax && b1.y() > ayMin && b1.y() < ayMax);
    }

    return false;
}

bool OrthogonalRouter::pathCollidesWithSegments(
    const QVector<QPointF> &pts,
    const QList<QLineF> &existingSegments,
    qreal minSpacing
) {
    if (existingSegments.isEmpty() || pts.size() < 2) return false;

    for (int i = 0; i < pts.size() - 1; ++i) {
        const QPointF &p1 = pts[i];
        const QPointF &p2 = pts[i + 1];
        bool isTerminal = (i == 0 || i == pts.size() - 2);
        qreal spacing = isTerminal ? std::min(minSpacing, 9.0) : minSpacing;

        for (const QLineF &seg : existingSegments) {
            // Если два крайних сегмента выходят из одного и того же порта, это допустимо
            if (isTerminal) {
                if ((p1 - seg.p1()).manhattanLength() < 1.5 ||
                    (p1 - seg.p2()).manhattanLength() < 1.5 ||
                    (p2 - seg.p1()).manhattanLength() < 1.5 ||
                    (p2 - seg.p2()).manhattanLength() < 1.5) {
                    continue;
                }
            }
            if (segmentsOverlapCollinear(p1, p2, seg.p1(), seg.p2(), spacing)) {
                return true;
            }
        }
    }
    return false;
}

QVector<QPointF> OrthogonalRouter::simplifyCollinear(const QVector<QPointF> &pts) {
    if (pts.size() <= 2) return pts;

    QVector<QPointF> result;
    result.append(pts.first());

    for (int i = 1; i < pts.size() - 1; ++i) {
        const QPointF &prev = result.last();
        const QPointF &curr = pts[i];
        const QPointF &next = pts[i + 1];

        if (std::hypot(curr.x() - prev.x(), curr.y() - prev.y()) < 1e-3) {
            continue;
        }

        bool collinX = std::abs(prev.x() - curr.x()) < 1e-3 && std::abs(curr.x() - next.x()) < 1e-3;
        bool collinY = std::abs(prev.y() - curr.y()) < 1e-3 && std::abs(curr.y() - next.y()) < 1e-3;

        if (!collinX && !collinY) {
            result.append(curr);
        }
    }

    if (result.isEmpty() || std::hypot(pts.last().x() - result.last().x(), pts.last().y() - result.last().y()) >= 1e-3) {
        result.append(pts.last());
    }

    if (result.size() < 2) {
        if (pts.size() >= 2) {
            return { pts.first(), pts.last() };
        } else if (!pts.isEmpty()) {
            return { pts.first(), pts.first() + QPointF(20.0, 0.0) };
        }
    }

    return result;
}

QVector<QPointF> OrthogonalRouter::adaptManualRoute(
    const QVector<QPointF> &currentPoints,
    const QPointF &newStart,
    const QPointF &startNormal,
    const QPointF &newEnd,
    const QPointF &endNormal,
    const QList<QRectF> &obstacles,
    qreal minStub
) {
    if (currentPoints.size() < 4) return {};

    QVector<QPointF> pts = currentPoints;
    int n = pts.size();

    // Подстраиваем первый сегмент под новое положение начального порта
    bool firstHoriz = std::abs(pts[0].y() - pts[1].y()) < 1.0;
    pts[0] = newStart;
    if (firstHoriz) {
        pts[1].setY(newStart.y());
    } else {
        pts[1].setX(newStart.x());
    }

    // Подстраиваем последний сегмент под новое положение конечного порта
    bool lastHoriz = std::abs(pts[n - 1].y() - pts[n - 2].y()) < 1.0;
    pts[n - 1] = newEnd;
    if (lastHoriz) {
        pts[n - 2].setY(newEnd.y());
    } else {
        pts[n - 2].setX(newEnd.x());
    }

    // Проверяем, что выход и вход в порты не вывернулись наизнанку при перемещении блоков
    if (startNormal.manhattanLength() > 0.1) {
        QPointF dStart = pts[1] - pts[0];
        qreal dotStart = dStart.x() * startNormal.x() + dStart.y() * startNormal.y();
        if (dotStart < minStub) return {};
    }
    if (endNormal.manhattanLength() > 0.1) {
        QPointF dEnd = pts[n - 2] - pts[n - 1];
        qreal dotEnd = dEnd.x() * endNormal.x() + dEnd.y() * endNormal.y();
        if (dotEnd < minStub) return {};
    }

    if (pathIntersectsObstacles(pts, obstacles)) {
        return {};
    }

    return pts;
}

QVector<QPointF> OrthogonalRouter::tryIdealFormulaRoute(
    const QPointF &start,
    const QPointF &startNormal,
    const QPointF &end,
    const QPointF &endNormal,
    const QList<QRectF> &obstacles,
    qreal margin,
    qreal laneOffset,
    const QList<QLineF> &existingSegments
) {
    QPointF n1 = startNormal;
    QPointF n2 = endNormal;

    bool isH1 = std::abs(n1.x()) > 0.5;
    bool isV1 = std::abs(n1.y()) > 0.5;
    bool isH2 = std::abs(n2.x()) > 0.5;
    bool isV2 = std::abs(n2.y()) > 0.5;

    const qreal shifts[] = {0.0, 15.0, -15.0, 30.0, -30.0, 45.0, -45.0, 60.0, -60.0, 75.0, -75.0};

    // СЛУЧАЙ 1: Оба порта горизонтальные (H-V-H Z-изгиб или П-образный C-изгиб)
    if (isH1 && isH2) {
        // Противоположные направления (навстречу друг другу): Right -> Left
        if (n1.x() > 0 && n2.x() < 0 && end.x() > start.x() + margin * 1.6) {
            qreal baseMidX = (start.x() + end.x()) * 0.5 + laneOffset;
            for (qreal s : shifts) {
                qreal midX = std::max(start.x() + 14.0, std::min(end.x() - 14.0, baseMidX + s));
                QVector<QPointF> cand = { start, QPointF(midX, start.y()), QPointF(midX, end.y()), end };
                if (!pathIntersectsObstacles(cand, obstacles) &&
                    !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                    return cand;
                }
            }
        }
        // Противоположные направления (навстречу друг другу): Left -> Right
        if (n1.x() < 0 && n2.x() > 0 && end.x() < start.x() - margin * 1.6) {
            qreal baseMidX = (start.x() + end.x()) * 0.5 + laneOffset;
            for (qreal s : shifts) {
                qreal midX = std::max(end.x() + 14.0, std::min(start.x() - 14.0, baseMidX + s));
                QVector<QPointF> cand = { start, QPointF(midX, start.y()), QPointF(midX, end.y()), end };
                if (!pathIntersectsObstacles(cand, obstacles) &&
                    !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                    return cand;
                }
            }
        }
        // Одинаковые направления: оба выходят вправо (C-изгиб справа)
        if (n1.x() > 0 && n2.x() > 0) {
            qreal minClearX = std::max(start.x(), end.x()) + 15.0;
            qreal baseClearX = std::max(minClearX, std::max(start.x(), end.x()) + margin * 1.5 + laneOffset);
            for (int k = 0; k < 8; ++k) {
                qreal clearX = baseClearX + k * 15.0;
                QVector<QPointF> cand = { start, QPointF(clearX, start.y()), QPointF(clearX, end.y()), end };
                if (!pathIntersectsObstacles(cand, obstacles) &&
                    !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                    return cand;
                }
            }
        }
        // Одинаковые направления: оба выходят влево (C-изгиб слева)
        if (n1.x() < 0 && n2.x() < 0) {
            qreal maxClearX = std::min(start.x(), end.x()) - 15.0;
            qreal baseClearX = std::min(maxClearX, std::min(start.x(), end.x()) - margin * 1.5 + laneOffset);
            for (int k = 0; k < 8; ++k) {
                qreal clearX = baseClearX - k * 15.0;
                QVector<QPointF> cand = { start, QPointF(clearX, start.y()), QPointF(clearX, end.y()), end };
                if (!pathIntersectsObstacles(cand, obstacles) &&
                    !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                    return cand;
                }
            }
        }
    }

    // СЛУЧАЙ 2: Оба порта вертикальные (V-H-V Z-изгиб или C-изгиб)
    if (isV1 && isV2) {
        if (n1.y() > 0 && n2.y() < 0 && end.y() > start.y() + margin * 1.6) {
            qreal baseMidY = (start.y() + end.y()) * 0.5 + laneOffset;
            for (qreal s : shifts) {
                qreal midY = std::max(start.y() + 14.0, std::min(end.y() - 14.0, baseMidY + s));
                QVector<QPointF> cand = { start, QPointF(start.x(), midY), QPointF(end.x(), midY), end };
                if (!pathIntersectsObstacles(cand, obstacles) &&
                    !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                    return cand;
                }
            }
        }
        if (n1.y() < 0 && n2.y() > 0 && end.y() < start.y() - margin * 1.6) {
            qreal baseMidY = (start.y() + end.y()) * 0.5 + laneOffset;
            for (qreal s : shifts) {
                qreal midY = std::max(end.y() + 14.0, std::min(start.y() - 14.0, baseMidY + s));
                QVector<QPointF> cand = { start, QPointF(start.x(), midY), QPointF(end.x(), midY), end };
                if (!pathIntersectsObstacles(cand, obstacles) &&
                    !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                    return cand;
                }
            }
        }
        if (n1.y() > 0 && n2.y() > 0) {
            qreal minClearY = std::max(start.y(), end.y()) + 15.0;
            qreal baseClearY = std::max(minClearY, std::max(start.y(), end.y()) + margin * 1.5 + laneOffset);
            for (int k = 0; k < 8; ++k) {
                qreal clearY = baseClearY + k * 15.0;
                QVector<QPointF> cand = { start, QPointF(start.x(), clearY), QPointF(end.x(), clearY), end };
                if (!pathIntersectsObstacles(cand, obstacles) &&
                    !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                    return cand;
                }
            }
        }
        if (n1.y() < 0 && n2.y() < 0) {
            qreal maxClearY = std::min(start.y(), end.y()) - 15.0;
            qreal baseClearY = std::min(maxClearY, std::min(start.y(), end.y()) - margin * 1.5 + laneOffset);
            for (int k = 0; k < 8; ++k) {
                qreal clearY = baseClearY - k * 15.0;
                QVector<QPointF> cand = { start, QPointF(start.x(), clearY), QPointF(end.x(), clearY), end };
                if (!pathIntersectsObstacles(cand, obstacles) &&
                    !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                    return cand;
                }
            }
        }
    }

    // СЛУЧАЙ 3: Один горизонтальный, один вертикальный (L-изгиб под 90 градусов)
    if (isH1 && isV2) {
        bool canExitDirect = (end.x() - start.x()) * n1.x() > 0.0;
        bool canEnterDirect = (start.y() - end.y()) * n2.y() > 0.0;

        if (canExitDirect && canEnterDirect) {
            QVector<QPointF> cand = { start, QPointF(end.x(), start.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles) &&
                !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                return cand;
            }
        }
    }

    if (isV1 && isH2) {
        bool canExitDirect = (end.y() - start.y()) * n1.y() > 0.0;
        bool canEnterDirect = (start.x() - end.x()) * n2.x() > 0.0;

        if (canExitDirect && canEnterDirect) {
            QVector<QPointF> cand = { start, QPointF(start.x(), end.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles) &&
                !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                return cand;
            }
        }
    }

    // СЛУЧАЙ 4: Конечный порт не зафиксирован (ведение линии мышью в пустую область)
    if (isH1 && !isH2 && !isV2) {
        if ((end.x() - start.x()) * n1.x() > margin) {
            qreal midX = (start.x() + end.x()) * 0.5 + laneOffset;
            QVector<QPointF> cand = { start, QPointF(midX, start.y()), QPointF(midX, end.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles) &&
                !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                return cand;
            }
        }
    }

    if (isV1 && !isH2 && !isV2) {
        if ((end.y() - start.y()) * n1.y() > margin) {
            qreal midY = (start.y() + end.y()) * 0.5 + laneOffset;
            QVector<QPointF> cand = { start, QPointF(start.x(), midY), QPointF(end.x(), midY), end };
            if (!pathIntersectsObstacles(cand, obstacles) &&
                !pathCollidesWithSegments(cand, existingSegments, 13.0)) {
                return cand;
            }
        }
    }

    return {};
}

QVector<QPointF> OrthogonalRouter::nudgeCollidingSegments(
    const QVector<QPointF> &pts,
    const QList<QRectF> &obstacles,
    const QList<QLineF> &existingSegments,
    qreal margin
) {
    if (existingSegments.isEmpty() || pts.size() < 4) return pts;

    QVector<QPointF> res = pts;
    const qreal deltas[] = {15.0, -15.0, 30.0, -30.0, 45.0, -45.0, 60.0, -60.0, 75.0, -75.0};
    const qreal minStub = std::min(12.0, margin * 0.6);

    for (int i = 1; i <= res.size() - 3; ++i) {
        QPointF a = res[i];
        QPointF b = res[i + 1];
        bool isVert = std::abs(a.x() - b.x()) < 1.0;
        bool isHoriz = std::abs(a.y() - b.y()) < 1.0;
        if (!isVert && !isHoriz) continue;

        auto hasCollision = [&](const QPointF &p1, const QPointF &p2) {
            for (const QLineF &seg : existingSegments) {
                if (segmentsOverlapCollinear(p1, p2, seg.p1(), seg.p2(), 13.0)) {
                    return true;
                }
            }
            return false;
        };

        if (!hasCollision(a, b)) continue;

        for (qreal d : deltas) {
            QVector<QPointF> trial = res;
            if (isVert) {
                trial[i].setX(a.x() + d);
                trial[i + 1].setX(b.x() + d);
            } else {
                trial[i].setY(a.y() + d);
                trial[i + 1].setY(b.y() + d);
            }

            // Не допускаем слишком коротких или вывернутых крайних сегментов у портов
            if (i == 1) {
                QPointF origDir = res[1] - res[0];
                QPointF newDir = trial[1] - trial[0];
                if (origDir.x() * newDir.x() + origDir.y() * newDir.y() <= 0.0) continue;
                if (std::hypot(newDir.x(), newDir.y()) < minStub) continue;
            }
            if (i == res.size() - 3) {
                int last = res.size() - 1;
                QPointF origDir = res[last - 1] - res[last];
                QPointF newDir = trial[last - 1] - trial[last];
                if (origDir.x() * newDir.x() + origDir.y() * newDir.y() <= 0.0) continue;
                if (std::hypot(newDir.x(), newDir.y()) < minStub) continue;
            }

            if (!hasCollision(trial[i], trial[i + 1]) &&
                !segmentIntersectsObstacles(trial[i - 1], trial[i], obstacles) &&
                !segmentIntersectsObstacles(trial[i], trial[i + 1], obstacles) &&
                !segmentIntersectsObstacles(trial[i + 1], trial[i + 2], obstacles)) {
                res = trial;
                break;
            }
        }
    }

    return res;
}

QVector<QPointF> OrthogonalRouter::route(
    const QPointF &start,
    const QPointF &startNormal,
    const QPointF &end,
    const QPointF &endNormal,
    const QList<QRectF> &obstacles,
    qreal margin,
    qreal laneOffset,
    const QList<QLineF> &existingSegments
) {
    if (std::hypot(start.x() - end.x(), start.y() - end.y()) < 2.0) {
        QPointF norm = (startNormal.manhattanLength() > 0.1) ? startNormal : QPointF(1.0, 0.0);
        return { start, start + norm * margin };
    }

    // 0. Пространственная фильтрация препятствий и чужих сегментов вокруг активного коридора (Bounding Box Culling)
    QRectF activeBox = QRectF(start, end).normalized().adjusted(-240.0, -200.0, 240.0, 200.0);
    for (int pass = 0; pass < 2; ++pass) {
        for (const QRectF &obs : obstacles) {
            if (activeBox.intersects(obs)) {
                activeBox = activeBox.united(obs.adjusted(-90.0, -90.0, 90.0, 90.0));
            }
        }
    }

    QList<QRectF> localObstacles;
    localObstacles.reserve(obstacles.size());
    for (const QRectF &obs : obstacles) {
        if (activeBox.intersects(obs)) {
            localObstacles.append(obs);
        }
    }

    QList<QLineF> localSegments;
    localSegments.reserve(existingSegments.size());
    for (const QLineF &seg : existingSegments) {
        QRectF segBox = QRectF(seg.p1(), seg.p2()).normalized().adjusted(-16.0, -16.0, 16.0, 16.0);
        if (activeBox.intersects(segBox)) {
            localSegments.append(seg);
        }
    }

    // 1. Сначала пробуем идеальные формулы с учетом смещения полосы laneOffset и локальных сегментов
    QVector<QPointF> ideal = tryIdealFormulaRoute(
        start, startNormal, end, endNormal, localObstacles, margin, laneOffset, localSegments);
    if (!ideal.isEmpty()) {
        return simplifyCollinear(ideal);
    }

    // 2. Быстрый A* поиск на плоских массивах (без аллокаций std::unordered_map)
    const qreal minStub = 15.0;
    qreal startLen = std::hypot(startNormal.x(), startNormal.y());
    QPointF pStart = (startLen > 0.1) ? (start + startNormal * minStub) : start;

    qreal endLen = std::hypot(endNormal.x(), endNormal.y());
    QPointF pEnd = (endLen > 0.1) ? (end + endNormal * minStub) : end;

    std::vector<qreal> xs;
    std::vector<qreal> ys;
    xs.reserve(32 + localObstacles.size() * 6);
    ys.reserve(32 + localObstacles.size() * 6);

    qreal midX = (pStart.x() + pEnd.x()) * 0.5 + laneOffset;
    qreal midY = (pStart.y() + pEnd.y()) * 0.5 + laneOffset;

    xs.push_back(start.x());
    xs.push_back(pStart.x());
    xs.push_back(end.x());
    xs.push_back(pEnd.x());
    xs.push_back(midX);

    ys.push_back(start.y());
    ys.push_back(pStart.y());
    ys.push_back(end.y());
    ys.push_back(pEnd.y());
    ys.push_back(midY);

    const qreal trackStep = 15.0;
    for (int k = 1; k <= 3; ++k) {
        xs.push_back(midX + k * trackStep);
        xs.push_back(midX - k * trackStep);
        ys.push_back(midY + k * trackStep);
        ys.push_back(midY - k * trackStep);

        if (std::abs(startNormal.x()) > 0.5) {
            xs.push_back(start.x() + startNormal.x() * (minStub + k * trackStep));
        }
        if (std::abs(endNormal.x()) > 0.5) {
            xs.push_back(end.x() + endNormal.x() * (minStub + k * trackStep));
        }
        if (std::abs(startNormal.y()) > 0.5) {
            ys.push_back(start.y() + startNormal.y() * (minStub + k * trackStep));
        }
        if (std::abs(endNormal.y()) > 0.5) {
            ys.push_back(end.y() + endNormal.y() * (minStub + k * trackStep));
        }
    }

    for (const QRectF &obs : localObstacles) {
        for (int k = 0; k < 3; ++k) {
            qreal dist = margin + k * trackStep;
            xs.push_back(obs.left() - dist);
            xs.push_back(obs.right() + dist);
            ys.push_back(obs.top() - dist);
            ys.push_back(obs.bottom() + dist);
        }
    }

    auto deduplicate = [](std::vector<qreal> &vec) {
        std::sort(vec.begin(), vec.end());
        std::vector<qreal> clean;
        clean.reserve(vec.size());
        for (qreal v : vec) {
            if (clean.empty() || std::abs(clean.back() - v) > 3.0) {
                clean.push_back(v);
            }
        }
        vec = std::move(clean);
    };

    deduplicate(xs);
    deduplicate(ys);

    int W = static_cast<int>(xs.size());
    int H = static_cast<int>(ys.size());

    if (W < 2 || H < 2) {
        return { start, end };
    }

    auto findClosestNode = [&](const QPointF &pt) -> int {
        int bestIdx = 0;
        qreal bestDist = 1e18;
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                qreal d = std::hypot(xs[x] - pt.x(), ys[y] - pt.y());
                if (d < bestDist) {
                    bestDist = d;
                    bestIdx = y * W + x;
                }
            }
        }
        return bestIdx;
    };

    int startNode = findClosestNode(pStart);
    int targetNode = findClosestNode(pEnd);

    struct State {
        int node;
        int dir;
        qreal cost;
        qreal priority;

        bool operator>(const State &other) const {
            return priority > other.priority;
        }
    };

    const int totalStates = W * H * 4;
    std::vector<qreal> gScore(totalStates, 1e18);
    std::vector<int> cameFrom(totalStates, -1);
    std::priority_queue<State, std::vector<State>, std::greater<State>> openSet;

    auto encode = [](int node, int dir) { return node * 4 + dir; };

    int initDir = 0;
    if (std::abs(startNormal.x()) > 0.5) initDir = 1;
    else if (std::abs(startNormal.y()) > 0.5) initDir = 2;

    int startState = encode(startNode, initDir);
    gScore[startState] = 0.0;
    openSet.push({startNode, initDir, 0.0, 0.0});

    int targetX = targetNode % W;
    int targetY = targetNode / W;
    QPointF targetPt(xs[targetX], ys[targetY]);

    const qreal turnPenalty = 65.0;
    int bestEndState = -1;

    while (!openSet.empty()) {
        State current = openSet.top();
        openSet.pop();

        int curKey = encode(current.node, current.dir);
        if (current.cost > gScore[curKey] + 1e-4) {
            continue;
        }

        if (current.node == targetNode) {
            bestEndState = curKey;
            break;
        }

        int cx = current.node % W;
        int cy = current.node / W;
        QPointF cPos(xs[cx], ys[cy]);

        const int dx[4] = {-1, 1, 0, 0};
        const int dy[4] = {0, 0, -1, 1};
        const int dirs[4] = {1, 1, 2, 2}; // 1 = horiz, 2 = vert

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;

            if (current.node == startNode && startLen > 0.1) {
                if (dx[i] * startNormal.x() + dy[i] * startNormal.y() < -0.1) {
                    continue;
                }
            }

            QPointF nPos(xs[nx], ys[ny]);

            if (segmentIntersectsObstacles(cPos, nPos, localObstacles)) {
                continue;
            }

            qreal dist = std::hypot(nPos.x() - cPos.x(), nPos.y() - cPos.y());
            qreal extraTurn = (current.dir != 0 && current.dir != dirs[i]) ? turnPenalty : 0.0;

            int nNode = ny * W + nx;
            if (nNode == targetNode && endLen > 0.1) {
                int endDir = (std::abs(endNormal.x()) > 0.5) ? 1 : 2;
                if (dirs[i] != endDir) {
                    extraTurn += turnPenalty;
                }
            }

            qreal collisionPenalty = 0.0;
            for (const QLineF &seg : localSegments) {
                if (segmentsOverlapCollinear(cPos, nPos, seg.p1(), seg.p2(), 13.0)) {
                    collisionPenalty += 3000.0;
                } else if (segmentsCrossPerpendicular(cPos, nPos, seg.p1(), seg.p2())) {
                    collisionPenalty += 55.0;
                }
            }

            qreal nonCenterPenalty = (std::abs(nPos.x() - midX) < 1.0 || std::abs(nPos.y() - midY) < 1.0) ? 0.0 : 4.0;

            qreal tentG = current.cost + dist + extraTurn + collisionPenalty + nonCenterPenalty;

            int nKey = encode(nNode, dirs[i]);

            if (tentG < gScore[nKey]) {
                gScore[nKey] = tentG;
                cameFrom[nKey] = curKey;
                qreal h = std::abs(targetPt.x() - nPos.x()) + std::abs(targetPt.y() - nPos.y());
                openSet.push({nNode, dirs[i], tentG, tentG + h});
            }
        }
    }

    QVector<QPointF> path;
    if (bestEndState != -1) {
        int curr = bestEndState;
        int maxSteps = W * H * 4 + 10;
        while (curr != startState && --maxSteps > 0) {
            int node = curr / 4;
            int x = node % W;
            int y = node / W;
            path.prepend(QPointF(xs[x], ys[y]));
            int prev = cameFrom[curr];
            if (prev < 0) break;
            curr = prev;
        }
        int sX = startNode % W;
        int sY = startNode / W;
        path.prepend(QPointF(xs[sX], ys[sY]));
    } else {
        path.append(pStart);
        path.append(QPointF(midX, pStart.y()));
        path.append(QPointF(midX, pEnd.y()));
        path.append(pEnd);
    }

    if (path.isEmpty() || (path.first() - pStart).manhattanLength() > 1.0) {
        path.prepend(pStart);
    }
    if ((path.first() - start).manhattanLength() > 1.0) {
        path.prepend(start);
    }

    if ((path.last() - pEnd).manhattanLength() > 1.0) {
        path.append(pEnd);
    }
    if ((path.last() - end).manhattanLength() > 1.0) {
        path.append(end);
    }

    QVector<QPointF> simplified = simplifyCollinear(path);
    simplified = nudgeCollidingSegments(simplified, localObstacles, localSegments, margin);
    return simplifyCollinear(simplified);
}

} // namespace editor
