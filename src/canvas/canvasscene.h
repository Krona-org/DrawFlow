#pragma once

#include "src/elements/connectorElement/connectorelement.h"
#include "src/elements/common/elementtype.h"
#include "src/elements/maskElement/maskelement.h"
#include "src/elements/masterBlockElement/masterblockelement.h"
#include "src/elements/common/portposition.h"
#include "src/elements/shapeElement/shapeelement.h"

#include <QGraphicsScene>
#include <QList>
#include <QRectF>
#include <QSet>
#include <QUndoStack>

namespace editor {

enum class PageFormat { A4Landscape, A4Portrait, Infinite };

class CanvasScene : public QGraphicsScene {
  Q_OBJECT

public:
  explicit CanvasScene(QObject *parent = nullptr);
  ~CanvasScene() override;

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

  QList<ShapeElement *> allElements() const;
  ShapeElement *selectedElement() const;

  ShapeElement *addElementAt(ElementType type, const QPointF &scenePos);
  void deleteSelectedElements();
  void clearAll();

  QUndoStack *undoStack() const { return undoStack_; }

  // Буфер обмена (Copy / Cut / Paste / Duplicate / Select All)
  void copySelection();
  void cutSelection();
  void pasteClipboard();
  void duplicateSelection();
  void selectAllElements();
  void beginSelectionBatch();
  void endSelectionBatch();
  bool isSelectionBatching() const { return selectionBatchDepth_ > 0; }
  void updateConnectedLinesForBlocks(const QSet<MasterBlockElement *> &movedBlocks);
  static bool hasClipboardContent();

  // Маршрутизация и обновление связанных линий
  QList<QRectF> obstacleRects(const QGraphicsItem *exclude1 = nullptr,
                              const QGraphicsItem *exclude2 = nullptr) const;
  void updateConnectedLines(MasterBlockElement *block = nullptr);
  void notifyBlockMoved(MasterBlockElement *block);
  QPair<MasterBlockElement *, PortPosition>
  findPortAt(const QPointF &scenePos, MasterBlockElement *exclude = nullptr,
             qreal maxDist = 24.0) const;
  void setConnectingMode(bool connecting);
  void openBlockParamsDialog(MasterBlockElement *block);

signals:
  void elementAdded(ShapeElement *element);
  void elementRemoved(ShapeElement *element);
  void sceneCleared();
  void activeToolChanged(ElementType tool);
  void sceneSelectionChanged();
  void selectionBatchFinished();
  void gridSizeChanged(qreal size);
  void pageFormatChanged(PageFormat format);

protected:
  void drawBackground(QPainter *painter, const QRectF &rect) override;
  void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private:
  qreal gridSize_ = 20.0; ///< Шаг сетки по умолчанию
  bool showGrid_ = true;
  bool snapToGrid_ = true;
  PageFormat pageFormat_ = PageFormat::Infinite;

  ElementType activeTool_ = ElementType::None;

  bool isCreating_ = false;
  QPointF createStartPos_;
  ShapeElement *currentCreatingElement_ = nullptr;

  // Режим ведения коннектора от порта блока
  bool isConnectingFromPort_ = false;
  MasterBlockElement *connectStartBlock_ = nullptr;
  PortPosition connectStartPort_ = PortPosition::None;
  ConnectorElement *currentCreatingConnector_ = nullptr;

  MasterBlockElement *lastHoveredBlock_ = nullptr;
  PortPosition lastHoveredPort_ = PortPosition::None;

  bool isBatchingMove_ = false;
  int selectionBatchDepth_ = 0;
  bool selectionBatchChanged_ = false;
  QSet<MasterBlockElement *> dirtyMovedBlocks_;

  quint32 nextElementId();
  quint16 nextAvailableMasterBlockNumber() const;
  quint32 nextAvailableConnectorNumber() const;
  quint32 nextAvailableMaskNumber() const;

  struct MasterBlockClipboardData {
    quint32 originalId = 0;
    MasterBlockElement *originalPtr = nullptr;
    QString name;
    QPointF pos;
    QRectF rect;
    quint16 address = 1;
    QString stepName;
    std::array<QString, 6> lines;
    std::array<quint16, 16> mFields{};
    QColor borderColor;
    qreal borderWidth = 1.5;
    Qt::PenStyle borderStyle = Qt::SolidLine;
    QColor fillColor;
    Qt::BrushStyle fillStyle = Qt::SolidPattern;
    qreal zValue = 0.0;
  };

  struct ConnectorClipboardData {
    quint32 originalId = 0;
    QString name;
    QPointF pos;
    QPointF p1;
    QPointF p2;
    QVector<QPointF> points;
    MasterBlockElement *startBlock = nullptr;
    PortPosition startPort = PortPosition::None;
    MasterBlockElement *endBlock = nullptr;
    PortPosition endPort = PortPosition::None;
    bool isOrthogonal = true;
    bool hasManualOffset = false;
    qreal manualMidOffset = 0.0;
    LineMarkerType beginMarker = LineMarkerType::None;
    LineMarkerType endMarker = LineMarkerType::None;
    QColor borderColor;
    qreal borderWidth = 1.5;
    Qt::PenStyle borderStyle = Qt::SolidLine;
    qreal zValue = 0.0;
  };

  struct MaskClipboardData {
    QString name;
    QPointF pos;
    QRectF rect;
    QString maskText;
    QColor borderColor;
    qreal borderWidth = 1.0;
    Qt::PenStyle borderStyle = Qt::NoPen;
    QColor fillColor;
    Qt::BrushStyle fillStyle = Qt::SolidPattern;
    qreal zValue = 0.0;
  };

  struct TextClipboardData {
    QPointF pos;
    QRectF rect;
    QString text;
    QString fontFamily;
    int fontSize = 12;
    QColor textColor;
    QColor borderColor;
    qreal borderWidth = 1.0;
    Qt::PenStyle borderStyle = Qt::NoPen;
    QColor fillColor;
    Qt::BrushStyle fillStyle = Qt::NoBrush;
    qreal zValue = 0.0;
  };

  struct SceneClipboard {
    QList<MasterBlockClipboardData> blocks;
    QList<ConnectorClipboardData> connectors;
    QList<MaskClipboardData> masks;
    QList<TextClipboardData> texts;
    int pasteCount = 0;
  };

  static SceneClipboard s_clipboard_;

  QUndoStack *undoStack_ = nullptr;
  QHash<ShapeElement *, QPointF> preMovePositions_;
  ShapeElement *preResizeShape_ = nullptr;
  QPointF preResizePos_;
  QRectF preResizeRect_;
  ConnectorElement::RouteState preResizeConnectorState_;

  quint32 nextElementId_ = 1;
  quint32 masterBlockCount_ = 0;
  quint32 connectorCount_ = 0;
  quint32 maskCount_ = 0;
};

} // namespace editor
