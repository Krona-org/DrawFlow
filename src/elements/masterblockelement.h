#pragma once

#include "portposition.h"
#include "shapeelement.h"

#include <array>

namespace editor {

class MasterBlockElement : public ShapeElement {
    Q_OBJECT

public:
    explicit MasterBlockElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent = nullptr);
    ~MasterBlockElement() override = default;

    ElementType elementType() const override { return ElementType::MasterBlock; }

    QRectF rect() const { return rect_; }
    void setRect(const QRectF &r);
    void setRectSize(qreal w, qreal h);

    // Геометрия верхних бейджей
    QRectF leftBadgeRect() const;
    QRectF rightBadgeRect() const;

    // Свойства микрокоманды (по модели K1804 Visual Editor)
    quint16 address() const { return address_; }
    void setAddress(quint16 addr);

    QString stepName() const { return stepName_; }
    void setStepName(const QString &step);

    QString lineText(int index) const;
    void setLineText(int index, const QString &text);
    const std::array<QString, 6>& allLines() const { return lines_; }
    void setAllLines(const std::array<QString, 6> &lines);

    // Микрокомандные поля (M1..M15) для бинарного экспорта
    quint16 mField(int index) const;
    void setMField(int index, quint16 val);

    // Точки подключения (порты) для коннекторов
    PortPosition portAt(const QPointF &localPos) const;
    QPointF portLocalPos(PortPosition port) const;
    QPointF portScenePos(PortPosition port) const;
    QPointF portNormal(PortPosition port) const;

    void setHoveredPort(PortPosition port);
    PortPosition hoveredPort() const { return hoveredPort_; }

    void setShowPortsAlways(bool show);
    bool showPortsAlways() const { return showPortsAlways_; }

    // Изменение размера и границы
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    void startHandleDrag(ResizeHandle handle, const QPointF &startScenePos) override;
    void moveHandle(ResizeHandle handle, const QPointF &currentScenePos, bool snapToGrid = false, qreal gridSize = 20.0) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void drawShape(QPainter *painter) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void drawPortMarker(QPainter *painter, const QPointF &pt, bool isHovered, bool isSnapping);

    quint16 address_ = 1;
    QString stepName_ = QStringLiteral("1");
    std::array<QString, 6> lines_;
    std::array<quint16, 16> mFields_{}; // Индексы 1..15

    bool isHovered_ = false;
    bool showPortsAlways_ = false;
    PortPosition hoveredPort_ = PortPosition::None;
};

} // namespace editor

