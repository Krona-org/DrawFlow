#include "canvasscene.h"
#include "orthogonalrouter.h"
#include "src/frames/bParamsDialog/data/blockparamsdialog.h"
#include "undocommands.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QPainter>
#include <cmath>

namespace editor {

CanvasScene::SceneClipboard CanvasScene::s_clipboard_;

bool CanvasScene::hasClipboardContent() {
  return !s_clipboard_.blocks.isEmpty() || !s_clipboard_.connectors.isEmpty() ||
         !s_clipboard_.masks.isEmpty();
}

void CanvasScene::copySelection() {
  const auto sel = selectedItems();
  if (sel.isEmpty())
    return;

  s_clipboard_.blocks.clear();
  s_clipboard_.connectors.clear();
  s_clipboard_.masks.clear();
  s_clipboard_.pasteCount = 0;

  for (QGraphicsItem *item : sel) {
    if (auto block = dynamic_cast<MasterBlockElement *>(item)) {
      MasterBlockClipboardData data;
      data.originalId = block->id();
      data.originalPtr = block;
      data.name = block->name();
      data.pos = block->pos();
      data.rect = block->rect();
      data.address = block->address();
      data.stepName = block->stepName();
      data.lines = block->allLines();
      for (int i = 1; i <= 15; ++i) {
        data.mFields[i] = block->mField(i);
      }
      data.borderColor = block->borderColor();
      data.borderWidth = block->borderWidth();
      data.borderStyle = block->borderStyle();
      data.fillColor = block->fillColor();
      data.fillStyle = block->fillStyle();
      data.zValue = block->zValue();
      s_clipboard_.blocks.append(data);
    } else if (auto conn = dynamic_cast<ConnectorElement *>(item)) {
      ConnectorClipboardData data;
      data.originalId = conn->id();
      data.name = conn->name();
      data.pos = conn->pos();
      data.p1 = conn->p1();
      data.p2 = conn->p2();
      data.points = conn->points();
      data.startBlock = conn->startBlock();
      data.startPort = conn->startPort();
      data.endBlock = conn->endBlock();
      data.endPort = conn->endPort();
      data.isOrthogonal = conn->isOrthogonal();
      data.beginMarker = conn->beginMarker();
      data.endMarker = conn->endMarker();
      data.borderColor = conn->borderColor();
      data.borderWidth = conn->borderWidth();
      data.borderStyle = conn->borderStyle();
      data.zValue = conn->zValue();
      s_clipboard_.connectors.append(data);
    } else if (auto mask = dynamic_cast<MaskElement *>(item)) {
      MaskClipboardData data;
      data.name = mask->name();
      data.pos = mask->pos();
      data.rect = mask->rect();
      data.maskText = mask->maskText();
      data.borderColor = mask->borderColor();
      data.borderWidth = mask->borderWidth();
      data.borderStyle = mask->borderStyle();
      data.fillColor = mask->fillColor();
      data.fillStyle = mask->fillStyle();
      data.zValue = mask->zValue();
      s_clipboard_.masks.append(data);
    }
  }
}

void CanvasScene::cutSelection() {
  copySelection();
  deleteSelectedElements();
}

void CanvasScene::pasteClipboard() {
  if (!hasClipboardContent())
    return;

  s_clipboard_.pasteCount++;
  qreal step = gridSize_ > 5.0 ? gridSize_ : 20.0;
  qreal offsetVal = step * s_clipboard_.pasteCount;
  QPointF offset(offsetVal, offsetVal);

  clearSelection();

  if (undoStack_) {
    undoStack_->beginMacro(QStringLiteral("Вставка элементов"));
  }

  QHash<MasterBlockElement *, MasterBlockElement *> oldToNewBlock;

  // 1. Создаем копии блоков K1804 MasterBlock
  for (const auto &data : s_clipboard_.blocks) {
    masterBlockCount_++;
    QString name = QStringLiteral("MasterBlock %1").arg(masterBlockCount_);
    auto newBlock =
        new MasterBlockElement(nextElementId_++, name, QUuid::createUuid());
    newBlock->setPos(snapPoint(data.pos + offset));
    newBlock->setRect(data.rect);
    newBlock->setAddress(data.address);
    newBlock->setStepName(data.stepName);
    newBlock->setAllLines(data.lines);
    for (int i = 1; i <= 15; ++i) {
      newBlock->setMField(i, data.mFields[i]);
    }
    newBlock->setBorderColor(data.borderColor);
    newBlock->setBorderWidth(data.borderWidth);
    newBlock->setBorderStyle(data.borderStyle);
    newBlock->setFillColor(data.fillColor);
    newBlock->setFillStyle(data.fillStyle);
    newBlock->setZValue(data.zValue);

    oldToNewBlock[data.originalPtr] = newBlock;
    if (undoStack_) {
      undoStack_->push(new AddElementCommand(this, newBlock, false));
    } else {
      addItem(newBlock);
      newBlock->setSelected(true);
      emit elementAdded(newBlock);
    }
  }

  // 2. Создаем копии коннекторов
  for (const auto &data : s_clipboard_.connectors) {
    connectorCount_++;
    QString name = QStringLiteral("Connector %1").arg(connectorCount_);
    auto newConn =
        new ConnectorElement(nextElementId_++, name, QUuid::createUuid());
    newConn->setPos(snapPoint(data.pos + offset));
    newConn->setP1(data.p1);
    newConn->setP2(data.p2);

    QVector<QPointF> newPts;
    for (const QPointF &pt : data.points) {
      newPts.append(pt + offset);
    }
    newConn->setPoints(newPts);

    newConn->setBeginMarker(data.beginMarker);
    newConn->setEndMarker(data.endMarker);
    newConn->setBorderColor(data.borderColor);
    newConn->setBorderWidth(data.borderWidth);
    newConn->setBorderStyle(data.borderStyle);
    newConn->setZValue(data.zValue);

    // Восстанавливаем привязки, если соединенные блоки также скопированы
    MasterBlockElement *mappedStart =
        oldToNewBlock.value(data.startBlock, nullptr);
    MasterBlockElement *mappedEnd = oldToNewBlock.value(data.endBlock, nullptr);

    if (mappedStart) {
      newConn->setStartConnection(mappedStart, data.startPort);
    }
    if (mappedEnd) {
      newConn->setEndConnection(mappedEnd, data.endPort);
    }

    if (undoStack_) {
      undoStack_->push(new AddElementCommand(this, newConn, false));
    } else {
      addItem(newConn);
      newConn->setSelected(true);
      emit elementAdded(newConn);
    }
  }

  // 3. Создаем копии плашек Mask
  for (const auto &data : s_clipboard_.masks) {
    maskCount_++;
    QString name = QStringLiteral("Mask %1").arg(maskCount_);
    auto newMask = new MaskElement(nextElementId_++, name, QUuid::createUuid());
    newMask->setPos(snapPoint(data.pos + offset));
    newMask->setRect(data.rect);
    newMask->setMaskText(data.maskText);
    newMask->setBorderColor(data.borderColor);
    newMask->setBorderWidth(data.borderWidth);
    newMask->setBorderStyle(data.borderStyle);
    newMask->setFillColor(data.fillColor);
    newMask->setFillStyle(data.fillStyle);
    newMask->setZValue(data.zValue);

    if (undoStack_) {
      undoStack_->push(new AddElementCommand(this, newMask, false));
    } else {
      addItem(newMask);
      newMask->setSelected(true);
      emit elementAdded(newMask);
    }
  }

  if (undoStack_) {
    undoStack_->endMacro();
  }

  // 3. Обновляем маршруты связанных коннекторов
  for (auto it = oldToNewBlock.begin(); it != oldToNewBlock.end(); ++it) {
    updateConnectedLines(it.value());
  }
}

void CanvasScene::duplicateSelection() {
  copySelection();
  pasteClipboard();
}

void CanvasScene::selectAllElements() {
  for (QGraphicsItem *item : items()) {
    if (auto shape = dynamic_cast<ShapeElement *>(item)) {
      shape->setSelected(true);
    }
  }
}

CanvasScene::CanvasScene(QObject *parent) : QGraphicsScene(parent) {
  setSceneRect(-5000, -5000, 10000, 10000);
  undoStack_ = new QUndoStack(this);
  connect(this, &QGraphicsScene::selectionChanged, this,
          &CanvasScene::sceneSelectionChanged);
}

void CanvasScene::setGridSize(qreal size) {
  if (size > 2.0 && !qFuzzyCompare(gridSize_, size)) {
    gridSize_ = size;
    update();
    emit gridSizeChanged(size);
  }
}

void CanvasScene::setGridVisible(bool visible) {
  if (showGrid_ != visible) {
    showGrid_ = visible;
    update();
  }
}

void CanvasScene::setSnapToGrid(bool snap) { snapToGrid_ = snap; }

void CanvasScene::setPageFormat(PageFormat format) {
  if (pageFormat_ != format) {
    pageFormat_ = format;
    update();
    emit pageFormatChanged(format);
  }
}

QRectF CanvasScene::pageRect() const {
  switch (pageFormat_) {
  case PageFormat::A4Landscape:
    return QRectF(-700, -500, 1400, 1000);
  case PageFormat::A4Portrait:
    return QRectF(-500, -700, 1000, 1400);
  case PageFormat::Infinite:
  default:
    return QRectF();
  }
}

QPointF CanvasScene::snapPoint(const QPointF &pt) const {
  if (!snapToGrid_ || gridSize_ <= 0.0) {
    return pt;
  }
  return QPointF(std::round(pt.x() / gridSize_) * gridSize_,
                 std::round(pt.y() / gridSize_) * gridSize_);
}

void CanvasScene::setActiveTool(ElementType tool) {
  if (activeTool_ != tool) {
    activeTool_ = tool;
    emit activeToolChanged(tool);
  }
}

QList<ShapeElement *> CanvasScene::allElements() const {
  QList<ShapeElement *> list;
  const auto sceneItems = items();
  for (QGraphicsItem *item : sceneItems) {
    if (auto shape = dynamic_cast<ShapeElement *>(item)) {
      list.append(shape);
    }
  }
  return list;
}

ShapeElement *CanvasScene::selectedElement() const {
  const auto sel = selectedItems();
  if (!sel.isEmpty()) {
    return dynamic_cast<ShapeElement *>(sel.first());
  }
  return nullptr;
}

QList<QRectF> CanvasScene::obstacleRects(const QGraphicsItem *exclude1,
                                         const QGraphicsItem *exclude2) const {
  QList<QRectF> obstacles;
  for (QGraphicsItem *item : items()) {
    if (item == exclude1 || item == exclude2)
      continue;
    if (auto block = dynamic_cast<MasterBlockElement *>(item)) {
      // Используем физическую геометрию блока на сцене
      obstacles.append(block->mapRectToScene(block->rect()));
    }
  }
  return obstacles;
}

QPair<MasterBlockElement *, PortPosition>
CanvasScene::findPortAt(const QPointF &scenePos, MasterBlockElement *exclude,
                        qreal maxDist) const {
  MasterBlockElement *bestBlock = nullptr;
  PortPosition bestPort = PortPosition::None;
  qreal bestDist = maxDist;

  for (QGraphicsItem *item : items()) {
    if (item == exclude)
      continue;
    if (auto block = dynamic_cast<MasterBlockElement *>(item)) {
      const PortPosition ports[] = {PortPosition::InTop,  PortPosition::InMid,
                                    PortPosition::InBot,  PortPosition::OutTop,
                                    PortPosition::OutMid, PortPosition::OutBot};
      for (PortPosition p : ports) {
        QPointF pPos = block->portScenePos(p);
        qreal dist =
            std::hypot(pPos.x() - scenePos.x(), pPos.y() - scenePos.y());
        if (dist < bestDist) {
          bestDist = dist;
          bestBlock = block;
          bestPort = p;
        }
      }
    }
  }
  return {bestBlock, bestPort};
}

void CanvasScene::setConnectingMode(bool connecting) {
  for (QGraphicsItem *item : items()) {
    if (auto block = dynamic_cast<MasterBlockElement *>(item)) {
      block->setShowPortsAlways(connecting);
    }
  }
}

void CanvasScene::openBlockParamsDialog(MasterBlockElement *block) {
  if (!block)
    return;

  QWidget *parentWidget = nullptr;
  if (!views().isEmpty()) {
    parentWidget = views().first()->window();
  }

  BlockParamsDialog dlg(block, this, parentWidget);
  if (dlg.exec() == QDialog::Accepted) {
    emit sceneSelectionChanged();
  }
}

void CanvasScene::updateConnectedLines(MasterBlockElement *block) {
  if (!block)
    return;

  for (QGraphicsItem *item : items()) {
    if (auto conn = dynamic_cast<ConnectorElement *>(item)) {
      if (conn->startBlock() == block || conn->endBlock() == block) {
        QPointF startPt = conn->p1();
        QPointF startNorm(0, 0);
        if (conn->startBlock()) {
          startPt = conn->startBlock()->portScenePos(conn->startPort());
          startNorm = conn->startBlock()->portNormal(conn->startPort());
        }

        QPointF endPt = conn->p2();
        QPointF endNorm(0, 0);
        if (conn->endBlock()) {
          endPt = conn->endBlock()->portScenePos(conn->endPort());
          endNorm = conn->endBlock()->portNormal(conn->endPort());
        }

        auto obstacles = obstacleRects();
        QVector<QPointF> route = OrthogonalRouter::route(
            startPt, startNorm, endPt, endNorm, obstacles, 18.0);
        conn->setPoints(route);
      }
    }
  }
}

ShapeElement *CanvasScene::addElementAt(ElementType type,
                                        const QPointF &scenePos) {
  QPointF pos = snapPoint(scenePos);

  if (type == ElementType::MasterBlock) {
    masterBlockCount_++;
    QString name = QStringLiteral("MasterBlock %1").arg(masterBlockCount_);
    auto blockItem =
        new MasterBlockElement(nextElementId_++, name, QUuid::createUuid());
    blockItem->setPos(pos);
    blockItem->setAddress(static_cast<quint16>(masterBlockCount_));
    blockItem->setStepName(QString::number(masterBlockCount_));
    blockItem->setRect(QRectF(0, 0, 180, 110));
    if (undoStack_) {
      undoStack_->push(new AddElementCommand(this, blockItem, false));
    } else {
      addItem(blockItem);
      blockItem->setSelected(true);
      emit elementAdded(blockItem);
    }
    return blockItem;
  } else if (type == ElementType::Connector) {
    connectorCount_++;
    QString name = QStringLiteral("Connector %1").arg(connectorCount_);
    auto connItem =
        new ConnectorElement(nextElementId_++, name, QUuid::createUuid());
    connItem->setPos(0, 0);
    connItem->setP1(pos);
    connItem->setP2(pos + QPointF(120, 60));
    if (undoStack_) {
      undoStack_->push(new AddElementCommand(this, connItem, false));
    } else {
      addItem(connItem);
      connItem->setSelected(true);
      emit elementAdded(connItem);
    }
    return connItem;
  } else if (type == ElementType::Mask) {
    maskCount_++;
    QString name = QStringLiteral("Mask %1").arg(maskCount_);
    auto maskItem =
        new MaskElement(nextElementId_++, name, QUuid::createUuid());
    maskItem->setPos(pos);
    maskItem->setRect(QRectF(0, 0, 100, 22));
    if (undoStack_) {
      undoStack_->push(new AddElementCommand(this, maskItem, false));
    } else {
      addItem(maskItem);
      maskItem->setSelected(true);
      emit elementAdded(maskItem);
    }
    return maskItem;
  }
  return nullptr;
}

void CanvasScene::deleteSelectedElements() {
  const auto sel = selectedItems();
  QList<ShapeElement *> toDelete;
  for (QGraphicsItem *item : sel) {
    if (auto shape = dynamic_cast<ShapeElement *>(item)) {
      toDelete.append(shape);
    }
  }
  if (toDelete.isEmpty())
    return;

  if (undoStack_) {
    undoStack_->push(new DeleteElementsCommand(this, toDelete));
  } else {
    for (ShapeElement *shape : toDelete) {
      removeItem(shape);
      emit elementRemoved(shape);
      shape->deleteLater();
    }
  }
}

void CanvasScene::clearAll() {
  clear();
  if (undoStack_) {
    undoStack_->clear();
  }
  nextElementId_ = 1;
  masterBlockCount_ = 0;
  connectorCount_ = 0;
  maskCount_ = 0;
}

void CanvasScene::drawBackground(QPainter *painter, const QRectF &rect) {
  painter->fillRect(rect, Qt::white);

  if (!showGrid_ || gridSize_ <= 0.0) {
    return;
  }

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, false);

  QPen minorPen(QColor(238, 240, 244), 1.0);
  minorPen.setCosmetic(true);

  QPen majorPen(QColor(218, 222, 228), 1.0);
  majorPen.setCosmetic(true);

  const QTransform &transform = painter->worldTransform();
  qreal pixelStep =
      std::hypot(gridSize_ * transform.m11(), gridSize_ * transform.m12());
  if (pixelStep <= 0.0) {
    pixelStep = gridSize_;
  }

  int stepMultiplier = 1;
  if (pixelStep < 4.0) {
    stepMultiplier = 5;
    while (pixelStep * stepMultiplier < 4.0 && stepMultiplier <= 500) {
      stepMultiplier *= 5;
    }
  }

  QRectF r = rect.normalized();
  qreal left = r.left();
  qreal right = r.right();
  qreal top = r.top();
  qreal bottom = r.bottom();

  QList<QLineF> minorLines;
  QList<QLineF> majorLines;

  int majorInterval = std::max(2, (int)std::round(100.0 / gridSize_));

  if (stepMultiplier > 1) {
    qreal effectiveStep = gridSize_ * stepMultiplier;
    qreal startX = std::floor(left / effectiveStep) * effectiveStep;
    qreal startY = std::floor(top / effectiveStep) * effectiveStep;

    for (qreal x = startX; x <= right; x += effectiveStep) {
      minorLines.append(QLineF(x, top, x, bottom));
    }
    for (qreal y = startY; y <= bottom; y += effectiveStep) {
      minorLines.append(QLineF(left, y, right, y));
    }
  } else {
    qreal startX = std::floor(left / gridSize_) * gridSize_;
    qreal startY = std::floor(top / gridSize_) * gridSize_;

    for (qreal x = startX; x <= right; x += gridSize_) {
      int gridIndex = (int)std::round(x / gridSize_);
      if (gridIndex % majorInterval == 0) {
        majorLines.append(QLineF(x, top, x, bottom));
      } else {
        minorLines.append(QLineF(x, top, x, bottom));
      }
    }

    for (qreal y = startY; y <= bottom; y += gridSize_) {
      int gridIndex = (int)std::round(y / gridSize_);
      if (gridIndex % majorInterval == 0) {
        majorLines.append(QLineF(left, y, right, y));
      } else {
        minorLines.append(QLineF(left, y, right, y));
      }
    }
  }

  if (!minorLines.isEmpty()) {
    painter->setPen(minorPen);
    painter->drawLines(minorLines.constData(), minorLines.size());
  }
  if (!majorLines.isEmpty()) {
    painter->setPen(majorPen);
    painter->drawLines(majorLines.constData(), majorLines.size());
  }

  painter->restore();
}

void CanvasScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    // 1. Проверяем клик по маркеру трансформации уже выбранного элемента
    for (QGraphicsItem *selItem : selectedItems()) {
      if (auto shape = dynamic_cast<ShapeElement *>(selItem)) {
        QPointF localPos = shape->mapFromScene(event->scenePos());
        ResizeHandle h = shape->handleAt(localPos);
        if (h != ResizeHandle::None) {
          preResizeShape_ = shape;
          if (auto block = dynamic_cast<MasterBlockElement *>(shape)) {
            preResizePos_ = block->pos();
            preResizeRect_ = block->rect();
          } else if (auto conn = dynamic_cast<ConnectorElement *>(shape)) {
            preResizeP1_ = conn->p1();
            preResizeP2_ = conn->p2();
            preResizePoints_ = conn->points();
          }
          preMovePositions_.clear();

          QGraphicsScene::mousePressEvent(event);
          event->accept();
          return;
        }
      }
    }

    // 2. Ведение коннектора от порта блока MasterBlock:
    // Срабатывает только при наведении на порт невыделенного объекта
    auto portMatch = findPortAt(event->scenePos(), nullptr, 14.0);
    if (portMatch.first != nullptr && portMatch.second != PortPosition::None &&
        !portMatch.first->isSelected()) {
      clearSelection();
      isConnectingFromPort_ = true;
      connectStartBlock_ = portMatch.first;
      connectStartPort_ = portMatch.second;

      // Включаем отображение портов для примагничивания на ВСЕХ блоках
      setConnectingMode(true);

      connectorCount_++;
      QString name = QStringLiteral("Connector %1").arg(connectorCount_);
      currentCreatingConnector_ =
          new ConnectorElement(nextElementId_++, name, QUuid::createUuid());
      currentCreatingConnector_->setPos(0, 0);
      currentCreatingConnector_->setStartConnection(connectStartBlock_,
                                                    connectStartPort_);

      QPointF startPt = connectStartBlock_->portScenePos(connectStartPort_);
      QPointF norm = connectStartBlock_->portNormal(connectStartPort_);
      QVector<QPointF> initPts = {startPt, startPt + norm * 20.0};
      currentCreatingConnector_->setPoints(initPts);

      addItem(currentCreatingConnector_);
      event->accept();
      return;
    }

    // 3. Обычное создание инструментами
    if (activeTool_ != ElementType::None) {
      clearSelection();
      isCreating_ = true;
      createStartPos_ = snapPoint(event->scenePos());

      if (activeTool_ == ElementType::MasterBlock) {
        masterBlockCount_++;
        QString name = QStringLiteral("MasterBlock %1").arg(masterBlockCount_);
        auto blockItem =
            new MasterBlockElement(nextElementId_++, name, QUuid::createUuid());
        blockItem->setPos(createStartPos_);
        blockItem->setAddress(static_cast<quint16>(masterBlockCount_));
        blockItem->setStepName(QString::number(masterBlockCount_));
        blockItem->setRect(QRectF(0, 0, 180, 110));
        addItem(blockItem);
        currentCreatingElement_ = blockItem;
      } else if (activeTool_ == ElementType::Connector) {
        connectorCount_++;
        QString name = QStringLiteral("Connector %1").arg(connectorCount_);
        auto connItem =
            new ConnectorElement(nextElementId_++, name, QUuid::createUuid());
        connItem->setPos(0, 0);
        connItem->setP1(createStartPos_);
        connItem->setP2(createStartPos_ + QPointF(1, 1));
        addItem(connItem);
        currentCreatingElement_ = connItem;
      } else if (activeTool_ == ElementType::Mask) {
        maskCount_++;
        QString name = QStringLiteral("Mask %1").arg(maskCount_);
        auto maskItem =
            new MaskElement(nextElementId_++, name, QUuid::createUuid());
        maskItem->setPos(createStartPos_);
        maskItem->setRect(QRectF(0, 0, 100, 22));
        addItem(maskItem);
        currentCreatingElement_ = maskItem;
      }

      event->accept();
      return;
    }
  }

  QGraphicsScene::mousePressEvent(event);

  if (event->button() == Qt::LeftButton) {
    preMovePositions_.clear();
    for (QGraphicsItem *item : selectedItems()) {
      if (auto shape = dynamic_cast<ShapeElement *>(item)) {
        preMovePositions_[shape] = shape->pos();
      }
    }

    preResizeShape_ = nullptr;
    if (selectedItems().size() == 1) {
      ShapeElement *sel = dynamic_cast<ShapeElement *>(selectedItems().first());
      if (auto block = dynamic_cast<MasterBlockElement *>(sel)) {
        preResizeShape_ = block;
        preResizePos_ = block->pos();
        preResizeRect_ = block->rect();
      } else if (auto conn = dynamic_cast<ConnectorElement *>(sel)) {
        preResizeShape_ = conn;
        preResizeP1_ = conn->p1();
        preResizeP2_ = conn->p2();
        preResizePoints_ = conn->points();
      }
    }
  }
}

void CanvasScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  // А. Ведение коннектора от порта блока
  if (isConnectingFromPort_ && currentCreatingConnector_ != nullptr) {
    QPointF curPos = event->scenePos();

    // Проверяем привязку к порту любого блока (кроме того же самого исходного
    // порта)
    auto targetMatch = findPortAt(curPos, nullptr);
    if (targetMatch.first == connectStartBlock_ &&
        targetMatch.second == connectStartPort_) {
      targetMatch = {nullptr, PortPosition::None};
    }

    QPointF targetPt = curPos;
    QPointF targetNormal(0, 0);
    MasterBlockElement *targetBlock = targetMatch.first;
    PortPosition targetPort = targetMatch.second;

    if (targetBlock && targetPort != PortPosition::None) {
      if (lastHoveredBlock_ && lastHoveredBlock_ != targetBlock) {
        lastHoveredBlock_->setHoveredPort(PortPosition::None);
      }
      targetPt = targetBlock->portScenePos(targetPort);
      targetNormal = targetBlock->portNormal(targetPort);
      targetBlock->setHoveredPort(targetPort);
      lastHoveredBlock_ = targetBlock;
      lastHoveredPort_ = targetPort;
    } else {
      if (lastHoveredBlock_) {
        lastHoveredBlock_->setHoveredPort(PortPosition::None);
        lastHoveredBlock_ = nullptr;
        lastHoveredPort_ = PortPosition::None;
      }
      if (snapToGrid_ && gridSize_ > 1.0) {
        targetPt = snapPoint(targetPt);
      }
    }

    QPointF startPt = connectStartBlock_->portScenePos(connectStartPort_);
    QPointF startNorm = connectStartBlock_->portNormal(connectStartPort_);

    auto obstacles = obstacleRects();
    QVector<QPointF> route = OrthogonalRouter::route(
        startPt, startNorm, targetPt, targetNormal, obstacles, 18.0);
    currentCreatingConnector_->setPoints(route);
    currentCreatingConnector_->setEndConnection(targetBlock, targetPort);

    event->accept();
    return;
  }

  // Б. Создание элементов через перетаскивание
  if (isCreating_ && currentCreatingElement_ != nullptr) {
    QPointF curPos = snapPoint(event->scenePos());
    QPointF delta = curPos - createStartPos_;

    if (auto blockItem =
            dynamic_cast<MasterBlockElement *>(currentCreatingElement_)) {
      qreal x = delta.x() >= 0 ? createStartPos_.x() : curPos.x();
      qreal y = delta.y() >= 0 ? createStartPos_.y() : curPos.y();
      qreal w = std::abs(delta.x());
      qreal h = std::abs(delta.y());
      blockItem->setPos(x, y);
      blockItem->setRect(QRectF(0, 0, qMax(120.0, w), qMax(80.0, h)));
    } else if (auto connItem =
                   dynamic_cast<ConnectorElement *>(currentCreatingElement_)) {
      connItem->setP2(curPos);
    } else if (auto maskItem =
                   dynamic_cast<MaskElement *>(currentCreatingElement_)) {
      qreal x = delta.x() >= 0 ? createStartPos_.x() : curPos.x();
      qreal y = delta.y() >= 0 ? createStartPos_.y() : curPos.y();
      qreal w = std::abs(delta.x());
      qreal h = std::abs(delta.y());
      maskItem->setPos(x, y);
      maskItem->setRect(QRectF(0, 0, qMax(80.0, w), qMax(20.0, h)));
    }

    event->accept();
    return;
  }

  QGraphicsScene::mouseMoveEvent(event);

  // Если перемещается выбранный блок, обновляем связанные линии
  auto sel = selectedElement();
  if (sel && sel->elementType() == ElementType::MasterBlock) {
    updateConnectedLines(dynamic_cast<MasterBlockElement *>(sel));
  }
}

void CanvasScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  // Завершение ведения коннектора от порта
  if (isConnectingFromPort_ && currentCreatingConnector_ != nullptr) {
    isConnectingFromPort_ = false;
    setConnectingMode(false);

    if (lastHoveredBlock_) {
      lastHoveredBlock_->setHoveredPort(PortPosition::None);
      lastHoveredBlock_ = nullptr;
      lastHoveredPort_ = PortPosition::None;
    }

    bool isUnconnected = (currentCreatingConnector_->endBlock() == nullptr);
    qreal totalLen = 0.0;
    const auto &pts = currentCreatingConnector_->points();
    for (int i = 0; i < pts.size() - 1; ++i) {
      totalLen +=
          std::hypot(pts[i + 1].x() - pts[i].x(), pts[i + 1].y() - pts[i].y());
    }

    if (pts.size() >= 2 && (!isUnconnected || totalLen >= 25.0)) {
      currentCreatingConnector_->setSelected(true);
      if (undoStack_) {
        undoStack_->push(
            new AddElementCommand(this, currentCreatingConnector_, true));
      } else {
        emit elementAdded(currentCreatingConnector_);
      }
    } else {
      removeItem(currentCreatingConnector_);
      delete currentCreatingConnector_;
    }

    currentCreatingConnector_ = nullptr;
    connectStartBlock_ = nullptr;
    connectStartPort_ = PortPosition::None;

    event->accept();
    return;
  }

  // Завершение создания элемента
  if (isCreating_ && currentCreatingElement_ != nullptr) {
    isCreating_ = false;

    if (auto blockItem =
            dynamic_cast<MasterBlockElement *>(currentCreatingElement_)) {
      if (blockItem->rect().width() < 25.0 ||
          blockItem->rect().height() < 25.0) {
        blockItem->setRect(QRectF(0, 0, 180, 110));
      }
    } else if (auto connItem =
                   dynamic_cast<ConnectorElement *>(currentCreatingElement_)) {
      if (QLineF(connItem->p1(), connItem->p2()).length() < 15.0) {
        connItem->setP2(connItem->p1() + QPointF(120, 60));
      }
    }

    currentCreatingElement_->setSelected(true);
    if (undoStack_) {
      undoStack_->push(
          new AddElementCommand(this, currentCreatingElement_, true));
    } else {
      emit elementAdded(currentCreatingElement_);
    }
    currentCreatingElement_ = nullptr;

    setActiveTool(ElementType::None);

    event->accept();
    return;
  }

  QGraphicsScene::mouseReleaseEvent(event);

  auto sel = selectedElement();
  if (sel && sel->elementType() == ElementType::MasterBlock) {
    updateConnectedLines(dynamic_cast<MasterBlockElement *>(sel));
  }

  // Проверяем изменение размера или перемещение
  if (preResizeShape_ && preResizeShape_->scene() == this) {
    if (auto block = dynamic_cast<MasterBlockElement *>(preResizeShape_)) {
      if (block->pos() != preResizePos_ || block->rect() != preResizeRect_) {
        if (undoStack_) {
          undoStack_->push(new ResizeMasterBlockCommand(
              this, block, preResizePos_, preResizeRect_, block->pos(),
              block->rect()));
        }
        preResizeShape_ = nullptr;
        preMovePositions_.clear();
        return;
      }
    } else if (auto conn = dynamic_cast<ConnectorElement *>(preResizeShape_)) {
      if (conn->p1() != preResizeP1_ || conn->p2() != preResizeP2_ ||
          conn->points() != preResizePoints_) {
        if (undoStack_) {
          undoStack_->push(new ResizeConnectorCommand(
              this, conn, preResizeP1_, preResizeP2_, preResizePoints_,
              conn->p1(), conn->p2(), conn->points()));
        }
        preResizeShape_ = nullptr;
        preMovePositions_.clear();
        return;
      }
    }
  }
  preResizeShape_ = nullptr;

  bool moved = false;
  QHash<ShapeElement *, QPointF> newPositions;
  for (auto it = preMovePositions_.begin(); it != preMovePositions_.end();
       ++it) {
    ShapeElement *elem = it.key();
    if (elem && elem->scene() == this) {
      newPositions[elem] = elem->pos();
      if ((elem->pos() - it.value()).manhattanLength() > 1.0) {
        moved = true;
      }
    }
  }
  if (moved && undoStack_) {
    undoStack_->push(
        new MoveElementsCommand(this, preMovePositions_, newPositions));
  }
  preMovePositions_.clear();
}

void CanvasScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    QGraphicsItem *clickedItem = itemAt(event->scenePos(), QTransform());
    if (auto block = dynamic_cast<MasterBlockElement *>(clickedItem)) {
      clearSelection();
      block->setSelected(true);
      openBlockParamsDialog(block);
      event->accept();
      return;
    } else if (auto mask = dynamic_cast<MaskElement *>(clickedItem)) {
      clearSelection();
      mask->setSelected(true);
      QWidget *parentWidget =
          views().isEmpty() ? nullptr : views().first()->window();
      bool ok = false;
      QString text = QInputDialog::getText(
          parentWidget,
          QStringLiteral("Редактирование маски — %1").arg(mask->name()),
          QStringLiteral("Текст битовой маски (например, 1111111XXX):"),
          QLineEdit::Normal, mask->maskText(), &ok);
      if (ok) {
        mask->setMaskText(text);
        emit sceneSelectionChanged();
      }
      event->accept();
      return;
    }
  }
  QGraphicsScene::mouseDoubleClickEvent(event);
}

void CanvasScene::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    if (auto block = dynamic_cast<MasterBlockElement *>(selectedElement())) {
      openBlockParamsDialog(block);
      event->accept();
      return;
    }
  }
  if (event->matches(QKeySequence::Undo) ||
      ((event->modifiers() & Qt::ControlModifier) &&
       event->key() == Qt::Key_Z &&
       !(event->modifiers() & Qt::ShiftModifier))) {
    if (undoStack_ && undoStack_->canUndo()) {
      undoStack_->undo();
    }
    event->accept();
    return;
  }
  if (event->matches(QKeySequence::Redo) ||
      ((event->modifiers() & Qt::ControlModifier) &&
       event->key() == Qt::Key_Y) ||
      ((event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) ==
           (Qt::ControlModifier | Qt::ShiftModifier) &&
       event->key() == Qt::Key_Z)) {
    if (undoStack_ && undoStack_->canRedo()) {
      undoStack_->redo();
    }
    event->accept();
    return;
  }
  if (event->matches(QKeySequence::Copy) ||
      (event->modifiers() & Qt::ControlModifier &&
       (event->key() == Qt::Key_C || event->key() == Qt::Key_Insert))) {
    copySelection();
    event->accept();
    return;
  }
  if (event->matches(QKeySequence::Paste) ||
      (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_V) ||
      (event->modifiers() & Qt::ShiftModifier &&
       event->key() == Qt::Key_Insert)) {
    pasteClipboard();
    event->accept();
    return;
  }
  if (event->matches(QKeySequence::Cut) ||
      (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_X) ||
      (event->modifiers() & Qt::ShiftModifier &&
       event->key() == Qt::Key_Delete)) {
    cutSelection();
    event->accept();
    return;
  }
  if (event->matches(QKeySequence::SelectAll) ||
      (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_A)) {
    selectAllElements();
    event->accept();
    return;
  }
  if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_D) {
    duplicateSelection();
    event->accept();
    return;
  }
  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
    deleteSelectedElements();
    event->accept();
    return;
  }
  if (event->key() == Qt::Key_Escape) {
    if (isConnectingFromPort_ && currentCreatingConnector_ != nullptr) {
      isConnectingFromPort_ = false;
      setConnectingMode(false);
      if (lastHoveredBlock_) {
        lastHoveredBlock_->setHoveredPort(PortPosition::None);
        lastHoveredBlock_ = nullptr;
      }
      removeItem(currentCreatingConnector_);
      delete currentCreatingConnector_;
      currentCreatingConnector_ = nullptr;
      connectStartBlock_ = nullptr;
    }
    setActiveTool(ElementType::None);
    clearSelection();
    event->accept();
    return;
  }
  QGraphicsScene::keyPressEvent(event);
}

} // namespace editor
