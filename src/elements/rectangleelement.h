#pragma once

#include "shapeelement.h"

namespace editor {

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

    void startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) override;
    void moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid = false, qreal gridSize = 20.0) override;

protected:
    void drawShape(QPainter *painter) override;

private:
    qreal rounding_ = 0.0;
};

} // namespace editor
