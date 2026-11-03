#pragma once

#include "rectangleelement.h"
#include "shapeelement.h"

#include <QVector>

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

    // Точки маршрута для ортогональной линии
    QVector<QPointF> points() const { return points_; }
    void setPoints(const QVector<QPointF> &pts);

    // Привязки к портам прямоугольников
    void setStartConnection(RectangleElement *rect, PortPosition port);
    RectangleElement* startRect() const { return startRect_; }
    PortPosition startPort() const { return startPort_; }

    void setEndConnection(RectangleElement *rect, PortPosition port);
    RectangleElement* endRect() const { return endRect_; }
    PortPosition endPort() const { return endPort_; }

    bool isOrthogonal() const { return isOrthogonal_; }
    void setOrthogonal(bool ortho) { isOrthogonal_ = ortho; update(); }

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
    QVector<QPointF> points_;

    RectangleElement *startRect_ = nullptr;
    PortPosition startPort_ = PortPosition::None;

    RectangleElement *endRect_ = nullptr;
    PortPosition endPort_ = PortPosition::None;

    bool isOrthogonal_ = true;

    LineMarkerType beginMarker_ = LineMarkerType::None;
    LineMarkerType endMarker_ = LineMarkerType::Arrow;

    QPointF initialP1_;
    QPointF initialP2_;
};

} // namespace editor
