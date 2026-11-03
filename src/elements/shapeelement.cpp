#include "shapeelement.h"
#include "src/canvas/canvasscene.h"

#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace editor {

ShapeElement::ShapeElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , id_(id)
    , name_(name)
    , uid_(uid.isNull() ? QUuid::createUuid() : uid)
{
    setFlags(QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
}

void ShapeElement::setName(const QString &name) {
    if (name_ != name) {
        name_ = name;
        emit propertyChanged();
    }
}

void ShapeElement::setBorderColor(const QColor &color) {
    if (borderColor_ != color) {
        borderColor_ = color;
        update();
        emit propertyChanged();
    }
}

void ShapeElement::setBorderWidth(qreal width) {
    if (!qFuzzyCompare(borderWidth_, width)) {
        borderWidth_ = qMax(0.5, width);
        prepareGeometryChange();
        update();
        emit propertyChanged();
    }
}

void ShapeElement::setBorderStyle(Qt::PenStyle style) {
    if (borderStyle_ != style) {
        borderStyle_ = style;
        update();
        emit propertyChanged();
    }
}

void ShapeElement::setFillColor(const QColor &color) {
    if (fillColor_ != color) {
        fillColor_ = color;
        update();
        emit propertyChanged();
    }
}

void ShapeElement::setFillStyle(Qt::BrushStyle style) {
    if (fillStyle_ != style) {
        fillStyle_ = style;
        update();
        emit propertyChanged();
    }
}

qreal ShapeElement::handleSize() const {
    return 9.0;
}

QRectF ShapeElement::handleRect(ResizeHandle handle) const {
    const qreal hs = handleSize();
    const qreal half = hs / 2.0;
    const QRectF r = rect_.normalized();

    switch (handle) {
    case ResizeHandle::TopLeft:
        return QRectF(r.left() - half, r.top() - half, hs, hs);
    case ResizeHandle::Top:
        return QRectF(r.center().x() - half, r.top() - half, hs, hs);
    case ResizeHandle::TopRight:
        return QRectF(r.right() - half, r.top() - half, hs, hs);
    case ResizeHandle::Right:
        return QRectF(r.right() - half, r.center().y() - half, hs, hs);
    case ResizeHandle::BottomRight:
        return QRectF(r.right() - half, r.bottom() - half, hs, hs);
    case ResizeHandle::Bottom:
        return QRectF(r.center().x() - half, r.bottom() - half, hs, hs);
    case ResizeHandle::BottomLeft:
        return QRectF(r.left() - half, r.bottom() - half, hs, hs);
    case ResizeHandle::Left:
        return QRectF(r.left() - half, r.center().y() - half, hs, hs);
    case ResizeHandle::Rotate:
    default:
        return QRectF();
    }
}

ResizeHandle ShapeElement::handleAt(const QPointF &localPos) const {
    if (!isSelected()) {
        return ResizeHandle::None;
    }

    static const ResizeHandle handles[] = {
        ResizeHandle::TopLeft, ResizeHandle::Top, ResizeHandle::TopRight,
        ResizeHandle::Right, ResizeHandle::BottomRight, ResizeHandle::Bottom,
        ResizeHandle::BottomLeft, ResizeHandle::Left
    };

    for (ResizeHandle h : handles) {
        // Удобная зона захвата 17x17 пикселей
        QRectF hr = handleRect(h).adjusted(-4, -4, 4, 4);
        if (hr.contains(localPos)) {
            return h;
        }
    }
    return ResizeHandle::None;
}

void ShapeElement::startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) {
    Q_UNUSED(handle);
    Q_UNUSED(startScenePos);
    initialPos_ = pos();
    initialRect_ = rect_;
}

void ShapeElement::moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid, qreal gridSize) {
    Q_UNUSED(handle);
    Q_UNUSED(currentScenePos);
    Q_UNUSED(snapToGrid);
    Q_UNUSED(gridSize);
}

QRectF ShapeElement::boundingRect() const {
    qreal extra = borderWidth_ / 2.0 + handleSize() + 8.0;
    return rect_.normalized().adjusted(-extra, -extra, extra, extra);
}

QPainterPath ShapeElement::shape() const {
    QPainterPath path;
    path.addRect(rect_.normalized());
    if (isSelected()) {
        static const ResizeHandle handles[] = {
            ResizeHandle::TopLeft, ResizeHandle::Top, ResizeHandle::TopRight,
            ResizeHandle::Right, ResizeHandle::BottomRight, ResizeHandle::Bottom,
            ResizeHandle::BottomLeft, ResizeHandle::Left
        };
        for (ResizeHandle h : handles) {
            path.addRect(handleRect(h).adjusted(-4, -4, 4, 4));
        }
    }
    return path;
}

QVariant ShapeElement::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemSelectedChange) {
        bool willSelect = value.toBool();
        if (willSelect && !isSelected()) {
            baseZValue_ = zValue();
            setZValue(baseZValue_ + 1000.0);
        } else if (!willSelect && isSelected()) {
            setZValue(baseZValue_);
        }
    }
    if (change == ItemPositionChange && scene()) {
        if (auto s = dynamic_cast<CanvasScene*>(scene())) {
            if (s->isSnapToGrid()) {
                QPointF newPos = value.toPointF();
                return s->snapPoint(newPos);
            }
        }
    }
    if (change == ItemPositionHasChanged) {
        emit geometryChanged();
        emit propertyChanged();
    }
    return QGraphicsObject::itemChange(change, value);
}

void ShapeElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(QPen(borderColor_, borderWidth_, borderStyle_, Qt::SquareCap, Qt::MiterJoin));
    painter->setBrush(QBrush(fillColor_, fillStyle_));

    drawShape(painter);

    // Отрисовка рамки выделения и маркеров в стиле Precision Blue
    if (isSelected()) {
        const QRectF r = rect_.normalized();

        // Четкая синяя рамка выделения
        QPen selectPen(QColor(37, 99, 235), 1.0, Qt::DashLine);
        selectPen.setCosmetic(true);
        painter->setPen(selectPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);

        // Маркеры с белой заливкой и акцентной синей рамкой
        QPen handlePen(QColor(37, 99, 235), 1.5);
        handlePen.setCosmetic(true);
        painter->setPen(handlePen);
        painter->setBrush(Qt::white);

        static const ResizeHandle handles[] = {
            ResizeHandle::TopLeft, ResizeHandle::Top, ResizeHandle::TopRight,
            ResizeHandle::Right, ResizeHandle::BottomRight, ResizeHandle::Bottom,
            ResizeHandle::BottomLeft, ResizeHandle::Left
        };

        for (ResizeHandle h : handles) {
            painter->drawRect(handleRect(h));
        }
    }

    painter->restore();
}

void ShapeElement::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isSelected()) {
        ResizeHandle h = handleAt(event->pos());
        if (h != ResizeHandle::None) {
            activeHandle_ = h;
            isDraggingHandle_ = true;
            dragStartScenePos_ = event->scenePos();
            startHandleDrag(h, event->scenePos());
            event->accept();
            return;
        }
    }
    isDraggingHandle_ = false;
    QGraphicsObject::mousePressEvent(event);
}

void ShapeElement::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (isDraggingHandle_) {
        bool snap = false;
        qreal gSize = 20.0;
        if (auto s = dynamic_cast<CanvasScene*>(scene())) {
            snap = s->isSnapToGrid();
            gSize = s->gridSize();
        }
        moveHandle(activeHandle_, event->scenePos(), snap, gSize);
        emit geometryChanged();
        emit propertyChanged();
        event->accept();
        return;
    }
    QGraphicsObject::mouseMoveEvent(event);
}

void ShapeElement::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (isDraggingHandle_) {
        isDraggingHandle_ = false;
        activeHandle_ = ResizeHandle::None;
        event->accept();
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
}

void ShapeElement::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    if (isSelected()) {
        ResizeHandle h = handleAt(event->pos());
        switch (h) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case ResizeHandle::TopRight:
        case ResizeHandle::BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case ResizeHandle::Top:
        case ResizeHandle::Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case ResizeHandle::Left:
        case ResizeHandle::Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case ResizeHandle::Point1:
        case ResizeHandle::Point2:
            setCursor(Qt::CrossCursor);
            break;
        default:
            setCursor(Qt::SizeAllCursor);
            break;
        }
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsObject::hoverMoveEvent(event);
}

void ShapeElement::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    setCursor(Qt::ArrowCursor);
    QGraphicsObject::hoverLeaveEvent(event);
}

} // namespace editor
