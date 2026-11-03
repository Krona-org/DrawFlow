#include "canvasscene.h"
#include "orthogonalrouter.h"

#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <cmath>

namespace editor {

CanvasScene::CanvasScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(-5000, -5000, 10000, 10000);
    connect(this, &QGraphicsScene::selectionChanged, this, &CanvasScene::sceneSelectionChanged);
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

void CanvasScene::setSnapToGrid(bool snap) {
    snapToGrid_ = snap;
}

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
    return QPointF(
        std::round(pt.x() / gridSize_) * gridSize_,
        std::round(pt.y() / gridSize_) * gridSize_
    );
}

void CanvasScene::setActiveTool(ElementType tool) {
    if (activeTool_ != tool) {
        activeTool_ = tool;
        emit activeToolChanged(tool);
    }
}

QList<ShapeElement*> CanvasScene::allElements() const {
    QList<ShapeElement*> list;
    const auto sceneItems = items();
    for (QGraphicsItem *item : sceneItems) {
        if (auto shape = dynamic_cast<ShapeElement*>(item)) {
            list.append(shape);
        }
    }
    return list;
}

ShapeElement* CanvasScene::selectedElement() const {
    const auto sel = selectedItems();
    if (!sel.isEmpty()) {
        return dynamic_cast<ShapeElement*>(sel.first());
    }
    return nullptr;
}

QList<QRectF> CanvasScene::obstacleRects(const QGraphicsItem *exclude1, const QGraphicsItem *exclude2) const {
    QList<QRectF> obstacles;
    for (QGraphicsItem *item : items()) {
        if (item == exclude1 || item == exclude2) continue;
        if (auto rect = dynamic_cast<RectangleElement*>(item)) {
            obstacles.append(rect->sceneBoundingRect());
        }
    }
    return obstacles;
}

QPair<RectangleElement*, PortPosition> CanvasScene::findPortAt(const QPointF &scenePos, RectangleElement *exclude, qreal maxDist) const {
    RectangleElement *bestRect = nullptr;
    PortPosition bestPort = PortPosition::None;
    qreal bestDist = maxDist;

    for (QGraphicsItem *item : items()) {
        if (item == exclude) continue;
        if (auto rect = dynamic_cast<RectangleElement*>(item)) {
            const PortPosition ports[] = {
                PortPosition::Top,
                PortPosition::Right,
                PortPosition::Bottom,
                PortPosition::Left
            };
            for (PortPosition p : ports) {
                QPointF pPos = rect->portScenePos(p);
                qreal dist = std::hypot(pPos.x() - scenePos.x(), pPos.y() - scenePos.y());
                if (dist < bestDist) {
                    bestDist = dist;
                    bestRect = rect;
                    bestPort = p;
                }
            }
        }
    }

    return {bestRect, bestPort};
}

void CanvasScene::setConnectingMode(bool connecting) {
    for (QGraphicsItem *item : items()) {
        if (auto rect = dynamic_cast<RectangleElement*>(item)) {
            rect->setShowPortsAlways(connecting);
        }
    }
}

void CanvasScene::updateConnectedLines(RectangleElement *rect) {
    if (!rect) return;

    for (QGraphicsItem *item : items()) {
        if (auto line = dynamic_cast<LineElement*>(item)) {
            if (line->startRect() == rect || line->endRect() == rect) {
                QPointF startPt = line->p1();
                QPointF startNorm(0, 0);
                if (line->startRect()) {
                    startPt = line->startRect()->portScenePos(line->startPort());
                    startNorm = line->startRect()->portNormal(line->startPort());
                }

                QPointF endPt = line->p2();
                QPointF endNorm(0, 0);
                if (line->endRect()) {
                    endPt = line->endRect()->portScenePos(line->endPort());
                    endNorm = line->endRect()->portNormal(line->endPort());
                }

                auto obstacles = obstacleRects(line->startRect(), line->endRect());
                QVector<QPointF> route = OrthogonalRouter::route(startPt, startNorm, endPt, endNorm, obstacles, 18.0);
                line->setPoints(route);
            }
        }
    }
}

ShapeElement* CanvasScene::addElementAt(ElementType type, const QPointF &scenePos) {
    QPointF pos = snapPoint(scenePos);

    if (type == ElementType::Rectangle) {
        rectangleCount_++;
        QString name = QStringLiteral("Прямоугольник %1").arg(rectangleCount_);
        auto rectItem = new RectangleElement(nextElementId_++, name, QUuid::createUuid());
        rectItem->setPos(pos);
        rectItem->setRect(QRectF(0, 0, 120, 80));
        addItem(rectItem);
        rectItem->setSelected(true);
        emit elementAdded(rectItem);
        return rectItem;
    } else if (type == ElementType::Line) {
        lineCount_++;
        QString name = QStringLiteral("Линия %1").arg(lineCount_);
        auto lineItem = new LineElement(nextElementId_++, name, QUuid::createUuid());
        lineItem->setPos(0, 0);
        lineItem->setP1(pos);
        lineItem->setP2(pos + QPointF(120, 60));
        addItem(lineItem);
        lineItem->setSelected(true);
        emit elementAdded(lineItem);
        return lineItem;
    }
    return nullptr;
}

void CanvasScene::deleteSelectedElements() {
    const auto sel = selectedItems();
    for (QGraphicsItem *item : sel) {
        if (auto shape = dynamic_cast<ShapeElement*>(item)) {
            removeItem(shape);
            emit elementRemoved(shape);
            shape->deleteLater();
        }
    }
}

void CanvasScene::clearAll() {
    clear();
    nextElementId_ = 1;
    rectangleCount_ = 0;
    lineCount_ = 0;
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
    qreal pixelStep = std::hypot(gridSize_ * transform.m11(), gridSize_ * transform.m12());
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
            majorLines.append(QLineF(x, top, x, bottom));
        }
        for (qreal y = startY; y <= bottom; y += effectiveStep) {
            majorLines.append(QLineF(left, y, right, y));
        }
    } else {
        qreal startX = std::floor(left / gridSize_) * gridSize_;
        qreal startY = std::floor(top / gridSize_) * gridSize_;

        for (qreal x = startX; x <= right; x += gridSize_) {
            long long lineIdx = std::llround(x / gridSize_);
            if (lineIdx % majorInterval == 0) {
                majorLines.append(QLineF(x, top, x, bottom));
            } else {
                minorLines.append(QLineF(x, top, x, bottom));
            }
        }
        for (qreal y = startY; y <= bottom; y += gridSize_) {
            long long lineIdx = std::llround(y / gridSize_);
            if (lineIdx % majorInterval == 0) {
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

    QRectF pr = pageRect();
    if (pageFormat_ != PageFormat::Infinite && !pr.isEmpty()) {
        QPen pageBorderPen(QColor(190, 200, 215), 1.0, Qt::DashLine);
        pageBorderPen.setCosmetic(true);
        painter->setPen(pageBorderPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(pr);
    }

    painter->restore();
}

void CanvasScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 1. Проверяем клик по порту подключения прямоугольника (даже при инструменте None / Выбор)
        auto portMatch = findPortAt(event->scenePos(), nullptr, 18.0);
        if (portMatch.first != nullptr && portMatch.second != PortPosition::None) {
            clearSelection();
            isConnectingFromPort_ = true;
            connectStartRect_ = portMatch.first;
            connectStartPort_ = portMatch.second;

            // Включаем отображение портов для примагничивания на ВСЕХ прямоугольниках
            setConnectingMode(true);

            lineCount_++;
            QString name = QStringLiteral("Линия %1").arg(lineCount_);
            currentCreatingLine_ = new LineElement(nextElementId_++, name, QUuid::createUuid());
            currentCreatingLine_->setPos(0, 0);
            currentCreatingLine_->setStartConnection(connectStartRect_, connectStartPort_);
            currentCreatingLine_->setEndMarker(LineMarkerType::Arrow);

            QPointF startPt = connectStartRect_->portScenePos(connectStartPort_);
            QPointF norm = connectStartRect_->portNormal(connectStartPort_);
            QVector<QPointF> initPts = { startPt, startPt + norm * 20.0 };
            currentCreatingLine_->setPoints(initPts);

            addItem(currentCreatingLine_);
            event->accept();
            return;
        }

        // 2. Обычное создание инструментами
        if (activeTool_ != ElementType::None) {
            clearSelection();
            isCreating_ = true;
            createStartPos_ = snapPoint(event->scenePos());

            if (activeTool_ == ElementType::Rectangle) {
                rectangleCount_++;
                QString name = QStringLiteral("Прямоугольник %1").arg(rectangleCount_);
                auto rectItem = new RectangleElement(nextElementId_++, name, QUuid::createUuid());
                rectItem->setPos(createStartPos_);
                rectItem->setRect(QRectF(0, 0, 1, 1));
                addItem(rectItem);
                currentCreatingElement_ = rectItem;
            } else if (activeTool_ == ElementType::Line) {
                lineCount_++;
                QString name = QStringLiteral("Линия %1").arg(lineCount_);
                auto lineItem = new LineElement(nextElementId_++, name, QUuid::createUuid());
                lineItem->setPos(0, 0);
                lineItem->setP1(createStartPos_);
                lineItem->setP2(createStartPos_ + QPointF(1, 1));
                addItem(lineItem);
                currentCreatingElement_ = lineItem;
            }

            event->accept();
            return;
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

void CanvasScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    // А. Ведение линии от порта прямоугольника
    if (isConnectingFromPort_ && currentCreatingLine_ != nullptr) {
        QPointF curPos = event->scenePos();

        // Проверяем привязку к порту другого прямоугольника
        auto targetMatch = findPortAt(curPos, connectStartRect_);
        QPointF targetPt = curPos;
        QPointF targetNormal(0, 0);
        RectangleElement *targetRect = targetMatch.first;
        PortPosition targetPort = targetMatch.second;

        if (targetRect && targetPort != PortPosition::None) {
            targetPt = targetRect->portScenePos(targetPort);
            targetNormal = targetRect->portNormal(targetPort);
            targetRect->setHoveredPort(targetPort);
            lastHoveredRect_ = targetRect;
            lastHoveredPort_ = targetPort;
        } else {
            if (lastHoveredRect_) {
                lastHoveredRect_->setHoveredPort(PortPosition::None);
                lastHoveredRect_ = nullptr;
                lastHoveredPort_ = PortPosition::None;
            }
            if (snapToGrid_ && gridSize_ > 1.0) {
                targetPt = snapPoint(targetPt);
            }
        }

        QPointF startPt = connectStartRect_->portScenePos(connectStartPort_);
        QPointF startNorm = connectStartRect_->portNormal(connectStartPort_);

        auto obstacles = obstacleRects(connectStartRect_, targetRect);
        QVector<QPointF> route = OrthogonalRouter::route(startPt, startNorm, targetPt, targetNormal, obstacles, 18.0);
        currentCreatingLine_->setPoints(route);
        currentCreatingLine_->setEndConnection(targetRect, targetPort);

        event->accept();
        return;
    }

    // Б. Создание элементов через перетаскивание
    if (isCreating_ && currentCreatingElement_ != nullptr) {
        QPointF curPos = snapPoint(event->scenePos());
        QPointF delta = curPos - createStartPos_;

        if (auto rectItem = dynamic_cast<RectangleElement*>(currentCreatingElement_)) {
            qreal x = delta.x() >= 0 ? createStartPos_.x() : curPos.x();
            qreal y = delta.y() >= 0 ? createStartPos_.y() : curPos.y();
            qreal w = std::abs(delta.x());
            qreal h = std::abs(delta.y());
            rectItem->setPos(x, y);
            rectItem->setRect(QRectF(0, 0, qMax(10.0, w), qMax(10.0, h)));
        } else if (auto lineItem = dynamic_cast<LineElement*>(currentCreatingElement_)) {
            lineItem->setP2(curPos);
        }

        event->accept();
        return;
    }

    QGraphicsScene::mouseMoveEvent(event);

    // Если перемещается выбранный прямоугольник, обновляем связанные линии
    auto sel = selectedElement();
    if (sel && sel->elementType() == ElementType::Rectangle) {
        updateConnectedLines(dynamic_cast<RectangleElement*>(sel));
    }
}

void CanvasScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    // Завершение ведения линии от порта
    if (isConnectingFromPort_ && currentCreatingLine_ != nullptr) {
        isConnectingFromPort_ = false;
        setConnectingMode(false);

        if (lastHoveredRect_) {
            lastHoveredRect_->setHoveredPort(PortPosition::None);
            lastHoveredRect_ = nullptr;
            lastHoveredPort_ = PortPosition::None;
        }

        bool isUnconnected = (currentCreatingLine_->endRect() == nullptr);
        qreal totalLen = 0.0;
        const auto &pts = currentCreatingLine_->points();
        for (int i = 0; i < pts.size() - 1; ++i) {
            totalLen += std::hypot(pts[i + 1].x() - pts[i].x(), pts[i + 1].y() - pts[i].y());
        }

        if (pts.size() >= 2 && (!isUnconnected || totalLen >= 25.0)) {
            currentCreatingLine_->setSelected(true);
            emit elementAdded(currentCreatingLine_);
        } else {
            removeItem(currentCreatingLine_);
            delete currentCreatingLine_;
        }

        currentCreatingLine_ = nullptr;
        connectStartRect_ = nullptr;
        connectStartPort_ = PortPosition::None;

        event->accept();
        return;
    }

    // Завершение создания элемента
    if (isCreating_ && currentCreatingElement_ != nullptr) {
        isCreating_ = false;

        if (auto rectItem = dynamic_cast<RectangleElement*>(currentCreatingElement_)) {
            if (rectItem->rect().width() < 15.0 || rectItem->rect().height() < 15.0) {
                rectItem->setRect(QRectF(0, 0, 120, 80));
            }
        } else if (auto lineItem = dynamic_cast<LineElement*>(currentCreatingElement_)) {
            if (QLineF(lineItem->p1(), lineItem->p2()).length() < 15.0) {
                lineItem->setP2(lineItem->p1() + QPointF(120, 60));
            }
        }

        currentCreatingElement_->setSelected(true);
        emit elementAdded(currentCreatingElement_);
        currentCreatingElement_ = nullptr;

        setActiveTool(ElementType::None);

        event->accept();
        return;
    }

    QGraphicsScene::mouseReleaseEvent(event);

    auto sel = selectedElement();
    if (sel && sel->elementType() == ElementType::Rectangle) {
        updateConnectedLines(dynamic_cast<RectangleElement*>(sel));
    }
}

void CanvasScene::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelectedElements();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (isConnectingFromPort_ && currentCreatingLine_ != nullptr) {
            isConnectingFromPort_ = false;
            setConnectingMode(false);
            if (lastHoveredRect_) {
                lastHoveredRect_->setHoveredPort(PortPosition::None);
                lastHoveredRect_ = nullptr;
            }
            removeItem(currentCreatingLine_);
            delete currentCreatingLine_;
            currentCreatingLine_ = nullptr;
            connectStartRect_ = nullptr;
        }
        setActiveTool(ElementType::None);
        clearSelection();
        event->accept();
        return;
    }
    QGraphicsScene::keyPressEvent(event);
}

} // namespace editor
