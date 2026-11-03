#include "lineelement.h"

#include <QPainter>
#include <QtMath>
#include <cmath>

namespace editor {

LineElement::LineElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    borderColor_ = QColor(51, 65, 85);
    borderWidth_ = 2.0;
    endMarker_ = LineMarkerType::Arrow;

    points_.append(p1_);
    points_.append(p2_);
    updateBounds();
}

void LineElement::setP1(const QPointF &p) {
    if (p1_ != p) {
        prepareGeometryChange();
        p1_ = p;
        if (!points_.isEmpty()) {
            points_.first() = p;
        } else {
            points_.append(p);
            points_.append(p2_);
        }
        updateBounds();
        emit geometryChanged();
        emit propertyChanged();
    }
}

void LineElement::setP2(const QPointF &p) {
    if (p2_ != p) {
        prepareGeometryChange();
        p2_ = p;
        if (!points_.isEmpty()) {
            points_.last() = p;
        } else {
            points_.append(p1_);
            points_.append(p);
        }
        updateBounds();
        emit geometryChanged();
        emit propertyChanged();
    }
}

void LineElement::setPoints(const QVector<QPointF> &pts) {
    if (pts.size() < 2) return;
    prepareGeometryChange();
    points_ = pts;
    p1_ = points_.first();
    p2_ = points_.last();
    updateBounds();
    emit geometryChanged();
    emit propertyChanged();
}

void LineElement::setStartConnection(RectangleElement *rect, PortPosition port) {
    startRect_ = rect;
    startPort_ = port;
}

void LineElement::setEndConnection(RectangleElement *rect, PortPosition port) {
    endRect_ = rect;
    endPort_ = port;
}

void LineElement::setBeginMarker(LineMarkerType marker) {
    if (beginMarker_ != marker) {
        beginMarker_ = marker;
        update();
        emit propertyChanged();
    }
}

void LineElement::setEndMarker(LineMarkerType marker) {
    if (endMarker_ != marker) {
        endMarker_ = marker;
        update();
        emit propertyChanged();
    }
}

void LineElement::updateBounds() {
    if (points_.isEmpty()) {
        points_.append(p1_);
        points_.append(p2_);
    }

    qreal minX = points_[0].x();
    qreal maxX = points_[0].x();
    qreal minY = points_[0].y();
    qreal maxY = points_[0].y();

    for (const QPointF &pt : points_) {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
    }

    rect_ = QRectF(minX, minY, qMax(10.0, maxX - minX), qMax(10.0, maxY - minY));
    update();
}

QRectF LineElement::handleRect(ResizeHandle handle) const {
    const qreal hs = handleSize();
    const qreal half = hs / 2.0;

    if (handle == ResizeHandle::Point1) {
        return QRectF(p1_.x() - half, p1_.y() - half, hs, hs);
    }
    if (handle == ResizeHandle::Point2) {
        return QRectF(p2_.x() - half, p2_.y() - half, hs, hs);
    }
    return QRectF();
}

ResizeHandle LineElement::handleAt(const QPointF &localPos) const {
    if (!isSelected()) {
        return ResizeHandle::None;
    }

    QRectF hr1 = handleRect(ResizeHandle::Point1).adjusted(-4, -4, 4, 4);
    if (hr1.contains(localPos)) {
        return ResizeHandle::Point1;
    }

    QRectF hr2 = handleRect(ResizeHandle::Point2).adjusted(-4, -4, 4, 4);
    if (hr2.contains(localPos)) {
        return ResizeHandle::Point2;
    }

    return ResizeHandle::None;
}

void LineElement::startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) {
    ShapeElement::startHandleDrag(handle, startScenePos);
    initialP1_ = p1_;
    initialP2_ = p2_;
}

void LineElement::moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid, qreal gridSize) {
    prepareGeometryChange();

    QPointF targetPt = currentScenePos - pos();
    if (snapToGrid && gridSize > 1.0) {
        targetPt.setX(std::round(targetPt.x() / gridSize) * gridSize);
        targetPt.setY(std::round(targetPt.y() / gridSize) * gridSize);
    }

    if (handle == ResizeHandle::Point1) {
        setP1(targetPt);
    } else if (handle == ResizeHandle::Point2) {
        setP2(targetPt);
    }
}

QRectF LineElement::boundingRect() const {
    qreal extra = borderWidth_ + handleSize() + 24.0;
    if (points_.isEmpty()) {
        return rect_.adjusted(-extra, -extra, extra, extra);
    }

    qreal minX = points_[0].x();
    qreal maxX = points_[0].x();
    qreal minY = points_[0].y();
    qreal maxY = points_[0].y();

    for (const QPointF &pt : points_) {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
    }

    return QRectF(minX - extra, minY - extra, (maxX - minX) + 2.0 * extra, (maxY - minY) + 2.0 * extra);
}

QPainterPath LineElement::shape() const {
    QPainterPath path;
    if (points_.size() >= 2) {
        path.moveTo(points_[0]);
        for (int i = 1; i < points_.size(); ++i) {
            path.lineTo(points_[i]);
        }
    } else {
        path.moveTo(p1_);
        path.lineTo(p2_);
    }

    QPainterPathStroker stroker;
    stroker.setWidth(qMax(14.0, borderWidth_ + 10.0));
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    QPainterPath stroke = stroker.createStroke(path);
    if (isSelected()) {
        stroke.addRect(handleRect(ResizeHandle::Point1).adjusted(-4, -4, 4, 4));
        stroke.addRect(handleRect(ResizeHandle::Point2).adjusted(-4, -4, 4, 4));
    }
    return stroke;
}

static void drawArrowHead(QPainter *painter, const QPointF &from, const QPointF &to, qreal size) {
    qreal dx = to.x() - from.x();
    qreal dy = to.y() - from.y();
    qreal length = std::hypot(dx, dy);
    if (length < 1e-4) return;

    qreal angle = std::atan2(dy, dx);
    const qreal arrowAngle = M_PI / 6.0;

    QPointF pt1 = to - QPointF(size * std::cos(angle - arrowAngle), size * std::sin(angle - arrowAngle));
    QPointF pt2 = to - QPointF(size * std::cos(angle + arrowAngle), size * std::sin(angle + arrowAngle));

    QPolygonF arrowPolygon;
    arrowPolygon << to << pt1 << pt2;
    painter->drawPolygon(arrowPolygon);
}

void LineElement::drawShape(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    const auto &pts = (points_.size() >= 2) ? points_ : QVector<QPointF>{p1_, p2_};

    QPen linePen(borderColor_, borderWidth_, borderStyle_, Qt::SquareCap, Qt::MiterJoin);
    painter->setPen(linePen);
    painter->setBrush(Qt::NoBrush);

    for (int i = 0; i < pts.size() - 1; ++i) {
        painter->drawLine(pts[i], pts[i + 1]);
    }

    qreal arrowSize = qMax(9.0, borderWidth_ * 3.5);

    if (beginMarker_ == LineMarkerType::Arrow && pts.size() >= 2) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(borderColor_);
        drawArrowHead(painter, pts[1], pts[0], arrowSize);
    }
    if (endMarker_ == LineMarkerType::Arrow && pts.size() >= 2) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(borderColor_);
        drawArrowHead(painter, pts[pts.size() - 2], pts.last(), arrowSize);
    }
}

void LineElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    drawShape(painter);

    if (isSelected()) {
        QPen handlePen(QColor(71, 85, 105), 1.5);
        handlePen.setCosmetic(true);
        painter->setPen(handlePen);
        painter->setBrush(Qt::white);

        painter->drawRect(handleRect(ResizeHandle::Point1));
        painter->drawRect(handleRect(ResizeHandle::Point2));
    }

    painter->restore();
}

} // namespace editor
