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
      data.zValue = block->persistentZValue();
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
      data.hasManualOffset = conn->hasManualOffset();
      data.manualMidOffset = conn->manualMidOffset();
      data.beginMarker = conn->beginMarker();
      data.endMarker = conn->endMarker();
      data.borderColor = conn->borderColor();
      data.borderWidth = conn->borderWidth();
      data.borderStyle = conn->borderStyle();
      data.zValue = conn->persistentZValue();
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
      data.zValue = mask->persistentZValue();
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
    quint16 blockNum = nextAvailableMasterBlockNumber();
    QString name = QStringLiteral("MasterBlock %1").arg(blockNum);
    auto newBlock =
        new MasterBlockElement(nextElementId(), name, QUuid::createUuid());
    newBlock->setPos(snapPoint(data.pos + offset));
    newBlock->setRect(data.rect);
    newBlock->setAddress(blockNum);
    newBlock->setStepName(QString::number(blockNum));
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
    quint32 connNum = nextAvailableConnectorNumber();
    QString name = QStringLiteral("Connector %1").arg(connNum);
    auto newConn =
        new ConnectorElement(nextElementId(), name, QUuid::createUuid());
    // Маршрутизатор работает в координатах сцены; положение коннектора
    // оставляем нулевым, перенося смещение в его локальные точки ровно раз.
    const QPointF translatedPos = snapPoint(data.pos + offset);
    newConn->setPos(0, 0);
    QVector<QPointF> newPoints;
    for (const QPointF &point : data.points)
      newPoints.append(point + translatedPos);
    newConn->setPoints(newPoints);
    newConn->setOrthogonal(data.isOrthogonal);
    if (data.hasManualOffset)
      newConn->setManualMidOffset(data.manualMidOffset);

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
    quint32 maskNum = nextAvailableMaskNumber();
    QString name = QStringLiteral("Mask %1").arg(maskNum);
    auto newMask = new MaskElement(nextElementId(), name, QUuid::createUuid());
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
  connect(this, &QGraphicsScene::selectionChanged, this, [this]() {
    bool hasConn = false;
    for (QGraphicsItem *item : selectedItems()) {
      if (dynamic_cast<ConnectorElement *>(item)) {
        hasConn = true;
        break;
      }
    }
    setConnectingMode(hasConn || activeTool_ == ElementType::Connector ||
                      isConnectingFromPort_);
  });
}

CanvasScene::~CanvasScene() {
  // QGraphicsScene уничтожает выбранные элементы уже после деструктора
  // CanvasScene; его signal selectionChanged больше не должен вызывать нас.
  disconnect(this, &QGraphicsScene::selectionChanged, this, nullptr);
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
    bool hasConn = false;
    for (QGraphicsItem *item : selectedItems()) {
      if (dynamic_cast<ConnectorElement *>(item)) {
        hasConn = true;
        break;
      }
    }
    setConnectingMode(tool == ElementType::Connector || hasConn ||
                      isConnectingFromPort_);
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
      // Используем полную геометрию блока вместе с верхними плашками [ 1 ]
      obstacles.append(block->mapRectToScene(block->obstacleRect()));
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

void CanvasScene::notifyBlockMoved(MasterBlockElement *block) {
  if (!block)
    return;
  if (isBatchingMove_) {
    dirtyMovedBlocks_.insert(block);
  } else {
    updateConnectedLinesForBlocks({block});
  }
}

void CanvasScene::updateConnectedLines(MasterBlockElement *block) {
  if (block) {
    updateConnectedLinesForBlocks({block});
  } else {
    updateConnectedLinesForBlocks({});
  }
}

void CanvasScene::updateConnectedLinesForBlocks(
    const QSet<MasterBlockElement *> &movedBlocks) {
  // 1. Собираем коннекторы сцены; если известно, какие блоки сдвинулись,
  // пересчитываем ТОЛЬКО затронутые коннекторы, а остальные сразу берем в existingSegments
  QList<ConnectorElement *> allConns;
  QList<QLineF> existingSegments;

  QList<QRectF> movedZones;
  if (!movedBlocks.isEmpty()) {
    for (MasterBlockElement *mb : movedBlocks) {
      if (mb) {
        movedZones.append(
            mb->mapRectToScene(mb->obstacleRect()).adjusted(-42, -42, 42, 42));
      }
    }
  }

  // Находим все блоки, связанные с перемещенными блоками общим пучком коннекторов
  QSet<MasterBlockElement *> affectedBlocks = movedBlocks;
  if (!movedBlocks.isEmpty()) {
    for (QGraphicsItem *item : items()) {
      if (auto conn = dynamic_cast<ConnectorElement *>(item)) {
        if (conn == currentCreatingConnector_ || conn == currentCreatingElement_)
          continue;
        if (movedBlocks.contains(conn->startBlock()) ||
            movedBlocks.contains(conn->endBlock())) {
          if (conn->startBlock())
            affectedBlocks.insert(conn->startBlock());
          if (conn->endBlock())
            affectedBlocks.insert(conn->endBlock());
        }
      }
    }
  }

  for (QGraphicsItem *item : items()) {
    if (auto conn = dynamic_cast<ConnectorElement *>(item)) {
      if (conn == currentCreatingConnector_ || conn == currentCreatingElement_)
        continue;

      bool isConnected = (conn->startBlock() || conn->endBlock());
      if (!isConnected) {
        const auto &pts = conn->points();
        for (int i = 0; i < pts.size() - 1; ++i) {
          existingSegments.append(QLineF(pts[i], pts[i + 1]));
        }
        continue;
      }

      bool needsUpdate = movedBlocks.isEmpty();
      if (!needsUpdate) {
        if (affectedBlocks.contains(conn->startBlock()) ||
            affectedBlocks.contains(conn->endBlock())) {
          needsUpdate = true;
        } else if (conn->points().size() > 4) {
          // Линия огибала препятствия — проверяем, не освободился ли путь
          needsUpdate = true;
        } else {
          QRectF cb = conn->boundingRect();
          for (const QRectF &zone : movedZones) {
            if (cb.intersects(zone)) {
              needsUpdate = true;
              break;
            }
          }
        }
      }

      if (needsUpdate) {
        allConns.append(conn);
      } else {
        const auto &pts = conn->points();
        for (int i = 0; i < pts.size() - 1; ++i) {
          existingSegments.append(QLineF(pts[i], pts[i + 1]));
        }
      }
    }
  }
  if (allConns.isEmpty())
    return;

  std::sort(allConns.begin(), allConns.end(),
            [](ConnectorElement *a, ConnectorElement *b) {
              return a->id() < b->id();
            });

  auto obstacles = obstacleRects();
  auto appendRouteSegments = [&](const QVector<QPointF> &pts) {
    for (int i = 0; i < pts.size() - 1; ++i) {
      existingSegments.append(QLineF(pts[i], pts[i + 1]));
    }
  };

  // 2. Сначала адаптируем коннекторы с ручной настройкой сегментов (Rubber-banding)
  QSet<ConnectorElement *> handledManual;
  for (ConnectorElement *conn : allConns) {
    if (!conn->hasManualOffset() || !conn->isOrthogonal())
      continue;

    QPointF startPt = conn->startBlock()
                          ? conn->startBlock()->portScenePos(conn->startPort())
                          : conn->p1();
    QPointF startNorm = conn->startBlock()
                            ? conn->startBlock()->portNormal(conn->startPort())
                            : QPointF(0, 0);
    QPointF endPt = conn->endBlock()
                        ? conn->endBlock()->portScenePos(conn->endPort())
                        : conn->p2();
    QPointF endNorm = conn->endBlock()
                          ? conn->endBlock()->portNormal(conn->endPort())
                          : QPointF(0, 0);

    QVector<QPointF> adapted = OrthogonalRouter::adaptManualRoute(
        conn->points(), startPt, startNorm, endPt, endNorm, obstacles, 12.0);
    if (!adapted.isEmpty()) {
      conn->setPoints(adapted);
      appendRouteSegments(adapted);
      handledManual.insert(conn);
    }
  }

  // 3. Группируем автоматические парные коннекторы по каналам (пара блоков + стороны выхода/входа)
  struct ChannelKey {
    MasterBlockElement *bA;
    int sideA;
    MasterBlockElement *bB;
    int sideB;
    bool operator<(const ChannelKey &o) const {
      if (bA != o.bA) return bA < o.bA;
      if (sideA != o.sideA) return sideA < o.sideA;
      if (bB != o.bB) return bB < o.bB;
      return sideB < o.sideB;
    }
  };

  auto portSide = [](PortPosition p) -> int {
    return (p == PortPosition::OutTop || p == PortPosition::OutMid ||
            p == PortPosition::OutBot)
               ? 1
               : 0;
  };

  QMap<ChannelKey, QList<ConnectorElement *>> channelGroups;
  QList<ConnectorElement *> singleConns;

  for (ConnectorElement *conn : allConns) {
    if (handledManual.contains(conn))
      continue;
    MasterBlockElement *b1 = conn->startBlock();
    MasterBlockElement *b2 = conn->endBlock();
    if (b1 && b2) {
      int s1 = portSide(conn->startPort());
      int s2 = portSide(conn->endPort());
      if (b1 > b2 || (b1 == b2 && s1 > s2)) {
        std::swap(b1, b2);
        std::swap(s1, s2);
      }
      channelGroups[{b1, s1, b2, s2}].append(conn);
    } else {
      singleConns.append(conn);
    }
  }

  // 4. Маршрутизация пучков между блоками без перехлестов и наложений
  for (auto it = channelGroups.begin(); it != channelGroups.end(); ++it) {
    QList<ConnectorElement *> &conns = it.value();

    auto leftRightPts = [](ConnectorElement *c) -> QPair<QPointF, QPointF> {
      QPointF pA = c->startBlock()->portScenePos(c->startPort());
      QPointF pB = c->endBlock()->portScenePos(c->endPort());
      if (pA.x() <= pB.x())
        return {pA, pB};
      return {pB, pA};
    };

    // Сортируем пучок сверху вниз по координатам портов
    std::sort(conns.begin(), conns.end(),
              [&](ConnectorElement *a, ConnectorElement *b) {
                auto lrA = leftRightPts(a);
                auto lrB = leftRightPts(b);
                if (std::abs(lrA.first.y() - lrB.first.y()) > 1.0)
                  return lrA.first.y() < lrB.first.y();
                if (std::abs(lrA.second.y() - lrB.second.y()) > 1.0)
                  return lrA.second.y() < lrB.second.y();
                return a->id() < b->id();
              });

    // Определяем общий наклон пучка (идет ли пучок вправо-вверх или вправо-вниз)
    qreal sumLeftY = 0.0, sumRightY = 0.0;
    for (ConnectorElement *c : conns) {
      auto lr = leftRightPts(c);
      sumLeftY += lr.first.y();
      sumRightY += lr.second.y();
    }
    bool goesDown = (sumRightY > sumLeftY + 5.0 * conns.size());

    int total = conns.size();
    for (int k = 0; k < total; ++k) {
      ConnectorElement *conn = conns[k];

      qreal laneOffset = 0.0;
      if (conn->hasManualOffset()) {
        laneOffset = conn->manualMidOffset();
      } else if (total > 1) {
        qreal rawIdx = goesDown ? ((total - 1 - k) - (total - 1) * 0.5)
                                : (k - (total - 1) * 0.5);
        laneOffset = rawIdx * 16.0;
      }

      QPointF startPt = conn->startBlock()->portScenePos(conn->startPort());
      QPointF startNorm = conn->startBlock()->portNormal(conn->startPort());
      QPointF endPt = conn->endBlock()->portScenePos(conn->endPort());
      QPointF endNorm = conn->endBlock()->portNormal(conn->endPort());

      if (conn->isOrthogonal()) {
        QVector<QPointF> route = OrthogonalRouter::route(
            startPt, startNorm, endPt, endNorm, obstacles, 18.0, laneOffset,
            existingSegments);
        conn->setPoints(route);
        appendRouteSegments(route);
      } else {
        conn->setPoints({startPt, endPt});
      }
    }
  }

  // 5. Одиночные коннекторы (один конец привязан, другой свободный)
  for (ConnectorElement *conn : singleConns) {
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

    if (conn->isOrthogonal()) {
      qreal laneOffset =
          conn->hasManualOffset() ? conn->manualMidOffset() : 0.0;
      QVector<QPointF> route = OrthogonalRouter::route(
          startPt, startNorm, endPt, endNorm, obstacles, 18.0, laneOffset,
          existingSegments);
      conn->setPoints(route);
      appendRouteSegments(route);
    } else {
      conn->setPoints({startPt, endPt});
    }
  }
}

quint32 CanvasScene::nextElementId() {
  QSet<quint32> used;
  for (ShapeElement *element : allElements())
    used.insert(element->id());
  while (nextElementId_ == 0 || used.contains(nextElementId_))
    ++nextElementId_;
  return nextElementId_++;
}

quint16 CanvasScene::nextAvailableMasterBlockNumber() const {
  QSet<quint16> used;
  for (QGraphicsItem *item : items()) {
    if (auto *block = dynamic_cast<MasterBlockElement *>(item)) {
      if (block->address() > 0) {
        used.insert(block->address());
      }
      bool ok = false;
      uint stepNum = block->stepName().trimmed().toUInt(&ok);
      if (ok && stepNum > 0 && stepNum <= 65535) {
        used.insert(static_cast<quint16>(stepNum));
      }
      if (block->name().startsWith(QStringLiteral("MasterBlock "))) {
        uint nameNum = block->name().mid(12).trimmed().toUInt(&ok);
        if (ok && nameNum > 0 && nameNum <= 65535) {
          used.insert(static_cast<quint16>(nameNum));
        }
      }
    }
  }
  quint16 candidate = 1;
  while (used.contains(candidate) && candidate < 65535) {
    ++candidate;
  }
  return candidate;
}

quint32 CanvasScene::nextAvailableConnectorNumber() const {
  QSet<quint32> used;
  for (QGraphicsItem *item : items()) {
    if (auto *conn = dynamic_cast<ConnectorElement *>(item)) {
      if (conn->name().startsWith(QStringLiteral("Connector "))) {
        bool ok = false;
        uint num = conn->name().mid(10).trimmed().toUInt(&ok);
        if (ok && num > 0) {
          used.insert(num);
        }
      }
    }
  }
  quint32 candidate = 1;
  while (used.contains(candidate)) {
    ++candidate;
  }
  return candidate;
}

quint32 CanvasScene::nextAvailableMaskNumber() const {
  QSet<quint32> used;
  for (QGraphicsItem *item : items()) {
    if (auto *mask = dynamic_cast<MaskElement *>(item)) {
      if (mask->name().startsWith(QStringLiteral("Mask "))) {
        bool ok = false;
        uint num = mask->name().mid(5).trimmed().toUInt(&ok);
        if (ok && num > 0) {
          used.insert(num);
        }
      }
    }
  }
  quint32 candidate = 1;
  while (used.contains(candidate)) {
    ++candidate;
  }
  return candidate;
}

ShapeElement *CanvasScene::addElementAt(ElementType type,
                                        const QPointF &scenePos) {
  QPointF pos = snapPoint(scenePos);

  if (type == ElementType::MasterBlock) {
    quint16 blockNum = nextAvailableMasterBlockNumber();
    QString name = QStringLiteral("MasterBlock %1").arg(blockNum);
    auto blockItem =
        new MasterBlockElement(nextElementId(), name, QUuid::createUuid());
    blockItem->setPos(pos);
    blockItem->setAddress(blockNum);
    blockItem->setStepName(QString::number(blockNum));
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
    quint32 connNum = nextAvailableConnectorNumber();
    QString name = QStringLiteral("Connector %1").arg(connNum);
    auto connItem =
        new ConnectorElement(nextElementId(), name, QUuid::createUuid());
    connItem->setPos(0, 0);

    auto portMatch = findPortAt(pos, nullptr, 22.0);
    QPointF startPt = pos;
    if (portMatch.first && portMatch.second != PortPosition::None) {
      connItem->setStartConnection(portMatch.first, portMatch.second);
      startPt = portMatch.first->portScenePos(portMatch.second);
    }
    connItem->setP1(startPt);
    connItem->setP2(startPt + QPointF(120, 60));
    connItem->setPoints({startPt, startPt + QPointF(120, 60)});
    if (undoStack_) {
      undoStack_->push(new AddElementCommand(this, connItem, false));
    } else {
      addItem(connItem);
      connItem->setSelected(true);
      emit elementAdded(connItem);
    }
    return connItem;
  } else if (type == ElementType::Mask) {
    quint32 maskNum = nextAvailableMaskNumber();
    QString name = QStringLiteral("Mask %1").arg(maskNum);
    auto maskItem =
        new MaskElement(nextElementId(), name, QUuid::createUuid());
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
  emit sceneCleared();
}

void CanvasScene::drawBackground(QPainter *painter, const QRectF &rect) {
  painter->fillRect(rect, QColor(232, 235, 239));

  if (!showGrid_ || gridSize_ <= 0.0) {
    return;
  }

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, false);

  QPen minorPen(QColor(212, 217, 223), 1.0);
  minorPen.setCosmetic(true);

  QPen majorPen(QColor(190, 197, 205), 1.0);
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
            preResizeConnectorState_ = conn->routeState();
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

      quint32 connNum = nextAvailableConnectorNumber();
      QString name = QStringLiteral("Connector %1").arg(connNum);
      currentCreatingConnector_ =
          new ConnectorElement(nextElementId(), name, QUuid::createUuid());
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
        quint16 blockNum = nextAvailableMasterBlockNumber();
        QString name = QStringLiteral("MasterBlock %1").arg(blockNum);
        auto blockItem =
            new MasterBlockElement(nextElementId(), name, QUuid::createUuid());
        blockItem->setPos(createStartPos_);
        blockItem->setAddress(blockNum);
        blockItem->setStepName(QString::number(blockNum));
        blockItem->setRect(QRectF(0, 0, 180, 110));
        addItem(blockItem);
        currentCreatingElement_ = blockItem;
      } else if (activeTool_ == ElementType::Connector) {
        quint32 connNum = nextAvailableConnectorNumber();
        QString name = QStringLiteral("Connector %1").arg(connNum);
        auto connItem =
            new ConnectorElement(nextElementId(), name, QUuid::createUuid());
        connItem->setPos(0, 0);

        auto portMatch = findPortAt(createStartPos_, nullptr, 22.0);
        QPointF startPt = createStartPos_;
        if (portMatch.first && portMatch.second != PortPosition::None) {
          connItem->setStartConnection(portMatch.first, portMatch.second);
          startPt = portMatch.first->portScenePos(portMatch.second);
        }
        connItem->setP1(startPt);
        connItem->setP2(startPt + QPointF(1, 1));
        connItem->setPoints({startPt, startPt + QPointF(1, 1)});
        addItem(connItem);
        currentCreatingElement_ = connItem;
      } else if (activeTool_ == ElementType::Mask) {
        quint32 maskNum = nextAvailableMaskNumber();
        QString name = QStringLiteral("Mask %1").arg(maskNum);
        auto maskItem =
            new MaskElement(nextElementId(), name, QUuid::createUuid());
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
        preResizeConnectorState_ = conn->routeState();
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
    QList<QLineF> existingSegments;
    int existingCount = 0;
    for (QGraphicsItem *item : items()) {
      if (auto c = dynamic_cast<ConnectorElement *>(item)) {
        if (c == currentCreatingConnector_)
          continue;
        const auto &pts = c->points();
        for (int i = 0; i < pts.size() - 1; ++i) {
          existingSegments.append(QLineF(pts[i], pts[i + 1]));
        }
        if (connectStartBlock_ && targetBlock &&
            ((c->startBlock() == connectStartBlock_ &&
              c->endBlock() == targetBlock) ||
             (c->startBlock() == targetBlock &&
              c->endBlock() == connectStartBlock_))) {
          existingCount++;
        }
      }
    }
    qreal laneOffset = 0.0;
    if (existingCount > 0) {
      laneOffset = (existingCount % 2 == 1)
                       ? ((existingCount + 1) / 2) * 16.0
                       : -(existingCount / 2) * 16.0;
    }
    QVector<QPointF> route = OrthogonalRouter::route(
        startPt, startNorm, targetPt, targetNormal, obstacles, 18.0, laneOffset,
        existingSegments);
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
      QPointF rawCurPos = event->scenePos();
      auto targetMatch = findPortAt(rawCurPos, nullptr, 22.0);
      if (targetMatch.first == connItem->startBlock() &&
          targetMatch.second == connItem->startPort()) {
        targetMatch = {nullptr, PortPosition::None};
      }

      QPointF targetPt = rawCurPos;
      QPointF targetNorm(0, 0);
      MasterBlockElement *targetBlock = targetMatch.first;
      PortPosition targetPort = targetMatch.second;

      if (targetBlock && targetPort != PortPosition::None) {
        if (lastHoveredBlock_ && lastHoveredBlock_ != targetBlock) {
          lastHoveredBlock_->setHoveredPort(PortPosition::None);
        }
        targetPt = targetBlock->portScenePos(targetPort);
        targetNorm = targetBlock->portNormal(targetPort);
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

      connItem->setP2(targetPt);
      connItem->setEndConnection(targetBlock, targetPort);

      QPointF startPt = connItem->p1();
      QPointF startNorm(0, 0);
      if (connItem->startBlock() && connItem->startPort() != PortPosition::None) {
        startPt = connItem->startBlock()->portScenePos(connItem->startPort());
        startNorm = connItem->startBlock()->portNormal(connItem->startPort());
      }
      auto obstacles = obstacleRects();
      QList<QLineF> existingSegments;
      int existingCount = 0;
      for (QGraphicsItem *item : items()) {
        if (auto c = dynamic_cast<ConnectorElement *>(item)) {
          if (c == connItem)
            continue;
          const auto &pts = c->points();
          for (int i = 0; i < pts.size() - 1; ++i) {
            existingSegments.append(QLineF(pts[i], pts[i + 1]));
          }
          if (connItem->startBlock() && targetBlock &&
              ((c->startBlock() == connItem->startBlock() &&
                c->endBlock() == targetBlock) ||
               (c->startBlock() == targetBlock &&
                c->endBlock() == connItem->startBlock()))) {
            existingCount++;
          }
        }
      }
      qreal laneOffset = 0.0;
      if (existingCount > 0) {
        laneOffset = (existingCount % 2 == 1)
                         ? ((existingCount + 1) / 2) * 16.0
                         : -(existingCount / 2) * 16.0;
      }
      QVector<QPointF> route = OrthogonalRouter::route(
          startPt, startNorm, targetPt, targetNorm, obstacles, 18.0, laneOffset,
          existingSegments);
      connItem->setPoints(route);
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

  isBatchingMove_ = true;
  dirtyMovedBlocks_.clear();
  QGraphicsScene::mouseMoveEvent(event);
  isBatchingMove_ = false;

  if (!dirtyMovedBlocks_.isEmpty()) {
    QSet<MasterBlockElement *> moved = std::move(dirtyMovedBlocks_);
    dirtyMovedBlocks_.clear();
    updateConnectedLinesForBlocks(moved);
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
      if (currentCreatingConnector_->startBlock() && currentCreatingConnector_->endBlock()) {
        updateConnectedLines(currentCreatingConnector_->startBlock());
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
      if (lastHoveredBlock_) {
        lastHoveredBlock_->setHoveredPort(PortPosition::None);
        lastHoveredBlock_ = nullptr;
        lastHoveredPort_ = PortPosition::None;
      }
      if (QLineF(connItem->p1(), connItem->p2()).length() < 12.0) {
        removeItem(connItem);
        delete connItem;
        currentCreatingElement_ = nullptr;
        setActiveTool(ElementType::None);
        event->accept();
        return;
      }
    }

    currentCreatingElement_->setSelected(true);
    if (undoStack_) {
      undoStack_->push(
          new AddElementCommand(this, currentCreatingElement_, true));
    } else {
      emit elementAdded(currentCreatingElement_);
    }
    if (auto c = dynamic_cast<ConnectorElement *>(currentCreatingElement_)) {
      if (c->startBlock() && c->endBlock()) {
        updateConnectedLines(c->startBlock());
      }
    }
    currentCreatingElement_ = nullptr;

    setActiveTool(ElementType::None);

    event->accept();
    return;
  }

  isBatchingMove_ = true;
  dirtyMovedBlocks_.clear();
  QGraphicsScene::mouseReleaseEvent(event);
  isBatchingMove_ = false;

  if (!dirtyMovedBlocks_.isEmpty()) {
    QSet<MasterBlockElement *> moved = std::move(dirtyMovedBlocks_);
    dirtyMovedBlocks_.clear();
    updateConnectedLinesForBlocks(moved);
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
      if (conn->routeState() != preResizeConnectorState_) {
        if (undoStack_) {
          undoStack_->push(new ResizeConnectorCommand(
              this, conn, preResizeConnectorState_, conn->routeState()));
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
