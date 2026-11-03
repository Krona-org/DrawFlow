#pragma once

#include "src/elements/elementtype.h"
#include "src/elements/shapeelement.h"
#include "src/elements/rectangleelement.h"
#include "src/elements/lineelement.h"

#include <QGraphicsScene>
#include <QList>
#include <QRectF>

namespace editor {

enum class PageFormat {
    A4Landscape,
    A4Portrait,
    Infinite
};

class CanvasScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit CanvasScene(QObject *parent = nullptr);
    ~CanvasScene() override = default;

    qreal gridSize() const { return gridSize_; }
    void setGridSize(qreal size);

    bool isGridVisible() const { return showGrid_; }
    void setGridVisible(bool visible);

    bool isSnapToGrid() const { return snapToGrid_; }
    void setSnapToGrid(bool snap);

    QPointF snapPoint(const QPointF &pt) const;

    PageFormat pageFormat() const { return pageFormat_; }
    void setPageFormat(PageFormat format);
    QRectF pageRect() const;

    void setActiveTool(ElementType tool);
    ElementType activeTool() const { return activeTool_; }

    QList<ShapeElement*> allElements() const;
    ShapeElement* selectedElement() const;

    ShapeElement* addElementAt(ElementType type, const QPointF &scenePos);
    void deleteSelectedElements();
    void clearAll();

    // Буфер обмена (Copy / Cut / Paste / Duplicate / Select All)
    void copySelection();
    void cutSelection();
    void pasteClipboard();
    void duplicateSelection();
    void selectAllElements();
    static bool hasClipboardContent();

    // Маршрутизация и обновление связанных линий
    QList<QRectF> obstacleRects(const QGraphicsItem *exclude1 = nullptr, const QGraphicsItem *exclude2 = nullptr) const;
    void updateConnectedLines(RectangleElement *rect);
    QPair<RectangleElement*, PortPosition> findPortAt(const QPointF &scenePos, RectangleElement *exclude = nullptr, qreal maxDist = 24.0) const;
    void setConnectingMode(bool connecting);

signals:
    void elementAdded(ShapeElement *element);
    void elementRemoved(ShapeElement *element);
    void activeToolChanged(ElementType tool);
    void sceneSelectionChanged();
    void gridSizeChanged(qreal size);
    void pageFormatChanged(PageFormat format);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    qreal gridSize_ = 20.0;
    bool showGrid_ = true;
    bool snapToGrid_ = true;
    PageFormat pageFormat_ = PageFormat::Infinite;

    ElementType activeTool_ = ElementType::None;

    bool isCreating_ = false;
    QPointF createStartPos_;
    ShapeElement *currentCreatingElement_ = nullptr;

    // Режим ведения линии от порта прямоугольника
    bool isConnectingFromPort_ = false;
    RectangleElement *connectStartRect_ = nullptr;
    PortPosition connectStartPort_ = PortPosition::None;
    LineElement *currentCreatingLine_ = nullptr;

    RectangleElement *lastHoveredRect_ = nullptr;
    PortPosition lastHoveredPort_ = PortPosition::None;

    struct RectangleClipboardData {
        quint32 originalId = 0;
        RectangleElement *originalPtr = nullptr;
        QString name;
        QPointF pos;
        QRectF rect;
        qreal rounding = 0.0;
        QColor borderColor;
        qreal borderWidth = 1.5;
        Qt::PenStyle borderStyle = Qt::SolidLine;
        QColor fillColor;
        Qt::BrushStyle fillStyle = Qt::SolidPattern;
        qreal zValue = 0.0;
    };

    struct LineClipboardData {
        quint32 originalId = 0;
        QString name;
        QPointF pos;
        QPointF p1;
        QPointF p2;
        QVector<QPointF> points;
        RectangleElement *startRect = nullptr;
        PortPosition startPort = PortPosition::None;
        RectangleElement *endRect = nullptr;
        PortPosition endPort = PortPosition::None;
        bool isOrthogonal = true;
        LineMarkerType beginMarker = LineMarkerType::None;
        LineMarkerType endMarker = LineMarkerType::Arrow;
        QColor borderColor;
        qreal borderWidth = 2.0;
        Qt::PenStyle borderStyle = Qt::SolidLine;
        qreal zValue = 0.0;
    };

    struct SceneClipboard {
        QList<RectangleClipboardData> rects;
        QList<LineClipboardData> lines;
        int pasteCount = 0;
    };

    static SceneClipboard s_clipboard_;

    quint32 nextElementId_ = 1;
    quint32 rectangleCount_ = 0;
    quint32 lineCount_ = 0;
};

} // namespace editor
