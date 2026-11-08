#include "masterblockelement.h"
#include "src/canvas/canvasscene.h"
#include "src/elements/masterBlockElement/k1804microcode.h"

#include <QCursor>
#include <QFont>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <cmath>

namespace editor {

MasterBlockElement::MasterBlockElement(quint32 id, const QString &name,
                                       const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent) {
  rect_ = QRectF(0, 0, 180, 110);
  borderColor_ = QColor(24, 24, 27);
  borderWidth_ = 1.5;
  setAcceptHoverEvents(true);
  setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  // По умолчанию блок пустой: заполнены только номер шага и адрес, микрокоманда не задана
  address_ = static_cast<quint16>(id > 0 ? id : 1);
  stepName_ = QString::number(address_);
  mFields_.fill(0);
  lines_.fill(QString());
}

void MasterBlockElement::setRect(const QRectF &r) {
  if (rect_ != r) {
    prepareGeometryChange();
    rect_ = r;
    update();
    emit geometryChanged();
  }
}

void MasterBlockElement::setRectSize(qreal w, qreal h) {
  QSizeF newSize(qMax(120.0, w), qMax(80.0, h));
  if (rect_.size() != newSize) {
    prepareGeometryChange();
    rect_.setSize(newSize);
    update();
    emit geometryChanged();
  }
}

QRectF MasterBlockElement::leftBadgeRect() const {
  const qreal badgeWidth = 42.0;
  const qreal badgeHeight = 18.0;
  return QRectF(rect_.left() + 8.0, rect_.top() - 9.0, badgeWidth, badgeHeight);
}

QRectF MasterBlockElement::rightBadgeRect() const {
  const qreal badgeWidth = 42.0;
  const qreal badgeHeight = 18.0;
  return QRectF(rect_.right() - badgeWidth - 8.0, rect_.top() - 9.0, badgeWidth,
                badgeHeight);
}

void MasterBlockElement::setAddress(quint16 addr) {
  if (address_ != addr) {
    address_ = addr;
    update();
    emit propertyChanged();
  }
}

void MasterBlockElement::setStepName(const QString &step) {
  if (stepName_ != step) {
    stepName_ = step;
    update();
    emit propertyChanged();
  }
}

QString MasterBlockElement::lineText(int index) const {
  if (index >= 0 && index < 6) {
    return lines_[index];
  }
  return QString();
}

void MasterBlockElement::setLineText(int index, const QString &text) {
  if (index >= 0 && index < 6 && lines_[index] != text) {
    lines_[index] = text;
    update();
    emit propertyChanged();
  }
}

void MasterBlockElement::setAllLines(const std::array<QString, 6> &lines) {
  if (lines_ != lines) {
    lines_ = lines;
    update();
    emit propertyChanged();
  }
}

quint16 MasterBlockElement::mField(int index) const {
  if (index >= 1 && index <= 15) {
    return mFields_[index];
  }
  return 0;
}

void MasterBlockElement::setMField(int index, quint16 val) {
  if (index >= 1 && index <= 15 && mFields_[index] != val) {
    mFields_[index] = val;
    emit propertyChanged();
  }
}

void MasterBlockElement::setAllMFields(const std::array<quint16, 16> &fields) {
  if (mFields_ != fields) {
    mFields_ = fields;
    lines_ = K1804Microcode::generateBlockLines(mFields_);
    update();
    emit propertyChanged();
  }
}

void MasterBlockElement::regenerateLinesFromMFields() {
  lines_ = K1804Microcode::generateBlockLines(mFields_);
  update();
  emit propertyChanged();
}

QPointF MasterBlockElement::portLocalPos(PortPosition port) const {
  const QRectF r = rect_.normalized();
  const qreal h = r.height();
  const qreal yTop = r.top() + h * 0.25;
  const qreal yMid = r.top() + h * 0.50;
  const qreal yBot = r.top() + h * 0.75;

  switch (port) {
  case PortPosition::InTop:
    return QPointF(r.left(), yTop);
  case PortPosition::InMid:
    return QPointF(r.left(), yMid);
  case PortPosition::InBot:
    return QPointF(r.left(), yBot);
  case PortPosition::OutTop:
    return QPointF(r.right(), yTop);
  case PortPosition::OutMid:
    return QPointF(r.right(), yMid);
  case PortPosition::OutBot:
    return QPointF(r.right(), yBot);
  case PortPosition::None:
  default:
    return r.center();
  }
}

QPointF MasterBlockElement::portScenePos(PortPosition port) const {
  return mapToScene(portLocalPos(port));
}

QPointF MasterBlockElement::portNormal(PortPosition port) const {
  if (isInputPort(port)) {
    return QPointF(-1.0, 0.0);
  }
  if (isOutputPort(port)) {
    return QPointF(1.0, 0.0);
  }
  return QPointF(0.0, 0.0);
}

PortPosition MasterBlockElement::portAt(const QPointF &localPos) const {
  const qreal threshold = 12.0;
  const PortPosition ports[] = {PortPosition::InTop,  PortPosition::InMid,
                                PortPosition::InBot,  PortPosition::OutTop,
                                PortPosition::OutMid, PortPosition::OutBot};

  for (PortPosition p : ports) {
    QPointF pt = portLocalPos(p);
    if (std::hypot(pt.x() - localPos.x(), pt.y() - localPos.y()) <= threshold) {
      return p;
    }
  }
  return PortPosition::None;
}

void MasterBlockElement::setHoveredPort(PortPosition port) {
  if (hoveredPort_ != port) {
    hoveredPort_ = port;
    update();
  }
}

void MasterBlockElement::setShowPortsAlways(bool show) {
  if (showPortsAlways_ != show) {
    showPortsAlways_ = show;
    if (!show) {
      hoveredPort_ = PortPosition::None;
    }
    update();
  }
}

void MasterBlockElement::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
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

void MasterBlockElement::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
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

void MasterBlockElement::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  isHovered_ = false;
  hoveredPort_ = PortPosition::None;
  setCursor(Qt::ArrowCursor);
  update();
  ShapeElement::hoverLeaveEvent(event);
}

QRectF MasterBlockElement::boundingRect() const {
  qreal penW = qMax(qreal(1.0), borderWidth_);
  qreal margin = penW + 10.0;
  QRectF totalRect = rect_.united(leftBadgeRect()).united(rightBadgeRect());
  return totalRect.adjusted(-margin, -margin, margin, margin);
}

QPainterPath MasterBlockElement::shape() const {
  QPainterPath path;
  path.addRect(rect_);
  path.addRect(leftBadgeRect());
  path.addRect(rightBadgeRect());
  return path;
}

void MasterBlockElement::drawPortMarker(QPainter *painter, const QPointF &pt,
                                        bool isHovered, bool isSnapping) {
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

  if (isSnapping) {
    // Активный порт примагничивания: зеленый прицел с подсветкой
    QPen haloPen(QColor(16, 185, 129, 90), 4.0);
    haloPen.setCosmetic(true);
    painter->setPen(haloPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(pt, 7.0, 7.0);

    QPen ringPen(QColor(16, 185, 129), 2.0);
    ringPen.setCosmetic(true);
    painter->setPen(ringPen);
    painter->setBrush(QColor(236, 253, 245));
    painter->drawEllipse(pt, 5.0, 5.0);
  } else if (isHovered) {
    // Подсвеченный при наведении порт: синий ореол
    QPen ringPen(QColor(37, 99, 235), 2.0);
    ringPen.setCosmetic(true);
    painter->setPen(ringPen);
    painter->setBrush(QColor(219, 234, 254));
    painter->drawEllipse(pt, 6.0, 6.0);
  } else if (showPortsAlways_) {
    // Режим подключения: полупрозрачные синие точки магнитов на всех портах
    QPen ringPen(QColor(37, 99, 235, 170), 1.5);
    ringPen.setCosmetic(true);
    painter->setPen(ringPen);
    painter->setBrush(QColor(239, 246, 255, 220));
    painter->drawEllipse(pt, 4.5, 4.5);
  }

  // Синий крестик «x» как в оригинале Visio
  QColor crossColor = isSnapping ? QColor(16, 185, 129) : (showPortsAlways_ ? QColor(37, 99, 235) : QColor(0, 70, 228));
  QPen xPen(crossColor, 2.0, Qt::SolidLine, Qt::SquareCap);
  xPen.setCosmetic(true);
  painter->setPen(xPen);

  const qreal d = 3.5;
  painter->drawLine(QPointF(pt.x() - d, pt.y() - d),
                    QPointF(pt.x() + d, pt.y() + d));
  painter->drawLine(QPointF(pt.x() - d, pt.y() + d),
                    QPointF(pt.x() + d, pt.y() - d));

  painter->restore();
}

void MasterBlockElement::drawShape(QPainter *painter) {
  painter->setRenderHint(QPainter::Antialiasing, true);

  QPen mainPen(borderColor_, borderWidth_, borderStyle_, Qt::SquareCap,
               Qt::MiterJoin);
  painter->setPen(mainPen);
  painter->setBrush(fillColor_);

  // 1. Отрисовка основного прямоугольника
  painter->drawRect(rect_);

  // 2. Отрисовка двух верхних бейджей (перекрывают верхнюю грань блока)
  QRectF leftBadge = leftBadgeRect();
  QRectF rightBadge = rightBadgeRect();

  painter->drawRect(leftBadge);
  painter->drawRect(rightBadge);

  // 3. Текст внутри верхних бейджей
  painter->setPen(borderColor_);
  static const QFont badgeFont(QStringLiteral("Segoe UI"), 10, QFont::Bold);
  painter->setFont(badgeFont);

  painter->drawText(leftBadge, Qt::AlignCenter, stepName_);
  painter->drawText(rightBadge, Qt::AlignCenter,
                    K1804Microcode::toOctal(address_));

  // 4. Отрисовка 6 центрированных строк микрокоманды
  static const QFont bodyFont(QStringLiteral("Segoe UI"), 9, QFont::Normal);
  painter->setFont(bodyFont);

  const qreal topMargin = rect_.top() + 16.0;
  const qreal availableHeight = rect_.bottom() - topMargin - 4.0;
  const qreal lineStep = availableHeight / 6.0;

  for (int i = 0; i < 6; ++i) {
    QRectF lineBox(rect_.left() + 8.0, topMargin + i * lineStep,
                   rect_.width() - 16.0, lineStep);
    painter->drawText(lineBox, Qt::AlignCenter, lines_[i]);
  }
}

void MasterBlockElement::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *widget) {
  // 1. Отрисовка базовой геометрии блока
  ShapeElement::paint(painter, option, widget);

  // 2. Отрисовка 6 портов подключения (всегда отображаются крестиками, как в
  // Visio)
  const PortPosition ports[] = {PortPosition::InTop,  PortPosition::InMid,
                                PortPosition::InBot,  PortPosition::OutTop,
                                PortPosition::OutMid, PortPosition::OutBot};

  for (PortPosition p : ports) {
    QPointF pt = portLocalPos(p);
    bool isCurrent = (p == hoveredPort_);
    drawPortMarker(painter, pt, isCurrent, showPortsAlways_ && isCurrent);
  }
}

void MasterBlockElement::startHandleDrag(ResizeHandle handle,
                                         const QPointF &startScenePos) {
  ShapeElement::startHandleDrag(handle, startScenePos);
}

void MasterBlockElement::moveHandle(ResizeHandle handle,
                                    const QPointF &currentScenePos,
                                    bool snapToGrid, qreal gridSize) {
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

  qreal newLeft = sceneLeft;
  qreal newTop = sceneTop;
  qreal newRight = sceneRight;
  qreal newBottom = sceneBottom;

  const qreal minWidth = 140.0;
  const qreal minHeight = 90.0;

  switch (handle) {
  case ResizeHandle::TopLeft:
    newLeft = qMin(targetX, sceneRight - minWidth);
    newTop = qMin(targetY, sceneBottom - minHeight);
    break;
  case ResizeHandle::Top:
    newTop = qMin(targetY, sceneBottom - minHeight);
    break;
  case ResizeHandle::TopRight:
    newRight = qMax(targetX, sceneLeft + minWidth);
    newTop = qMin(targetY, sceneBottom - minHeight);
    break;
  case ResizeHandle::Right:
    newRight = qMax(targetX, sceneLeft + minWidth);
    break;
  case ResizeHandle::BottomRight:
    newRight = qMax(targetX, sceneLeft + minWidth);
    newBottom = qMax(targetY, sceneTop + minHeight);
    break;
  case ResizeHandle::Bottom:
    newBottom = qMax(targetY, sceneTop + minHeight);
    break;
  case ResizeHandle::BottomLeft:
    newLeft = qMin(targetX, sceneRight - minWidth);
    newBottom = qMax(targetY, sceneTop + minHeight);
    break;
  case ResizeHandle::Left:
    newLeft = qMin(targetX, sceneRight - minWidth);
    break;
  default:
    return;
  }

  QRectF newRect(0, 0, newRight - newLeft, newBottom - newTop);
  if (pos() == QPointF(newLeft, newTop) && rect_ == newRect) {
    return;
  }

  prepareGeometryChange();
  setPos(newLeft, newTop);
  rect_ = newRect;
  update();
  if (auto s = dynamic_cast<CanvasScene *>(scene())) {
    s->notifyBlockMoved(this);
  }
  emit geometryChanged();
}

QVariant MasterBlockElement::itemChange(GraphicsItemChange change,
                                        const QVariant &value) {
  if (change == ItemPositionHasChanged) {
    if (auto s = dynamic_cast<CanvasScene *>(scene())) {
      s->notifyBlockMoved(this);
    }
  }
  return ShapeElement::itemChange(change, value);
}

} // namespace editor
