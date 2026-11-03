#include "rectangleelement.h"

#include <QCursor>
#include <QGraphicsSceneHoverEvent>
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
    setAcceptHoverEvents(true);
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

QPointF RectangleElement::portLocalPos(PortPosition port) const {
    const QRectF r = rect_.normalized();
    switch (port) {
    case PortPosition::Top:
        return QPointF(r.center().x(), r.top());
    case PortPosition::Right:
        return QPointF(r.right(), r.center().y());
    case PortPosition::Bottom:
        return QPointF(r.center().x(), r.bottom());
    case PortPosition::Left:
        return QPointF(r.left(), r.center().y());
    case PortPosition::None:
    default:
        return r.center();
    }
}

QPointF RectangleElement::portScenePos(PortPosition port) const {
    return mapToScene(portLocalPos(port));
}

QPointF RectangleElement::portNormal(PortPosition port) const {
    switch (port) {
    case PortPosition::Top:
        return QPointF(0.0, -1.0);
    case PortPosition::Right:
        return QPointF(1.0, 0.0);
    case PortPosition::Bottom:
        return QPointF(0.0, 1.0);
    case PortPosition::Left:
        return QPointF(-1.0, 0.0);
    case PortPosition::None:
    default:
        return QPointF(0.0, 0.0);
    }
}

PortPosition RectangleElement::portAt(const QPointF &localPos) const {
    const qreal threshold = 12.0;

    const PortPosition ports[] = {
        PortPosition::Top,
        PortPosition::Right,
        PortPosition::Bottom,
        PortPosition::Left
    };

    for (PortPosition p : ports) {
        QPointF pt = portLocalPos(p);
        if (std::hypot(pt.x() - localPos.x(), pt.y() - localPos.y()) <= threshold) {
            return p;
        }
    }
    return PortPosition::None;
}

void RectangleElement::setHoveredPort(PortPosition port) {
    if (hoveredPort_ != port) {
        hoveredPort_ = port;
        update();
    }
}

void RectangleElement::setShowPortsAlways(bool show) {
    if (showPortsAlways_ != show) {
        showPortsAlways_ = show;
        if (!show) {
            hoveredPort_ = PortPosition::None;
        }
        update();
    }
}

void RectangleElement::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    isHovered_ = true;
    if (!isSelected()) {
        hoveredPort_ = portAt(event->pos());
        if (hoveredPort_ != PortPosition::None) {
            setCursor(Qt::CrossCursor);
        }
    } else {
        hoveredPort_ = PortPosition::None;
    }
    update();
    ShapeElement::hoverEnterEvent(event);
}

void RectangleElement::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    if (isSelected()) {
        hoveredPort_ = PortPosition::None;
        ShapeElement::hoverMoveEvent(event);
        return;
    }

    PortPosition p = portAt(event->pos());
    if (p != hoveredPort_) {
        hoveredPort_ = p;
        if (p != PortPosition::None) {
            setCursor(Qt::CrossCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        update();
    }
    ShapeElement::hoverMoveEvent(event);
}

void RectangleElement::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    isHovered_ = false;
    hoveredPort_ = PortPosition::None;
    setCursor(Qt::ArrowCursor);
    update();
    ShapeElement::hoverLeaveEvent(event);
}

void RectangleElement::drawShape(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (rounding_ > 0.0) {
        painter->drawRoundedRect(rect_, rounding_, rounding_);
    } else {
        painter->drawRect(rect_);
    }
}

void RectangleElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // 1. Отрисовка базовой фигуры и маркеров трансформации
    ShapeElement::paint(painter, option, widget);

    // 2. Отрисовка портов подключения:
    // Показываются только когда наводим курсор на НЕВЫДЕЛЕННЫЙ объект или когда активно ведем линию (showPortsAlways_)
    bool showPorts = showPortsAlways_ || (!isSelected() && isHovered_);
    if (showPorts) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        const PortPosition ports[] = {
            PortPosition::Top,
            PortPosition::Right,
            PortPosition::Bottom,
            PortPosition::Left
        };

        for (PortPosition p : ports) {
            QPointF center = portLocalPos(p);
            bool isCurrent = (p == hoveredPort_);

            if (showPortsAlways_) {
                // Режим ведения линии: отчетливые мишени-порты на всех объектах
                if (isCurrent) {
                    // Порт, к которому сейчас происходит примагничивание: яркий зеленый прицел
                    QPen snapPen(QColor(16, 185, 129), 2.0);
                    snapPen.setCosmetic(true);
                    painter->setPen(snapPen);
                    painter->setBrush(QColor(236, 253, 245));
                    painter->drawEllipse(center, 7.0, 7.0);

                    painter->setPen(Qt::NoPen);
                    painter->setBrush(QColor(16, 185, 129));
                    painter->drawEllipse(center, 2.5, 2.5);
                } else {
                    // Доступный для примагничивания порт: синий круг с точкой
                    QPen targetPen(QColor(37, 99, 235), 1.5);
                    targetPen.setCosmetic(true);
                    painter->setPen(targetPen);
                    painter->setBrush(Qt::white);
                    painter->drawEllipse(center, 5.0, 5.0);

                    painter->setPen(Qt::NoPen);
                    painter->setBrush(QColor(37, 99, 235));
                    painter->drawEllipse(center, 1.8, 1.8);
                }
            } else {
                // Обычный режим наведения курсора на невыделенный объект: изящные синие порты
                qreal r = isCurrent ? 5.5 : 4.0;
                QPen portPen(isCurrent ? QColor(37, 99, 235) : QColor(100, 116, 139), isCurrent ? 2.0 : 1.2);
                portPen.setCosmetic(true);

                painter->setPen(portPen);
                painter->setBrush(isCurrent ? QColor(59, 130, 246) : Qt::white);
                painter->drawEllipse(center, r, r);
            }
        }

        painter->restore();
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
    emit geometryChanged();
}

} // namespace editor
