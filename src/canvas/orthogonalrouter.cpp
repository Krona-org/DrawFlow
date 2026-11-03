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
        // Уменьшаем препятствие на 1px для устойчивости к касаниям по границе
        QRectF core = obs.adjusted(1.0, 1.0, -1.0, -1.0);
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
            // Диагональных отрезков в сетке нет, но на всякий случай
            if (obs.intersects(QRectF(xMin, yMin, xMax - xMin, yMax - yMin))) {
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

        bool collinX = std::abs(prev.x() - curr.x()) < 1e-3 && std::abs(curr.x() - next.x()) < 1e-3;
        bool collinY = std::abs(prev.y() - curr.y()) < 1e-3 && std::abs(curr.y() - next.y()) < 1e-3;

        if (!collinX && !collinY) {
            result.append(curr);
        }
    }

    result.append(pts.last());
    return result;
}

QVector<QPointF> OrthogonalRouter::route(
    const QPointF &start,
    const QPointF &startNormal,
    const QPointF &end,
    const QPointF &endNormal,
    const QList<QRectF> &obstacles,
    qreal margin
) {
    // Начальный и конечный выход с отступом в сторону нормали порта
    qreal startLen = std::hypot(startNormal.x(), startNormal.y());
    QPointF pStart = (startLen > 0.1) ? (start + startNormal * margin) : start;

    qreal endLen = std::hypot(endNormal.x(), endNormal.y());
    QPointF pEnd = (endLen > 0.1) ? (end + endNormal * margin) : end;

    // Сбор кандидатов X и Y для ортогональной сетки каналов
    std::vector<qreal> xs;
    std::vector<qreal> ys;

    xs.push_back(start.x());
    xs.push_back(pStart.x());
    xs.push_back(end.x());
    xs.push_back(pEnd.x());
    xs.push_back((pStart.x() + pEnd.x()) * 0.5);

    ys.push_back(start.y());
    ys.push_back(pStart.y());
    ys.push_back(end.y());
    ys.push_back(pEnd.y());
    ys.push_back((pStart.y() + pEnd.y()) * 0.5);

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
    // Состояние: (nodeIndex, dir), где dir: 0 - none, 1 - horizontal, 2 - vertical
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
    std::unordered_map<int, qreal> gScore; // key: node * 4 + dir
    std::unordered_map<int, int> cameFrom; // key: node * 4 + dir -> prev_node * 4 + prev_dir

    auto encode = [](int node, int dir) { return node * 4 + dir; };

    int startState = encode(startNode, 0);
    gScore[startState] = 0.0;
    openSet.push({startNode, 0, 0.0, 0.0});

    int targetX = targetNode % W;
    int targetY = targetNode / W;
    QPointF targetPt(xs[targetX], ys[targetY]);

    const qreal turnPenalty = 45.0; // Штраф за поворот для минимизации лишних изгибов
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

        // 4 ортогональных соседа: влево, вправо, вверх, вниз
        const int dx[4] = {-1, 1, 0, 0};
        const int dy[4] = {0, 0, -1, 1};
        const int dirs[4] = {1, 1, 2, 2}; // 1 = horiz, 2 = vert

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;

            QPointF nPos(xs[nx], ys[ny]);

            // Проверяем, не пересекает ли отрезок препятствия
            if (segmentIntersectsObstacles(cPos, nPos, obstacles)) {
                continue;
            }

            qreal dist = std::hypot(nPos.x() - cPos.x(), nPos.y() - cPos.y());
            qreal extraTurn = (current.dir != 0 && current.dir != dirs[i]) ? turnPenalty : 0.0;
            qreal tentG = current.cost + dist + extraTurn;

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
        while (curr != startState) {
            int node = curr / 4;
            int x = node % W;
            int y = node / W;
            path.prepend(QPointF(xs[x], ys[y]));
            curr = cameFrom[curr];
        }
        int sX = startNode % W;
        int sY = startNode / W;
        path.prepend(QPointF(xs[sX], ys[sY]));
    } else {
        // Запасной классический Z/L-маршрут при изоляции
        path.append(pStart);
        path.append(QPointF(pEnd.x(), pStart.y()));
        path.append(pEnd);
    }

    // Добавляем точку старта и финиша
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

    // Упрощаем коллинеарные точки для строгого прямого 90-градусного вида
    return simplifyCollinear(path);
}

} // namespace editor

