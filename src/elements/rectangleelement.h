#pragma once

#include "shapeelement.h"

namespace editor {

enum class PortPosition {
    None = 0,
    Top,
    Right,
    Bottom,
    Left
};

class RectangleElement : public ShapeElement {
    Q_OBJECT

public:
    explicit RectangleElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent = nullptr);
    ~RectangleElement() override = default;

    ElementType elementType() const override { return ElementType::Rectangle; }

    qreal rounding() const { return rounding_; }
    void setRounding(qreal r);

    QRectF rect() const { return rect_; }
    void setRect(const QRectF &r);
    void setRectSize(qreal w, qreal h);

    // Точки подключения (порты) для линий
    PortPosition portAt(const QPointF &localPos) const;
    QPointF portLocalPos(PortPosition port) const;
    QPointF portScenePos(PortPosition port) const;
    QPointF portNormal(PortPosition port) const;

    void setHoveredPort(PortPosition port);
    PortPosition hoveredPort() const { return hoveredPort_; }

    void startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) override;
    void moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid = false, qreal gridSize = 20.0) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void drawShape(QPainter *painter) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    qreal rounding_ = 0.0;
    bool isHovered_ = false;
    PortPosition hoveredPort_ = PortPosition::None;
};

} // namespace editor
