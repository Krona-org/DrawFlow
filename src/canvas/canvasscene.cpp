#include "canvasscene.h"

#include "src/elements/lineelement.h"
#include "src/elements/rectangleelement.h"

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
    qreal x = std::round(pt.x() / gridSize_) * gridSize_;
    qreal y = std::round(pt.y() / gridSize_) * gridSize_;
    return QPointF(x, y);
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
    for (QGraphicsItem *item : sel) {
        if (auto shape = dynamic_cast<ShapeElement*>(item)) {
            return shape;
        }
    }
    return nullptr;
}

ShapeElement* CanvasScene::addElementAt(ElementType type, const QPointF &scenePos) {
    clearSelection();
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
        lineItem->setPos(pos);
        lineItem->setP1(QPointF(0, 0));
        lineItem->setP2(QPointF(120, 60));
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
    // Чистый белый фон на всю рабочую область (в точности как в референсе Image 5)
    painter->fillRect(rect, Qt::white);

    if (!showGrid_ || gridSize_ <= 0.0) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    // Четкая и комфортная сетка:
    // Тонкие ячейки: заметный, но мягкий тон
    QPen minorPen(QColor(215, 220, 228), 1.0);
    minorPen.setCosmetic(true);

    // Главные направляющие: более выразительные опорные линии
    QPen majorPen(QColor(176, 186, 200), 1.0);
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

    // Деликатная пунктирная граница формата листа (если выбран формат)
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
    if (event->button() == Qt::LeftButton && activeTool_ != ElementType::None) {
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
            lineItem->setPos(createStartPos_);
            lineItem->setP1(QPointF(0, 0));
            lineItem->setP2(QPointF(1, 1));
            addItem(lineItem);
            currentCreatingElement_ = lineItem;
        }

        event->accept();
        return;
    }

    QGraphicsScene::mousePressEvent(event);
}

void CanvasScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
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
            lineItem->setP2(delta);
        }

        event->accept();
        return;
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void CanvasScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (isCreating_ && currentCreatingElement_ != nullptr) {
        isCreating_ = false;

        if (auto rectItem = dynamic_cast<RectangleElement*>(currentCreatingElement_)) {
            if (rectItem->rect().width() < 15.0 || rectItem->rect().height() < 15.0) {
                rectItem->setRect(QRectF(0, 0, 120, 80));
            }
        } else if (auto lineItem = dynamic_cast<LineElement*>(currentCreatingElement_)) {
            if (QLineF(lineItem->p1(), lineItem->p2()).length() < 15.0) {
                lineItem->setP2(QPointF(120, 60));
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
}

void CanvasScene::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelectedElements();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (activeTool_ != ElementType::None) {
            setActiveTool(ElementType::None);
            if (isCreating_ && currentCreatingElement_ != nullptr) {
                removeItem(currentCreatingElement_);
                currentCreatingElement_->deleteLater();
                currentCreatingElement_ = nullptr;
                isCreating_ = false;
            }
            event->accept();
            return;
        }
    }
    QGraphicsScene::keyPressEvent(event);
}

} // namespace editor
