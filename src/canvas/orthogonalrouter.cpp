#include "orthogonalrouter.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <vector>

namespace editor {

bool OrthogonalRouter::segmentIntersectsObstacles(const QPointF &p1, const QPointF &p2, const QList<QRectF> &obstacles) {
    qreal xMin = std::min(p1.x(), p2.x());
    qreal xMax = std::max(p1.x(), p2.x());
    qreal yMin = std::min(p1.y(), p2.y());
    qreal yMax = std::max(p1.y(), p2.y());

    bool isHorizontal = std::abs(p1.y() - p2.y()) < 1e-4;
    bool isVertical = std::abs(p1.x() - p2.x()) < 1e-4;

    for (const QRectF &obs : obstacles) {
        // Уменьшаем препятствие на 1.5px для исключения ложных касаний
        QRectF core = obs.adjusted(1.5, 1.5, -1.5, -1.5);
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
            if (obs.intersects(QRectF(xMin, yMin, qMax(1.0, xMax - xMin), qMax(1.0, yMax - yMin)))) {
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

QVector<QPointF> OrthogonalRouter::simplifyCollinear(const QVector<QPointF> &pts) {
    if (pts.size() <= 2) return pts;

    QVector<QPointF> result;
    result.append(pts.first());

    for (int i = 1; i < pts.size() - 1; ++i) {
        const QPointF &prev = result.last();
        const QPointF &curr = pts[i];
        const QPointF &next = pts[i + 1];

        // Пропускаем дубликаты
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

QVector<QPointF> OrthogonalRouter::tryIdealFormulaRoute(
    const QPointF &start,
    const QPointF &startNormal,
    const QPointF &end,
    const QPointF &endNormal,
    const QList<QRectF> &obstacles,
    qreal margin
) {
    QPointF n1 = startNormal;
    QPointF n2 = endNormal;

    bool isH1 = std::abs(n1.x()) > 0.5;
    bool isV1 = std::abs(n1.y()) > 0.5;
    bool isH2 = std::abs(n2.x()) > 0.5;
    bool isV2 = std::abs(n2.y()) > 0.5;

    // СЛУЧАЙ 1: Оба порта горизонтальные (H-V-H Z-изгиб с вертикальной линией ровно посередине)
    if (isH1 && isH2) {
        // Противоположные направления (навстречу друг другу): Right -> Left
        if (n1.x() > 0 && n2.x() < 0 && end.x() > start.x() + margin * 2.0) {
            qreal midX = (start.x() + end.x()) * 0.5;
            QVector<QPointF> cand = { start, QPointF(midX, start.y()), QPointF(midX, end.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
        // Противоположные направления (навстречу друг другу): Left -> Right
        if (n1.x() < 0 && n2.x() > 0 && end.x() < start.x() - margin * 2.0) {
            qreal midX = (start.x() + end.x()) * 0.5;
            QVector<QPointF> cand = { start, QPointF(midX, start.y()), QPointF(midX, end.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
        // Одинаковые направления: оба выходят вправо
        if (n1.x() > 0 && n2.x() > 0) {
            qreal clearX = std::max(start.x(), end.x()) + margin * 1.5;
            QVector<QPointF> cand = { start, QPointF(clearX, start.y()), QPointF(clearX, end.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
        // Одинаковые направления: оба выходят влево
        if (n1.x() < 0 && n2.x() < 0) {
            qreal clearX = std::min(start.x(), end.x()) - margin * 1.5;
            QVector<QPointF> cand = { start, QPointF(clearX, start.y()), QPointF(clearX, end.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
    }

    // СЛУЧАЙ 2: Оба порта вертикальные (V-H-V Z-изгиб с горизонтальной линией ровно посередине)
    if (isV1 && isV2) {
        // Навстречу: Down -> Up
        if (n1.y() > 0 && n2.y() < 0 && end.y() > start.y() + margin * 2.0) {
            qreal midY = (start.y() + end.y()) * 0.5;
            QVector<QPointF> cand = { start, QPointF(start.x(), midY), QPointF(end.x(), midY), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
        // Навстречу: Up -> Down
        if (n1.y() < 0 && n2.y() > 0 && end.y() < start.y() - margin * 2.0) {
            qreal midY = (start.y() + end.y()) * 0.5;
            QVector<QPointF> cand = { start, QPointF(start.x(), midY), QPointF(end.x(), midY), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
        // Одинаковые направления: оба вниз
        if (n1.y() > 0 && n2.y() > 0) {
            qreal clearY = std::max(start.y(), end.y()) + margin * 1.5;
            QVector<QPointF> cand = { start, QPointF(start.x(), clearY), QPointF(end.x(), clearY), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
        // Одинаковые направления: оба вверх
        if (n1.y() < 0 && n2.y() < 0) {
            qreal clearY = std::min(start.y(), end.y()) - margin * 1.5;
            QVector<QPointF> cand = { start, QPointF(start.x(), clearY), QPointF(end.x(), clearY), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
    }

    // СЛУЧАЙ 3: Один горизонтальный, один вертикальный (L-изгиб под 90 градусов)
    if (isH1 && isV2) {
        // Проверяем возможность соединения прямым углом через точку (end.x, start.y)
        bool canExitDirect = (end.x() - start.x()) * n1.x() > 0.0;
        bool canEnterDirect = (start.y() - end.y()) * n2.y() > 0.0;

        if (canExitDirect && canEnterDirect) {
            QVector<QPointF> cand = { start, QPointF(end.x(), start.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
    }

    if (isV1 && isH2) {
        // Проверяем возможность соединения прямым углом через точку (start.x, end.y)
        bool canExitDirect = (end.y() - start.y()) * n1.y() > 0.0;
        bool canEnterDirect = (start.x() - end.x()) * n2.x() > 0.0;

        if (canExitDirect && canEnterDirect) {
            QVector<QPointF> cand = { start, QPointF(start.x(), end.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
    }

    // СЛУЧАЙ 4: Конечный порт не зафиксирован (ведение линии мышью в пустую область)
    if (isH1 && !isH2 && !isV2) {
        if ((end.x() - start.x()) * n1.x() > margin) {
            qreal midX = (start.x() + end.x()) * 0.5;
            QVector<QPointF> cand = { start, QPointF(midX, start.y()), QPointF(midX, end.y()), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
    }

    if (isV1 && !isH2 && !isV2) {
        if ((end.y() - start.y()) * n1.y() > margin) {
            qreal midY = (start.y() + end.y()) * 0.5;
            QVector<QPointF> cand = { start, QPointF(start.x(), midY), QPointF(end.x(), midY), end };
            if (!pathIntersectsObstacles(cand, obstacles)) return cand;
        }
    }

    return {};
}

QVector<QPointF> OrthogonalRouter::route(
    const QPointF &start,
    const QPointF &startNormal,
    const QPointF &end,
    const QPointF &endNormal,
    const QList<QRectF> &obstacles,
    qreal margin
) {
    // Быстрый выход: если начальная и конечная точки совпадают
    if (std::hypot(start.x() - end.x(), start.y() - end.y()) < 2.0) {
        QPointF norm = (startNormal.manhattanLength() > 0.1) ? startNormal : QPointF(1.0, 0.0);
        return { start, start + norm * margin };
    }

    // 1. Сначала пробуем идеальные формулы (H-V-H midpoint, V-H-V midpoint, L-изгиб)
    QVector<QPointF> ideal = tryIdealFormulaRoute(start, startNormal, end, endNormal, obstacles, margin);
    if (!ideal.isEmpty()) {
        return simplifyCollinear(ideal);
    }

    // 2. Если идеальный путь перекрыт препятствиями, запускаем интеллектуальный A* поиск
    qreal startLen = std::hypot(startNormal.x(), startNormal.y());
    QPointF pStart = (startLen > 0.1) ? (start + startNormal * margin) : start;

    qreal endLen = std::hypot(endNormal.x(), endNormal.y());
    QPointF pEnd = (endLen > 0.1) ? (end + endNormal * margin) : end;

    // Сбор кандидатов X и Y для ортогональной сетки каналов
    std::vector<qreal> xs;
    std::vector<qreal> ys;

    qreal midX = (pStart.x() + pEnd.x()) * 0.5;
    qreal midY = (pStart.y() + pEnd.y()) * 0.5;

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

    for (const QRectF &obs : obstacles) {
        xs.push_back(obs.left() - margin);
        xs.push_back(obs.right() + margin);
        ys.push_back(obs.top() - margin);
        ys.push_back(obs.bottom() + margin);
    }

    auto deduplicate = [](std::vector<qreal> &vec) {
        std::sort(vec.begin(), vec.end());
        std::vector<qreal> clean;
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

    // A* поиск по ортогональной сетке каналов
    struct State {
        int node;
        int dir;
        qreal cost;
        qreal priority;

        bool operator>(const State &other) const {
            return priority > other.priority;
        }
    };

    std::priority_queue<State, std::vector<State>, std::greater<State>> openSet;
    std::unordered_map<int, qreal> gScore;
    std::unordered_map<int, int> cameFrom;

    auto encode = [](int node, int dir) { return node * 4 + dir; };

    int startState = encode(startNode, 0);
    gScore[startState] = 0.0;
    openSet.push({startNode, 0, 0.0, 0.0});

    int targetX = targetNode % W;
    int targetY = targetNode / W;
    QPointF targetPt(xs[targetX], ys[targetY]);

    const qreal turnPenalty = 55.0; // Высокий штраф за поворот для предотвращения лишних изгибов
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

            QPointF nPos(xs[nx], ys[ny]);

            if (segmentIntersectsObstacles(cPos, nPos, obstacles)) {
                continue;
            }

            qreal dist = std::hypot(nPos.x() - cPos.x(), nPos.y() - cPos.y());
            qreal extraTurn = (current.dir != 0 && current.dir != dirs[i]) ? turnPenalty : 0.0;

            // Предпочтение центрированным каналам (midX / midY) для эстетики: положительный штраф за отклонение
            qreal nonCenterPenalty = (std::abs(nPos.x() - midX) < 1.0 || std::abs(nPos.y() - midY) < 1.0) ? 0.0 : 6.0;

            qreal tentG = current.cost + dist + extraTurn + nonCenterPenalty;

            int nNode = ny * W + nx;
            int nKey = encode(nNode, dirs[i]);

            if (gScore.find(nKey) == gScore.end() || tentG < gScore[nKey]) {
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
            auto it = cameFrom.find(curr);
            if (it == cameFrom.end()) break;
            curr = it->second;
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

    return simplifyCollinear(path);
}

} // namespace editor
