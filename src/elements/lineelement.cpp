#include "lineelement.h"

#include <QPainter>
#include <QtMath>
#include <cmath>

namespace editor {

LineElement::LineElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    borderColor_ = QColor(33, 37, 41);
    borderWidth_ = 2.0;
    updateBounds();
}

void LineElement::setP1(const QPointF &p) {
    if (p1_ != p) {
        prepareGeometryChange();
        p1_ = p;
        updateBounds();
        emit geometryChanged();
        emit propertyChanged();
    }
}

void LineElement::setP2(const QPointF &p) {
    if (p2_ != p) {
        prepareGeometryChange();
        p2_ = p;
        updateBounds();
        emit geometryChanged();
        emit propertyChanged();
    }
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
    qreal minX = qMin(p1_.x(), p2_.x());
    qreal minY = qMin(p1_.y(), p2_.y());
    qreal maxX = qMax(p1_.x(), p2_.x());
    qreal maxY = qMax(p1_.y(), p2_.y());

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

    QRectF hr1 = handleRect(ResizeHandle::Point1).adjusted(-3, -3, 3, 3);
    if (hr1.contains(localPos)) {
        return ResizeHandle::Point1;
    }

    QRectF hr2 = handleRect(ResizeHandle::Point2).adjusted(-3, -3, 3, 3);
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

    QPointF targetPt = currentScenePos;
    if (snapToGrid && gridSize > 1.0) {
        targetPt.setX(std::round(targetPt.x() / gridSize) * gridSize);
        targetPt.setY(std::round(targetPt.y() / gridSize) * gridSize);
    }

    if (handle == ResizeHandle::Point1) {
        p1_ = targetPt - pos();
    } else if (handle == ResizeHandle::Point2) {
        p2_ = targetPt - pos();
    }

    updateBounds();
}

QRectF LineElement::boundingRect() const {
    qreal extra = borderWidth_ + handleSize() + 20.0;
    qreal minX = qMin(p1_.x(), p2_.x());
    qreal minY = qMin(p1_.y(), p2_.y());
    qreal maxX = qMax(p1_.x(), p2_.x());
    qreal maxY = qMax(p1_.y(), p2_.y());
    return QRectF(minX - extra, minY - extra, (maxX - minX) + 2 * extra, (maxY - minY) + 2 * extra);
}

QPainterPath LineElement::shape() const {
    QPainterPath path;
    path.moveTo(p1_);
    path.lineTo(p2_);
    QPainterPathStroker stroker;
    stroker.setWidth(qMax(12.0, borderWidth_ + 8.0));
    return stroker.createStroke(path);
}

static void drawArrowHead(QPainter *painter, const QPointF &from, const QPointF &to, qreal size) {
    qreal dx = to.x() - from.x();
    qreal dy = to.y() - from.y();
    qreal length = std::hypot(dx, dy);
    if (length < 1e-4) return;

    qreal angle = std::atan2(dy, dx);
    const qreal arrowAngle = M_PI / 6.0;

    QPointF p1 = to - QPointF(size * std::cos(angle - arrowAngle), size * std::sin(angle - arrowAngle));
    QPointF p2 = to - QPointF(size * std::cos(angle + arrowAngle), size * std::sin(angle + arrowAngle));

    QPolygonF arrowPolygon;
    arrowPolygon << to << p1 << p2;
    painter->drawPolygon(arrowPolygon);
}

void LineElement::drawShape(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawLine(p1_, p2_);

    qreal arrowSize = qMax(10.0, borderWidth_ * 4.0);

    if (beginMarker_ == LineMarkerType::Arrow) {
        painter->setBrush(borderColor_);
        drawArrowHead(painter, p2_, p1_, arrowSize);
    }
    if (endMarker_ == LineMarkerType::Arrow) {
        painter->setBrush(borderColor_);
        drawArrowHead(painter, p1_, p2_, arrowSize);
    }
}

void LineElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(borderColor_, borderWidth_, borderStyle_, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);

    drawShape(painter);

    if (isSelected()) {
        QPen handlePen(QColor(37, 99, 235), 1.5);
        handlePen.setCosmetic(true);
        painter->setPen(handlePen);
        painter->setBrush(Qt::white);

        painter->drawRect(handleRect(ResizeHandle::Point1));
        painter->drawRect(handleRect(ResizeHandle::Point2));
    }

    painter->restore();
}

} // namespace editor
