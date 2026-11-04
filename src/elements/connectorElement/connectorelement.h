#pragma once

#include "src/elements/common/portposition.h"
#include "src/elements/masterBlockElement/masterblockelement.h"
#include "src/elements/shapeElement/shapeelement.h"

#include <QVector>

namespace editor {

enum class LineMarkerType { None = 0, Arrow };

class ConnectorElement : public ShapeElement {
  Q_OBJECT

public:
  explicit ConnectorElement(quint32 id, const QString &name, const QUuid &uid,
                            QGraphicsItem *parent = nullptr);
  ~ConnectorElement() override = default;

  ElementType elementType() const override { return ElementType::Connector; }

  QPointF p1() const { return p1_; }
  void setP1(const QPointF &p);

  QPointF p2() const { return p2_; }
  void setP2(const QPointF &p);

  // Точки маршрута для ортогональной линии
  QVector<QPointF> points() const { return points_; }
  void setPoints(const QVector<QPointF> &pts);

  // Привязки к портам блоков K1804 MasterBlock
  void setStartConnection(MasterBlockElement *block, PortPosition port);
  MasterBlockElement *startBlock() const { return startBlock_; }
  PortPosition startPort() const { return startPort_; }

  void setEndConnection(MasterBlockElement *block, PortPosition port);
  MasterBlockElement *endBlock() const { return endBlock_; }
  PortPosition endPort() const { return endPort_; }

  bool isOrthogonal() const { return isOrthogonal_; }
  void setOrthogonal(bool ortho) {
    isOrthogonal_ = ortho;
    update();
  }

  LineMarkerType beginMarker() const { return beginMarker_; }
  void setBeginMarker(LineMarkerType marker);

  LineMarkerType endMarker() const { return endMarker_; }
  void setEndMarker(LineMarkerType marker);

  ResizeHandle handleAt(const QPointF &localPos) const override;
  void startHandleDrag(ResizeHandle handle,
                       const QPointF &startScenePos) override;
  void moveHandle(ResizeHandle handle, const QPointF &currentScenePos,
                  bool snapToGrid = false, qreal gridSize = 20.0) override;

  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

protected:
  void drawShape(QPainter *painter) override;
  QRectF handleRect(ResizeHandle handle) const override;

private:
  void updatePath();
  void drawArrow(QPainter *painter, const QPointF &tip, const QPointF &from);

  QPointF p1_{0, 0};
  QPointF p2_{100, 0};
  QVector<QPointF> points_;

  MasterBlockElement *startBlock_ = nullptr;
  PortPosition startPort_ = PortPosition::None;

  MasterBlockElement *endBlock_ = nullptr;
  PortPosition endPort_ = PortPosition::None;

  bool isOrthogonal_ = true;
  QPointF dragStartScenePos_;
  QPointF initialP1_;
  QPointF initialP2_;
  LineMarkerType beginMarker_ = LineMarkerType::None;
  LineMarkerType endMarker_ = LineMarkerType::None;
};

} // namespace editor
