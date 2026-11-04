#pragma once

#include "src/elements/common/elementtype.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsObject>
#include <QPen>
#include <QUuid>

namespace editor {

enum class ResizeHandle {
    None = 0,
    TopLeft,
    Top,
    TopRight,
    Right,
    BottomRight,
    Bottom,
    BottomLeft,
    Left,
    Point1,
    Point2,
    Rotate
};

class ShapeElement : public QGraphicsObject {
    Q_OBJECT

public:
    explicit ShapeElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent = nullptr);
    ~ShapeElement() override = default;

    quint32 id() const { return id_; }
    void setId(quint32 id) { id_ = id; }

    QString name() const { return name_; }
    void setName(const QString &name);

    QUuid uid() const { return uid_; }

    virtual ElementType elementType() const = 0;

    // Стили границы
    QColor borderColor() const { return borderColor_; }
    void setBorderColor(const QColor &color);

    qreal borderWidth() const { return borderWidth_; }
    void setBorderWidth(qreal width);

    Qt::PenStyle borderStyle() const { return borderStyle_; }
    void setBorderStyle(Qt::PenStyle style);

    // Стили заливки
    QColor fillColor() const { return fillColor_; }
    void setFillColor(const QColor &color);

    Qt::BrushStyle fillStyle() const { return fillStyle_; }
    void setFillStyle(Qt::BrushStyle style);

    // Хэндлы изменения размера
    virtual ResizeHandle handleAt(const QPointF &localPos) const;
    virtual void startHandleDrag(ResizeHandle handle, const QPointF &startScenePos);
    virtual void moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid = false, qreal gridSize = 20.0);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void propertyChanged();
    void geometryChanged();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void drawShape(QPainter *painter) = 0;
    virtual QRectF handleRect(ResizeHandle handle) const;
    qreal handleSize() const;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    quint32 id_ = 0;
    QString name_;
    QUuid uid_;

    QRectF rect_;

    QColor borderColor_{QColor(33, 37, 41)};
    qreal borderWidth_ = 1.5;
    Qt::PenStyle borderStyle_ = Qt::SolidLine;

    QColor fillColor_{Qt::white};
    Qt::BrushStyle fillStyle_ = Qt::SolidPattern;

    ResizeHandle activeHandle_ = ResizeHandle::None;
    QPointF dragStartScenePos_;
    QPointF initialPos_;
    QRectF initialRect_;
    bool isDraggingHandle_ = false;
    qreal baseZValue_ = 0.0;
};

} // namespace editor
