#pragma once

#include "shapeelement.h"

namespace editor {

enum class LineMarkerType {
    None = 0,
    Arrow
};

class LineElement : public ShapeElement {
    Q_OBJECT

public:
    explicit LineElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent = nullptr);
    ~LineElement() override = default;

    ElementType elementType() const override { return ElementType::Line; }

    QPointF p1() const { return p1_; }
    void setP1(const QPointF &p);

    QPointF p2() const { return p2_; }
    void setP2(const QPointF &p);

    LineMarkerType beginMarker() const { return beginMarker_; }
    void setBeginMarker(LineMarkerType marker);

    LineMarkerType endMarker() const { return endMarker_; }
    void setEndMarker(LineMarkerType marker);

    ResizeHandle handleAt(const QPointF &localPos) const override;
    void startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) override;
    void moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid = false, qreal gridSize = 20.0) override;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void drawShape(QPainter *painter) override;
    QRectF handleRect(ResizeHandle handle) const override;

private:
    void updateBounds();

    QPointF p1_{0, 0};
    QPointF p2_{120, 60};

    LineMarkerType beginMarker_ = LineMarkerType::None;
    LineMarkerType endMarker_ = LineMarkerType::None;

    QPointF initialP1_;
    QPointF initialP2_;
};

} // namespace editor
