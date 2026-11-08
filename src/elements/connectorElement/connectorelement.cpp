#include "connectorelement.h"
#include "src/canvas/canvasscene.h"
#include "src/canvas/orthogonalrouter.h"
#include "src/canvas/undocommands.h"

#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace editor {

ConnectorElement::ConnectorElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setPos(0, 0);
    // Коннекторы располагаются поверх обычных блоков, чтобы их маркеры на портах легко захватывались мышью
    setZValue(2.0);
    borderColor_ = QColor(24, 24, 27);
    borderWidth_ = 1.5;
    points_ = {p1_, p2_};
}

ConnectorElement::RouteState ConnectorElement::routeState() const {
    return {points_, startBlock_, startPort_, endBlock_, endPort_,
            hasManualOffset_, manualMidOffset_};
}

void ConnectorElement::setRouteState(const RouteState &state) {
    setStartConnection(state.startBlock, state.startPort);
    setEndConnection(state.endBlock, state.endPort);
    if (state.hasManualOffset) setManualMidOffset(state.manualMidOffset);
    else clearManualOffset();
    setPoints(state.points);
}

void ConnectorElement::setP1(const QPointF &p) {
    if (p1_ != p) {
        prepareGeometryChange();
        p1_ = p;
        if (points_.size() >= 2) points_[0] = p1_;
        update();
        emit geometryChanged();
    }
}

void ConnectorElement::setP2(const QPointF &p) {
    if (p2_ != p) {
        prepareGeometryChange();
        p2_ = p;
        if (points_.size() >= 2) points_.last() = p2_;
        update();
        emit geometryChanged();
    }
}

void ConnectorElement::setPoints(const QVector<QPointF> &pts) {
    if (pts.size() < 2) return;
    if (points_ == pts && p1_ == pts.first() && p2_ == pts.last()) {
        return;
    }
    prepareGeometryChange();
    points_ = pts;
    p1_ = pts.first();
    p2_ = pts.last();
    update();
    emit geometryChanged();
}

void ConnectorElement::setStartConnection(MasterBlockElement *block, PortPosition port) {
    startBlock_ = block;
    startPort_ = port;
    emit propertyChanged();
}

void ConnectorElement::setEndConnection(MasterBlockElement *block, PortPosition port) {
    endBlock_ = block;
    endPort_ = port;
    emit propertyChanged();
}

void ConnectorElement::setBeginMarker(LineMarkerType marker) {
    if (beginMarker_ != marker) {
        beginMarker_ = marker;
        update();
        emit propertyChanged();
    }
}

void ConnectorElement::setEndMarker(LineMarkerType marker) {
    if (endMarker_ != marker) {
        endMarker_ = marker;
        update();
        emit propertyChanged();
    }
}

bool ConnectorElement::isVerticalSegment(int idx) const {
    if (idx < 0 || idx >= points_.size() - 1) return false;
    return std::abs(points_[idx].x() - points_[idx + 1].x()) < 1.0;
}

int ConnectorElement::findNearestSegment(const QPointF &pt) const {
    if (points_.size() < 2) return -1;
    int bestIdx = -1;
    qreal bestDist = 14.0;
    for (int i = 0; i < points_.size() - 1; ++i) {
        const QPointF &a = points_[i];
        const QPointF &b = points_[i + 1];
        QPointF ab = b - a;
        qreal lenSq = ab.x() * ab.x() + ab.y() * ab.y();
        qreal dist = 1e9;
        if (lenSq < 1e-4) {
            dist = std::hypot(pt.x() - a.x(), pt.y() - a.y());
        } else {
            qreal t = qBound(0.0, ((pt.x() - a.x()) * ab.x() + (pt.y() - a.y()) * ab.y()) / lenSq, 1.0);
            QPointF proj = a + t * ab;
            dist = std::hypot(pt.x() - proj.x(), pt.y() - proj.y());
        }
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

QRectF ConnectorElement::boundingRect() const {
    qreal penW = qMax(qreal(1.0), borderWidth_);
    qreal margin = penW + 16.0;
    if (points_.isEmpty()) {
        return QRectF(p1_, p2_).normalized().adjusted(-margin, -margin, margin, margin);
    }
    qreal minX = points_[0].x(), maxX = points_[0].x();
    qreal minY = points_[0].y(), maxY = points_[0].y();
    for (const auto &pt : points_) {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
    }
    return QRectF(minX, minY, maxX - minX, maxY - minY).adjusted(-margin, -margin, margin, margin);
}

QPainterPath ConnectorElement::shape() const {
    QPainterPath path;
    if (points_.size() < 2) return path;

    QPainterPathStroker stroker;
    stroker.setWidth(qMax(borderWidth_ * 2.5, 13.0));
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);

    QPainterPath linePath;
    linePath.moveTo(points_[0]);
    for (int i = 1; i < points_.size(); ++i) {
        linePath.lineTo(points_[i]);
    }
    path = stroker.createStroke(linePath);

    // Включаем зоны маркеров в shape(), чтобы клик по углам маркеров никогда не сбрасывал выделение
    if (isSelected()) {
        path.setFillRule(Qt::WindingFill);
        const qreal hr = 10.0;
        const QPointF pStart = points_.first();
        const QPointF pEnd = points_.last();
        path.addRect(QRectF(pStart.x() - hr, pStart.y() - hr, hr * 2.0, hr * 2.0));
        path.addRect(QRectF(pEnd.x() - hr, pEnd.y() - hr, hr * 2.0, hr * 2.0));

        if (points_.size() >= 4) {
            for (int i = 1; i <= points_.size() - 3; ++i) {
                QPointF mid = (points_[i] + points_[i + 1]) * 0.5;
                path.addRect(QRectF(mid.x() - hr, mid.y() - hr, hr * 2.0, hr * 2.0));
            }
        }
    }

    return path;
}

ResizeHandle ConnectorElement::handleAt(const QPointF &localPos) const {
    if (!isSelected() || points_.isEmpty()) return ResizeHandle::None;

    const qreal r = 10.0;

    // 1. Только промежуточные сегменты в стиле Visio (желтые ромбы)
    // Проверяем их первыми, чтобы при коротких сегментах ромб легко захватывался
    if (points_.size() >= 4) {
        for (int i = 1; i <= points_.size() - 3; ++i) {
            if (std::hypot(points_[i + 1].x() - points_[i].x(),
                           points_[i + 1].y() - points_[i].y()) < 8.0) {
                continue;
            }
            QPointF mid = (points_[i] + points_[i + 1]) * 0.5;
            if (std::hypot(mid.x() - localPos.x(), mid.y() - localPos.y()) <= r) {
                return static_cast<ResizeHandle>(static_cast<int>(ResizeHandle::ConnectorSegmentBase) + i);
            }
        }
    }

    // 2. Конечные точки P1 и P2 (строго по первой и последней точке маршрута)
    if (std::hypot(points_.first().x() - localPos.x(), points_.first().y() - localPos.y()) <= r) {
        return ResizeHandle::Point1;
    }
    if (std::hypot(points_.last().x() - localPos.x(), points_.last().y() - localPos.y()) <= r) {
        return ResizeHandle::Point2;
    }

    return ResizeHandle::None;
}

QRectF ConnectorElement::handleRect(ResizeHandle handle) const {
    const qreal s = handleSize();
    QPointF center;
    int hVal = static_cast<int>(handle);
    if (handle == ResizeHandle::Point1) {
        center = points_.isEmpty() ? p1_ : points_.first();
    } else if (handle == ResizeHandle::Point2) {
        center = points_.isEmpty() ? p2_ : points_.last();
    } else if (hVal >= static_cast<int>(ResizeHandle::ConnectorSegmentBase) &&
               hVal < static_cast<int>(ResizeHandle::ConnectorBendBase)) {
        int segIdx = hVal - static_cast<int>(ResizeHandle::ConnectorSegmentBase);
        if (points_.size() >= 4 && segIdx >= 1 && segIdx <= points_.size() - 3) {
            center = (points_[segIdx] + points_[segIdx + 1]) * 0.5;
        } else {
            return QRectF();
        }
    } else {
        return QRectF();
    }

    return QRectF(center.x() - s / 2.0, center.y() - s / 2.0, s, s);
}

void ConnectorElement::startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) {
    ShapeElement::startHandleDrag(handle, startScenePos);
    activeHandle_ = handle;
    dragStartScenePos_ = startScenePos;
    initialP1_ = p1_;
    initialP2_ = p2_;
    dragInitialPoints_ = points_;

    if (handle == ResizeHandle::Point1 || handle == ResizeHandle::Point2) {
        if (auto s = dynamic_cast<CanvasScene *>(scene())) {
            s->setConnectingMode(true);
        }
    }
}

void ConnectorElement::moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid, qreal gridSize) {
    CanvasScene *s = dynamic_cast<CanvasScene *>(scene());

    if (handle == ResizeHandle::Point1 || handle == ResizeHandle::Point2) {
        QPointF targetPos = currentScenePos;
        MasterBlockElement *hoveredBlock = nullptr;
        PortPosition hoveredPort = PortPosition::None;

        if (s) {
            auto match = s->findPortAt(currentScenePos, nullptr, 22.0);
            if (match.first && match.second != PortPosition::None) {
                hoveredBlock = match.first;
                hoveredPort = match.second;
                targetPos = hoveredBlock->portScenePos(hoveredPort);
            }

            for (QGraphicsItem *item : s->items()) {
                if (auto b = dynamic_cast<MasterBlockElement *>(item)) {
                    b->setHoveredPort(b == hoveredBlock ? hoveredPort : PortPosition::None);
                }
            }
        }

        if (!hoveredBlock && snapToGrid && gridSize > 1.0) {
            targetPos = QPointF(
                std::round(currentScenePos.x() / gridSize) * gridSize,
                std::round(currentScenePos.y() / gridSize) * gridSize
            );
        }

        if (handle == ResizeHandle::Point1) {
            setP1(targetPos);
            setStartConnection(hoveredBlock, hoveredPort);
        } else if (handle == ResizeHandle::Point2) {
            setP2(targetPos);
            setEndConnection(hoveredBlock, hoveredPort);
        }

        if (isOrthogonal_ && s) {
            QPointF startPt = p1_;
            QPointF startNorm(0, 0);
            if (startBlock_ && startPort_ != PortPosition::None) {
                startPt = startBlock_->portScenePos(startPort_);
                startNorm = startBlock_->portNormal(startPort_);
            }
            QPointF endPt = p2_;
            QPointF endNorm(0, 0);
            if (endBlock_ && endPort_ != PortPosition::None) {
                endPt = endBlock_->portScenePos(endPort_);
                endNorm = endBlock_->portNormal(endPort_);
            }

            auto obstacles = s->obstacleRects();
            QList<QLineF> existingSegments;
            for (QGraphicsItem *item : s->items()) {
                if (auto c = dynamic_cast<ConnectorElement *>(item)) {
                    if (c == this) continue;
                    const auto &pts = c->points();
                    for (int i = 0; i < pts.size() - 1; ++i) {
                        existingSegments.append(QLineF(pts[i], pts[i + 1]));
                    }
                }
            }

            qreal laneOffset = hasManualOffset_ ? manualMidOffset_ : 0.0;
            QVector<QPointF> route = OrthogonalRouter::route(
                startPt, startNorm, endPt, endNorm, obstacles, 18.0, laneOffset, existingSegments);
            setPoints(route);
        } else if (!isOrthogonal_) {
            setPoints({p1_, p2_});
        }
        return;
    }

    // Перемещение строго промежуточного сегмента (желтый ромб Visio) относительно начальной точки захвата
    int hVal = static_cast<int>(handle);
    if (hVal >= static_cast<int>(ResizeHandle::ConnectorSegmentBase) &&
        hVal < static_cast<int>(ResizeHandle::ConnectorBendBase)) {
        int segIdx = hVal - static_cast<int>(ResizeHandle::ConnectorSegmentBase);
        if (points_.size() >= 4 &&
            dragInitialPoints_.size() == points_.size() &&
            segIdx >= 1 && segIdx <= points_.size() - 3) {

            QPointF delta = currentScenePos - dragStartScenePos_;
            bool vert = std::abs(dragInitialPoints_[segIdx].x() - dragInitialPoints_[segIdx + 1].x()) < 1.0;
            qreal fineSnap = (snapToGrid && gridSize > 1.0) ? std::min(gridSize * 0.25, 5.0) : 0.0;

            prepareGeometryChange();
            if (vert) {
                qreal targetX = dragInitialPoints_[segIdx].x() + delta.x();
                if (fineSnap > 1.0) {
                    targetX = std::round(targetX / fineSnap) * fineSnap;
                }
                points_[segIdx].setX(targetX);
                points_[segIdx + 1].setX(targetX);
                manualMidOffset_ = targetX - (points_.first().x() + points_.last().x()) * 0.5;
            } else {
                qreal targetY = dragInitialPoints_[segIdx].y() + delta.y();
                if (fineSnap > 1.0) {
                    targetY = std::round(targetY / fineSnap) * fineSnap;
                }
                points_[segIdx].setY(targetY);
                points_[segIdx + 1].setY(targetY);
                manualMidOffset_ = targetY - (points_.first().y() + points_.last().y()) * 0.5;
            }

            // ГАРАНТИЯ: Концы коннектора ВСЕГДА жестко зафиксированы на портах блоков
            if (startBlock_ && startPort_ != PortPosition::None) {
                points_[0] = startBlock_->portScenePos(startPort_);
            }
            if (endBlock_ && endPort_ != PortPosition::None) {
                points_.last() = endBlock_->portScenePos(endPort_);
            }
            p1_ = points_.first();
            p2_ = points_.last();

            hasManualOffset_ = true;
            update();
            emit geometryChanged();
        }
        return;
    }
}

void ConnectorElement::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        hasExceededDragThreshold_ = false;
        ResizeHandle h = handleAt(event->pos());
        if (h != ResizeHandle::None) {
            activeHandle_ = h;
            isDraggingHandle_ = true;
            dragStartScenePos_ = event->scenePos();
            startHandleDrag(h, event->scenePos());
            event->accept();
            return;
        }

        // Клик по телу коннектора — выделяем и подготавливаем возможное перетаскивание
        isDraggingBody_ = true;
        dragBodyStartScenePos_ = event->scenePos();
        dragStartScenePos_ = event->scenePos();
        dragBodyInitialP1_ = p1_;
        dragBodyInitialP2_ = p2_;
        dragBodyInitialPoints_ = points_;
        dragInitialPoints_ = points_;
        dragBodyNearestSegment_ = findNearestSegment(event->pos());

        if (!isSelected()) {
            if (scene() && !(event->modifiers() & Qt::ControlModifier)) {
                scene()->clearSelection();
            }
            setSelected(true);
        }
        event->accept();
        return;
    }
    ShapeElement::mousePressEvent(event);
}

void ConnectorElement::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (isDraggingHandle_) {
        if (!hasExceededDragThreshold_) {
            if (std::hypot(event->scenePos().x() - dragStartScenePos_.x(),
                           event->scenePos().y() - dragStartScenePos_.y()) < 2.0) {
                event->accept();
                return;
            }
            hasExceededDragThreshold_ = true;
        }
        bool snap = false;
        qreal gSize = 20.0;
        if (auto s = dynamic_cast<CanvasScene *>(scene())) {
            snap = s->isSnapToGrid();
            gSize = s->gridSize();
        }
        moveHandle(activeHandle_, event->scenePos(), snap, gSize);
        event->accept();
        return;
    }

    if (isDraggingBody_) {
        // Порог 3 пикселя гарантирует, что обычный клик для выделения линии никогда не сдвинет сегмент
        if (!hasExceededDragThreshold_) {
            if (std::hypot(event->scenePos().x() - dragBodyStartScenePos_.x(),
                           event->scenePos().y() - dragBodyStartScenePos_.y()) < 3.0) {
                event->accept();
                return;
            }
            hasExceededDragThreshold_ = true;
        }

        bool snap = false;
        qreal gSize = 20.0;
        if (auto s = dynamic_cast<CanvasScene *>(scene())) {
            snap = s->isSnapToGrid();
            gSize = s->gridSize();
        }

        // Если коннектор СВОБОДНЫЙ (не привязан к блокам) — перемещаем его целиком
        if (!startBlock_ && !endBlock_) {
            QPointF delta = event->scenePos() - dragBodyStartScenePos_;
            if (snap && gSize > 1.0) {
                delta = QPointF(
                    std::round(delta.x() / gSize) * gSize,
                    std::round(delta.y() / gSize) * gSize
                );
            }
            prepareGeometryChange();
            p1_ = dragBodyInitialP1_ + delta;
            p2_ = dragBodyInitialP2_ + delta;
            points_.clear();
            for (const auto &pt : dragBodyInitialPoints_) {
                points_.append(pt + delta);
            }
            update();
            emit geometryChanged();
        } else {
            // Если привязан к блокам — двигаем ТОЛЬКО тот промежуточный сегмент (1 .. size-3), за который тянет пользователь
            int segIdx = dragBodyNearestSegment_;
            if (points_.size() >= 4 && segIdx >= 1 && segIdx <= points_.size() - 3) {
                ResizeHandle segHandle = static_cast<ResizeHandle>(
                    static_cast<int>(ResizeHandle::ConnectorSegmentBase) + segIdx
                );
                moveHandle(segHandle, event->scenePos(), snap, gSize);
            }
        }
        event->accept();
        return;
    }

    ShapeElement::mouseMoveEvent(event);
}

void ConnectorElement::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (isDraggingHandle_) {
        ResizeHandle releasedHandle = activeHandle_;
        isDraggingHandle_ = false;
        activeHandle_ = ResizeHandle::None;
        hasExceededDragThreshold_ = false;

        if (auto s = dynamic_cast<CanvasScene *>(scene())) {
            for (QGraphicsItem *item : s->items()) {
                if (auto b = dynamic_cast<MasterBlockElement *>(item)) {
                    b->setHoveredPort(PortPosition::None);
                }
            }
            s->setConnectingMode(isSelected());
            // Если перетаскивался конец линии и он примагнитился к порту — пересчитываем полосы без наложений
            if ((releasedHandle == ResizeHandle::Point1 || releasedHandle == ResizeHandle::Point2) &&
                (startBlock_ || endBlock_)) {
                s->updateConnectedLines();
            }
        }
        event->accept();
        return;
    }

    if (isDraggingBody_) {
        bool wasMoved = hasExceededDragThreshold_;
        isDraggingBody_ = false;
        hasExceededDragThreshold_ = false;

        if (wasMoved && (!startBlock_ || !endBlock_)) {
            if (auto s = dynamic_cast<CanvasScene *>(scene())) {
                if (!startBlock_) {
                    auto match = s->findPortAt(p1_, nullptr, 22.0);
                    if (match.first && match.second != PortPosition::None) {
                        setStartConnection(match.first, match.second);
                        setP1(match.first->portScenePos(match.second));
                    }
                }
                if (!endBlock_) {
                    auto match = s->findPortAt(p2_, nullptr, 22.0);
                    if (match.first && match.second != PortPosition::None) {
                        setEndConnection(match.first, match.second);
                        setP2(match.first->portScenePos(match.second));
                    }
                }
                if ((startBlock_ || endBlock_) && isOrthogonal_) {
                    s->updateConnectedLines();
                }
            }
        }
        event->accept();
        return;
    }

    ShapeElement::mouseReleaseEvent(event);
}

void ConnectorElement::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton && hasManualOffset_) {
        auto *s = dynamic_cast<CanvasScene *>(scene());
        QHash<ConnectorElement *, RouteState> oldStates;
        if (s && s->undoStack()) {
            for (ShapeElement *element : s->allElements())
                if (auto *connector = dynamic_cast<ConnectorElement *>(element))
                    oldStates.insert(connector, connector->routeState());
        }
        clearManualOffset();
        if (s) {
            s->updateConnectedLines();
            if (s->undoStack()) {
                s->undoStack()->beginMacro(QStringLiteral("Сброс маршрута"));
                for (auto it = oldStates.cbegin(); it != oldStates.cend(); ++it) {
                    const RouteState newState = it.key()->routeState();
                    if (newState != it.value())
                        s->undoStack()->push(new ResizeConnectorCommand(s, it.key(), it.value(), newState));
                }
                s->undoStack()->endMacro();
            }
        }
        event->accept();
        return;
    }
    ShapeElement::mouseDoubleClickEvent(event);
}

void ConnectorElement::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    if (isSelected()) {
        ResizeHandle h = handleAt(event->pos());
        int hVal = static_cast<int>(h);
        if (h == ResizeHandle::Point1 || h == ResizeHandle::Point2) {
            setCursor(Qt::CrossCursor);
        } else if (hVal >= static_cast<int>(ResizeHandle::ConnectorSegmentBase) &&
                   hVal < static_cast<int>(ResizeHandle::ConnectorBendBase)) {
            int segIdx = hVal - static_cast<int>(ResizeHandle::ConnectorSegmentBase);
            setCursor(isVerticalSegment(segIdx) ? Qt::SizeHorCursor : Qt::SizeVerCursor);
        } else {
            if (!startBlock_ && !endBlock_) {
                setCursor(Qt::SizeAllCursor);
            } else {
                int nearSeg = findNearestSegment(event->pos());
                if (points_.size() >= 4 && nearSeg >= 1 && nearSeg <= points_.size() - 3) {
                    setCursor(isVerticalSegment(nearSeg) ? Qt::SizeHorCursor : Qt::SizeVerCursor);
                } else {
                    setCursor(Qt::ArrowCursor);
                }
            }
        }
    } else {
        setCursor(Qt::ArrowCursor);
    }
    ShapeElement::hoverMoveEvent(event);
}

void ConnectorElement::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    setCursor(Qt::ArrowCursor);
    ShapeElement::hoverLeaveEvent(event);
}

void ConnectorElement::drawArrow(QPainter *painter, const QPointF &tip, const QPointF &from) {
    QPointF diff = tip - from;
    qreal len = std::hypot(diff.x(), diff.y());
    if (len < 1.0) return;

    QPointF dir = diff / len;
    QPointF normal(-dir.y(), dir.x());

    const qreal arrowLength = 9.0;
    const qreal arrowWidth = 5.0;

    QPointF pA = tip - dir * arrowLength + normal * arrowWidth;
    QPointF pB = tip - dir * arrowLength - normal * arrowWidth;

    QPainterPath path;
    path.moveTo(tip);
    path.lineTo(pA);
    path.lineTo(pB);
    path.closeSubpath();

    painter->save();
    painter->setBrush(borderColor_);
    painter->setPen(Qt::NoPen);
    painter->drawPath(path);
    painter->restore();
}

void ConnectorElement::drawShape(QPainter *painter) {
    if (points_.size() < 2) return;

    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor lineColor = isSelected() ? QColor(37, 99, 235) : borderColor_;
    qreal lineW = isSelected() ? qMax(borderWidth_, 1.8) : borderWidth_;
    QPen pen(lineColor, lineW, borderStyle_, Qt::SquareCap, Qt::MiterJoin);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    for (int i = 0; i < points_.size() - 1; ++i) {
        painter->drawLine(points_[i], points_[i + 1]);
    }

    if (beginMarker_ == LineMarkerType::Arrow && points_.size() >= 2) {
        drawArrow(painter, points_[0], points_[1]);
    }
    if (endMarker_ == LineMarkerType::Arrow && points_.size() >= 2) {
        drawArrow(painter, points_.last(), points_[points_.size() - 2]);
    }
}

void ConnectorElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    drawShape(painter);

    if (isSelected() && !points_.isEmpty()) {
        const qreal s = handleSize();

        // 1. ТОЛЬКО промежуточные маркеры в стиле Visio (желтые ромбы)
        // Строго от 1 до points_.size() - 3, без квадратиков в местах излома
        if (points_.size() >= 4) {
            for (int i = 1; i <= points_.size() - 3; ++i) {
                if (std::hypot(points_[i + 1].x() - points_[i].x(),
                               points_[i + 1].y() - points_[i].y()) < 8.0) {
                    continue;
                }
                QPointF mid = (points_[i] + points_[i + 1]) * 0.5;
                QPolygonF diamond;
                diamond << QPointF(mid.x(), mid.y() - 5.5)
                        << QPointF(mid.x() + 5.5, mid.y())
                        << QPointF(mid.x(), mid.y() + 5.5)
                        << QPointF(mid.x() - 5.5, mid.y());

                painter->setPen(QPen(QColor(180, 83, 9), 1.3)); // Теплый янтарный контур
                painter->setBrush(QColor(250, 204, 21));         // Желтый Visio ромб
                painter->drawPolygon(diamond);
            }
        }

        // 2. Конечные маркеры (синие квадраты) СТРОГО на концах линии
        const QPointF startPt = points_.first();
        const QPointF endPt = points_.last();
        for (const QPointF &pt : {startPt, endPt}) {
            QRectF r(pt.x() - s / 2.0, pt.y() - s / 2.0, s, s);
            painter->setPen(QPen(QColor(37, 99, 235), 1.6));
            painter->setBrush(Qt::white);
            painter->drawRect(r);
        }
    }
}

} // namespace editor
