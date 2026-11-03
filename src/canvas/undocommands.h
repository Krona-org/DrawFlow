#pragma once

#include "src/elements/rectangleelement.h"
#include "src/elements/lineelement.h"

#include <QUndoCommand>
#include <QList>
#include <QHash>
#include <QPointF>
#include <QRectF>
#include <QVector>

namespace editor {

class CanvasScene;

// 1. Команда добавления элемента
class AddElementCommand : public QUndoCommand {
public:
    AddElementCommand(CanvasScene *scene, ShapeElement *element, bool alreadyInScene = false, QUndoCommand *parent = nullptr);
    ~AddElementCommand() override;

    void undo() override;
    void redo() override;

private:
    CanvasScene *scene_;
    ShapeElement *element_;
    bool inScene_;
    bool firstRedo_ = true;
};

// 2. Команда удаления элементов
class DeleteElementsCommand : public QUndoCommand {
public:
    DeleteElementsCommand(CanvasScene *scene, const QList<ShapeElement*> &elements, QUndoCommand *parent = nullptr);
    ~DeleteElementsCommand() override;

    void undo() override;
    void redo() override;

private:
    struct LineConnInfo {
        RectangleElement *startRect = nullptr;
        PortPosition startPort = PortPosition::None;
        RectangleElement *endRect = nullptr;
        PortPosition endPort = PortPosition::None;
    };

    CanvasScene *scene_;
    QList<ShapeElement*> elements_;
    QHash<LineElement*, LineConnInfo> lineConns_;
    QHash<LineElement*, LineConnInfo> externalLineConns_;
    bool inScene_ = true;
};

// 3. Команда перемещения элементов
class MoveElementsCommand : public QUndoCommand {
public:
    MoveElementsCommand(CanvasScene *scene,
                        const QHash<ShapeElement*, QPointF> &oldPositions,
                        const QHash<ShapeElement*, QPointF> &newPositions,
                        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CanvasScene *scene_;
    QHash<ShapeElement*, QPointF> oldPositions_;
    QHash<ShapeElement*, QPointF> newPositions_;
};

// 4. Команда изменения размера прямоугольника
class ResizeRectangleCommand : public QUndoCommand {
public:
    ResizeRectangleCommand(CanvasScene *scene,
                           RectangleElement *rect,
                           const QPointF &oldPos, const QRectF &oldRect,
                           const QPointF &newPos, const QRectF &newRect,
                           QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CanvasScene *scene_;
    RectangleElement *rectItem_;
    QPointF oldPos_;
    QRectF oldRect_;
    QPointF newPos_;
    QRectF newRect_;
};

// 5. Команда изменения формы/концов линии
class ResizeLineCommand : public QUndoCommand {
public:
    ResizeLineCommand(CanvasScene *scene,
                      LineElement *line,
                      const QPointF &oldP1, const QPointF &oldP2, const QVector<QPointF> &oldPoints,
                      const QPointF &newP1, const QPointF &newP2, const QVector<QPointF> &newPoints,
                      QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CanvasScene *scene_;
    LineElement *lineItem_;
    QPointF oldP1_;
    QPointF oldP2_;
    QVector<QPointF> oldPoints_;
    QPointF newP1_;
    QPointF newP2_;
    QVector<QPointF> newPoints_;
};

} // namespace editor
