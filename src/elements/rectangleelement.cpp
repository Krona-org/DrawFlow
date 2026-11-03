#include "rectangleelement.h"

#include <QPainter>
#include <cmath>

namespace editor {

RectangleElement::RectangleElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    rect_ = QRectF(0, 0, 120, 80);
    fillColor_ = Qt::white;
    borderColor_ = QColor(33, 37, 41);
    borderWidth_ = 1.5;
}

void RectangleElement::setRounding(qreal r) {
    if (!qFuzzyCompare(rounding_, r)) {
        rounding_ = qBound(0.0, r, 100.0);
        update();
        emit propertyChanged();
    }
}

void RectangleElement::setRect(const QRectF &r) {
    if (rect_ != r) {
        prepareGeometryChange();
        rect_ = r;
        update();
        emit geometryChanged();
        emit propertyChanged();
    }
}

void RectangleElement::setRectSize(qreal w, qreal h) {
    prepareGeometryChange();
    rect_.setSize(QSizeF(qMax(10.0, w), qMax(10.0, h)));
    update();
    emit geometryChanged();
    emit propertyChanged();
}

void RectangleElement::drawShape(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (rounding_ > 0.0) {
        painter->drawRoundedRect(rect_, rounding_, rounding_);
    } else {
        painter->drawRect(rect_);
    }
}

void RectangleElement::startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) {
    ShapeElement::startHandleDrag(handle, startScenePos);
}

void RectangleElement::moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid, qreal gridSize) {
    qreal sceneLeft = initialPos_.x();
    qreal sceneTop = initialPos_.y();
    qreal sceneRight = sceneLeft + initialRect_.width();
    qreal sceneBottom = sceneTop + initialRect_.height();

    qreal targetX = currentScenePos.x();
    qreal targetY = currentScenePos.y();

    if (snapToGrid && gridSize > 1.0) {
        targetX = std::round(targetX / gridSize) * gridSize;
        targetY = std::round(targetY / gridSize) * gridSize;
    }

    const qreal minSize = 10.0;

    switch (handle) {
    case ResizeHandle::Right:
        sceneRight = qMax(sceneLeft + minSize, targetX);
        break;
    case ResizeHandle::Bottom:
        sceneBottom = qMax(sceneTop + minSize, targetY);
        break;
    case ResizeHandle::BottomRight:
        sceneRight = qMax(sceneLeft + minSize, targetX);
        sceneBottom = qMax(sceneTop + minSize, targetY);
        break;
    case ResizeHandle::Left:
        sceneLeft = qMin(sceneRight - minSize, targetX);
        break;
    case ResizeHandle::Top:
        sceneTop = qMin(sceneBottom - minSize, targetY);
        break;
    case ResizeHandle::TopLeft:
        sceneLeft = qMin(sceneRight - minSize, targetX);
        sceneTop = qMin(sceneBottom - minSize, targetY);
        break;
    case ResizeHandle::TopRight:
        sceneRight = qMax(sceneLeft + minSize, targetX);
        sceneTop = qMin(sceneBottom - minSize, targetY);
        break;
    case ResizeHandle::BottomLeft:
        sceneLeft = qMin(sceneRight - minSize, targetX);
        sceneBottom = qMax(sceneTop + minSize, targetY);
        break;
    default:
        return;
    }

    prepareGeometryChange();
    setPos(sceneLeft, sceneTop);
    rect_ = QRectF(0, 0, sceneRight - sceneLeft, sceneBottom - sceneTop);
    update();
}

} // namespace editor
