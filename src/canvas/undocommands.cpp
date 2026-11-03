#include "undocommands.h"
#include "canvasscene.h"

namespace editor {

// --- AddElementCommand ---
AddElementCommand::AddElementCommand(CanvasScene *scene, ShapeElement *element, bool alreadyInScene, QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , element_(element)
    , inScene_(alreadyInScene)
    , firstRedo_(true)
{
    if (element_) {
        setText(QStringLiteral("Добавление: %1").arg(element_->name()));
    } else {
        setText(QStringLiteral("Добавление элемента"));
    }
}

AddElementCommand::~AddElementCommand() {
    if (!inScene_ && element_) {
        delete element_;
    }
}

void AddElementCommand::undo() {
    if (inScene_ && element_ && scene_) {
        scene_->removeItem(element_);
        inScene_ = false;
        emit scene_->elementRemoved(element_);
    }
}

void AddElementCommand::redo() {
    if (!scene_ || !element_) return;
    if (firstRedo_) {
        firstRedo_ = false;
        if (inScene_) {
            emit scene_->elementAdded(element_);
            return;
        }
    }

    if (!inScene_) {
        scene_->addItem(element_);
        inScene_ = true;
        emit scene_->elementAdded(element_);
        element_->setSelected(true);
    }
}

// --- DeleteElementsCommand ---
DeleteElementsCommand::DeleteElementsCommand(CanvasScene *scene, const QList<ShapeElement*> &elements, QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , elements_(elements)
    , inScene_(true)
{
    setText(QStringLiteral("Удаление элементов (%1)").arg(elements.size()));
    for (ShapeElement *elem : elements_) {
        if (auto line = dynamic_cast<LineElement*>(elem)) {
            LineConnInfo info;
            info.startRect = line->startRect();
            info.startPort = line->startPort();
            info.endRect = line->endRect();
            info.endPort = line->endPort();
            lineConns_[line] = info;
        }
    }

    // Сохраняем связи внешних линий, присоединенных к удаляемым прямоугольникам
    if (scene_) {
        for (QGraphicsItem *item : scene_->items()) {
            if (auto line = dynamic_cast<LineElement*>(item)) {
                if (elements_.contains(line)) continue;
                bool hasConn = false;
                for (ShapeElement *elem : elements_) {
                    if (elem == line->startRect() || elem == line->endRect()) {
                        hasConn = true;
                        break;
                    }
                }
                if (hasConn) {
                    LineConnInfo info;
                    info.startRect = line->startRect();
                    info.startPort = line->startPort();
                    info.endRect = line->endRect();
                    info.endPort = line->endPort();
                    externalLineConns_[line] = info;
                }
            }
        }
    }
}

DeleteElementsCommand::~DeleteElementsCommand() {
    if (!inScene_) {
        for (ShapeElement *elem : elements_) {
            delete elem;
        }
    }
}

void DeleteElementsCommand::undo() {
    if (!inScene_ && scene_) {
        for (ShapeElement *elem : elements_) {
            scene_->addItem(elem);
            emit scene_->elementAdded(elem);
            elem->setSelected(true);
        }
        for (auto it = lineConns_.begin(); it != lineConns_.end(); ++it) {
            LineElement *line = it.key();
            const LineConnInfo &info = it.value();
            line->setStartConnection(info.startRect, info.startPort);
            line->setEndConnection(info.endRect, info.endPort);
        }
        for (auto it = externalLineConns_.begin(); it != externalLineConns_.end(); ++it) {
            LineElement *line = it.key();
            const LineConnInfo &info = it.value();
            line->setStartConnection(info.startRect, info.startPort);
            line->setEndConnection(info.endRect, info.endPort);
        }
        for (ShapeElement *elem : elements_) {
            if (auto rect = dynamic_cast<RectangleElement*>(elem)) {
                scene_->updateConnectedLines(rect);
            }
        }
        inScene_ = true;
    }
}

void DeleteElementsCommand::redo() {
    if (inScene_ && scene_) {
        // Отсоединяем внешние линии от удаляемых элементов, чтобы не было битых указателей
        for (auto it = externalLineConns_.begin(); it != externalLineConns_.end(); ++it) {
            LineElement *line = it.key();
            for (ShapeElement *elem : elements_) {
                if (elem == line->startRect()) {
                    line->setStartConnection(nullptr, PortPosition::None);
                }
                if (elem == line->endRect()) {
                    line->setEndConnection(nullptr, PortPosition::None);
                }
            }
        }
        for (ShapeElement *elem : elements_) {
            scene_->removeItem(elem);
            emit scene_->elementRemoved(elem);
        }
        inScene_ = false;
    }
}

// --- MoveElementsCommand ---
MoveElementsCommand::MoveElementsCommand(CanvasScene *scene,
                                         const QHash<ShapeElement*, QPointF> &oldPositions,
                                         const QHash<ShapeElement*, QPointF> &newPositions,
                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , oldPositions_(oldPositions)
    , newPositions_(newPositions)
{
    setText(QStringLiteral("Перемещение элементов"));
}

void MoveElementsCommand::undo() {
    if (!scene_) return;
    for (auto it = oldPositions_.begin(); it != oldPositions_.end(); ++it) {
        ShapeElement *elem = it.key();
        if (elem && elem->scene() == scene_) {
            elem->setPos(it.value());
            if (auto rect = dynamic_cast<RectangleElement*>(elem)) {
                scene_->updateConnectedLines(rect);
            }
        }
    }
}

void MoveElementsCommand::redo() {
    if (!scene_) return;
    for (auto it = newPositions_.begin(); it != newPositions_.end(); ++it) {
        ShapeElement *elem = it.key();
        if (elem && elem->scene() == scene_) {
            elem->setPos(it.value());
            if (auto rect = dynamic_cast<RectangleElement*>(elem)) {
                scene_->updateConnectedLines(rect);
            }
        }
    }
}

// --- ResizeRectangleCommand ---
ResizeRectangleCommand::ResizeRectangleCommand(CanvasScene *scene,
                                               RectangleElement *rect,
                                               const QPointF &oldPos, const QRectF &oldRect,
                                               const QPointF &newPos, const QRectF &newRect,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , rectItem_(rect)
    , oldPos_(oldPos)
    , oldRect_(oldRect)
    , newPos_(newPos)
    , newRect_(newRect)
{
    setText(QStringLiteral("Изменение размера: %1").arg(rect ? rect->name() : QString()));
}

void ResizeRectangleCommand::undo() {
    if (!scene_ || !rectItem_ || rectItem_->scene() != scene_) return;
    rectItem_->setPos(oldPos_);
    rectItem_->setRect(oldRect_);
    scene_->updateConnectedLines(rectItem_);
}

void ResizeRectangleCommand::redo() {
    if (!scene_ || !rectItem_ || rectItem_->scene() != scene_) return;
    rectItem_->setPos(newPos_);
    rectItem_->setRect(newRect_);
    scene_->updateConnectedLines(rectItem_);
}

// --- ResizeLineCommand ---
ResizeLineCommand::ResizeLineCommand(CanvasScene *scene,
                                     LineElement *line,
                                     const QPointF &oldP1, const QPointF &oldP2, const QVector<QPointF> &oldPoints,
                                     const QPointF &newP1, const QPointF &newP2, const QVector<QPointF> &newPoints,
                                     QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , lineItem_(line)
    , oldP1_(oldP1)
    , oldP2_(oldP2)
    , oldPoints_(oldPoints)
    , newP1_(newP1)
    , newP2_(newP2)
    , newPoints_(newPoints)
{
    setText(QStringLiteral("Изменение линии: %1").arg(line ? line->name() : QString()));
}

void ResizeLineCommand::undo() {
    if (!scene_ || !lineItem_ || lineItem_->scene() != scene_) return;
    lineItem_->setP1(oldP1_);
    lineItem_->setP2(oldP2_);
    lineItem_->setPoints(oldPoints_);
}

void ResizeLineCommand::redo() {
    if (!scene_ || !lineItem_ || lineItem_->scene() != scene_) return;
    lineItem_->setP1(newP1_);
    lineItem_->setP2(newP2_);
    lineItem_->setPoints(newPoints_);
}

} // namespace editor
