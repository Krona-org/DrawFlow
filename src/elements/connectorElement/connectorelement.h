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

  struct RouteState {
    QVector<QPointF> points;
    MasterBlockElement *startBlock = nullptr;
    PortPosition startPort = PortPosition::None;
    MasterBlockElement *endBlock = nullptr;
    PortPosition endPort = PortPosition::None;
    bool hasManualOffset = false;
    qreal manualMidOffset = 0.0;
    bool operator==(const RouteState &other) const {
      return points == other.points && startBlock == other.startBlock &&
             startPort == other.startPort && endBlock == other.endBlock &&
             endPort == other.endPort && hasManualOffset == other.hasManualOffset &&
             manualMidOffset == other.manualMidOffset;
    }
    bool operator!=(const RouteState &other) const { return !(*this == other); }
  };
  RouteState routeState() const;
  void setRouteState(const RouteState &state);

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

  // Ручная регулировка изломов / полос (как в Visio)
  bool hasManualOffset() const { return hasManualOffset_; }
  qreal manualMidOffset() const { return manualMidOffset_; }
  void setManualMidOffset(qreal offset) {
    manualMidOffset_ = offset;
    hasManualOffset_ = true;
  }
  void clearManualOffset() {
    hasManualOffset_ = false;
    manualMidOffset_ = 0.0;
  }

  bool isVerticalSegment(int idx) const;
  int findNearestSegment(const QPointF &pt) const;

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

  void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

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
  bool hasManualOffset_ = false;
  qreal manualMidOffset_ = 0.0;

  bool isDraggingBody_ = false;
  bool hasExceededDragThreshold_ = false;
  int dragBodyNearestSegment_ = -1;
  QPointF dragBodyStartScenePos_;
  QPointF dragBodyInitialP1_;
  QPointF dragBodyInitialP2_;
  QVector<QPointF> dragBodyInitialPoints_;
  QVector<QPointF> dragInitialPoints_;

  QPointF dragStartScenePos_;
  QPointF initialP1_;
  QPointF initialP2_;
  LineMarkerType beginMarker_ = LineMarkerType::None;
  LineMarkerType endMarker_ = LineMarkerType::None;
};

} // namespace editor
