#include "canvasview.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QScrollBar>
#include <QWheelEvent>

namespace editor {

CanvasView::CanvasView(CanvasScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
    , scene_(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setAcceptDrops(true);

    centerOn(0, 0);

    connect(scene_, &CanvasScene::activeToolChanged, this, [this](ElementType tool) {
        if (tool == ElementType::None) {
            setDragMode(QGraphicsView::RubberBandDrag);
            viewport()->setCursor(Qt::ArrowCursor);
        } else {
            setDragMode(QGraphicsView::NoDrag);
            viewport()->setCursor(Qt::CrossCursor);
        }
    });
}

void CanvasView::applyZoom(double factor) {
    double currentScale = transform().m11();
    if ((factor < 1.0 && currentScale < 0.1) || (factor > 1.0 && currentScale > 20.0)) {
        return;
    }
    scale(factor, factor);
    emit zoomChanged(transform().m11());
}

void CanvasView::zoomIn() {
    applyZoom(1.2);
}

void CanvasView::zoomOut() {
    applyZoom(1.0 / 1.2);
}

void CanvasView::resetZoom() {
    resetTransform();
    centerOn(0, 0);
    emit zoomChanged(1.0);
}

void CanvasView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const double numDegrees = event->angleDelta().y() / 8.0;
        const double numSteps = numDegrees / 15.0;
        const double factor = std::pow(1.15, numSteps);
        applyZoom(factor);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void CanvasView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier))) {
        isPanning_ = true;
        panStartPos_ = event->position().toPoint();
        viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent *event) {
    if (isPanning_) {
        QPoint delta = event->position().toPoint() - panStartPos_;
        panStartPos_ = event->position().toPoint();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event) {
    if (isPanning_) {
        isPanning_ = false;
        viewport()->setCursor(scene_->activeTool() == ElementType::None ? Qt::ArrowCursor : Qt::CrossCursor);
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

static const char *kMimeDrawFlow = "application/x-drawflow-element-type";
static const char *kMimeDcsLegacy = "application/x-dcs-element-type";

void CanvasView::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat(QLatin1String(kMimeDrawFlow)) ||
        event->mimeData()->hasFormat(QLatin1String(kMimeDcsLegacy))) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CanvasView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat(QLatin1String(kMimeDrawFlow)) ||
        event->mimeData()->hasFormat(QLatin1String(kMimeDcsLegacy))) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CanvasView::dropEvent(QDropEvent *event) {
    QString format;
    if (event->mimeData()->hasFormat(QLatin1String(kMimeDrawFlow))) {
        format = QLatin1String(kMimeDrawFlow);
    } else if (event->mimeData()->hasFormat(QLatin1String(kMimeDcsLegacy))) {
        format = QLatin1String(kMimeDcsLegacy);
    }

    if (!format.isEmpty()) {
        QByteArray data = event->mimeData()->data(format);
        QString typeStr = QString::fromUtf8(data);

        QPointF scenePos = mapToScene(event->position().toPoint());

        if (typeStr == QStringLiteral("masterblock") || typeStr == QStringLiteral("rectangle")) {
            scene_->addElementAt(ElementType::MasterBlock, scenePos);
        } else if (typeStr == QStringLiteral("connector") || typeStr == QStringLiteral("line")) {
            scene_->addElementAt(ElementType::Connector, scenePos);
        } else if (typeStr == QStringLiteral("mask")) {
            scene_->addElementAt(ElementType::Mask, scenePos);
        }

        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

} // namespace editor
