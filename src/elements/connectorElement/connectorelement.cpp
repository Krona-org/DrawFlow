#include "connectorelement.h"

#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace editor {

ConnectorElement::ConnectorElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    borderColor_ = QColor(24, 24, 27);
    borderWidth_ = 1.5;
    points_ = {p1_, p2_};
}

void ConnectorElement::setP1(const QPointF &p) {
    if (p1_ != p) {
        prepareGeometryChange();
        p1_ = p;
        if (points_.size() >= 2) points_[0] = p1_;
        update();
        emit geometryChanged();
        emit propertyChanged();
    }
}

void ConnectorElement::setP2(const QPointF &p) {
    if (p2_ != p) {
        prepareGeometryChange();
        p2_ = p;
        if (points_.size() >= 2) points_.last() = p2_;
        update();
        emit geometryChanged();
        emit propertyChanged();
    }
}

void ConnectorElement::setPoints(const QVector<QPointF> &pts) {
    if (pts.size() < 2) return;
    prepareGeometryChange();
    points_ = pts;
    p1_ = pts.first();
    p2_ = pts.last();
    update();
    emit geometryChanged();
    emit propertyChanged();
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

QRectF ConnectorElement::boundingRect() const {
    qreal penW = qMax(qreal(1.0), borderWidth_);
    qreal margin = penW + 15.0; // с запасом под стрелки и маркеры
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
    stroker.setWidth(qMax(borderWidth_ * 2.5, 10.0));
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);

    QPainterPath linePath;
    linePath.moveTo(points_[0]);
    for (int i = 1; i < points_.size(); ++i) {
        linePath.lineTo(points_[i]);
    }
    return stroker.createStroke(linePath);
}

ResizeHandle ConnectorElement::handleAt(const QPointF &localPos) const {
    if (!isSelected()) return ResizeHandle::None;

    const qreal r = 7.0;
    if (std::hypot(p1_.x() - localPos.x(), p1_.y() - localPos.y()) <= r) {
        return ResizeHandle::Point1;
    }
    if (std::hypot(p2_.x() - localPos.x(), p2_.y() - localPos.y()) <= r) {
        return ResizeHandle::Point2;
    }
    return ResizeHandle::None;
}

QRectF ConnectorElement::handleRect(ResizeHandle handle) const {
    const qreal s = handleSize();
    QPointF center;
    if (handle == ResizeHandle::Point1) center = p1_;
    else if (handle == ResizeHandle::Point2) center = p2_;
    else return QRectF();

    return QRectF(center.x() - s / 2.0, center.y() - s / 2.0, s, s);
}

void ConnectorElement::startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) {
    activeHandle_ = handle;
    dragStartScenePos_ = startScenePos;
    initialP1_ = p1_;
    initialP2_ = p2_;
}

void ConnectorElement::moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid, qreal gridSize) {
    QPointF local = mapFromScene(currentScenePos);
    if (snapToGrid && gridSize > 1.0) {
        QPointF sceneSnapped(
            std::round(currentScenePos.x() / gridSize) * gridSize,
            std::round(currentScenePos.y() / gridSize) * gridSize
        );
        local = mapFromScene(sceneSnapped);
    }

    if (handle == ResizeHandle::Point1) {
        setP1(local);
    } else if (handle == ResizeHandle::Point2) {
        setP2(local);
    }
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

    QPen pen(borderColor_, borderWidth_, borderStyle_, Qt::SquareCap, Qt::MiterJoin);
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

    if (isSelected()) {
        const qreal s = handleSize();
        for (const QPointF &pt : {p1_, p2_}) {
            QRectF r(pt.x() - s / 2.0, pt.y() - s / 2.0, s, s);
            painter->setPen(QPen(QColor(37, 99, 235), 1.5));
            painter->setBrush(Qt::white);
            painter->drawRect(r);
        }
    }
}

} // namespace editor

