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
        if (auto connector = dynamic_cast<ConnectorElement*>(elem)) {
            ConnectorConnInfo info;
            info.startBlock = connector->startBlock();
            info.startPort = connector->startPort();
            info.endBlock = connector->endBlock();
            info.endPort = connector->endPort();
            connectorConns_[connector] = info;
        }
    }

    // Сохраняем связи внешних коннекторов, присоединенных к удаляемым блокам
    if (scene_) {
        for (QGraphicsItem *item : scene_->items()) {
            if (auto connector = dynamic_cast<ConnectorElement*>(item)) {
                if (elements_.contains(connector)) continue;
                bool hasConn = false;
                for (ShapeElement *elem : elements_) {
                    if (elem == connector->startBlock() || elem == connector->endBlock()) {
                        hasConn = true;
                        break;
                    }
                }
                if (hasConn) {
                    ConnectorConnInfo info;
                    info.startBlock = connector->startBlock();
                    info.startPort = connector->startPort();
                    info.endBlock = connector->endBlock();
                    info.endPort = connector->endPort();
                    externalConnectorConns_[connector] = info;
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
        for (auto it = connectorConns_.begin(); it != connectorConns_.end(); ++it) {
            ConnectorElement *connector = it.key();
            const ConnectorConnInfo &info = it.value();
            connector->setStartConnection(info.startBlock, info.startPort);
            connector->setEndConnection(info.endBlock, info.endPort);
        }
        for (auto it = externalConnectorConns_.begin(); it != externalConnectorConns_.end(); ++it) {
            ConnectorElement *connector = it.key();
            const ConnectorConnInfo &info = it.value();
            connector->setStartConnection(info.startBlock, info.startPort);
            connector->setEndConnection(info.endBlock, info.endPort);
        }
        for (ShapeElement *elem : elements_) {
            if (auto block = dynamic_cast<MasterBlockElement*>(elem)) {
                scene_->updateConnectedLines(block);
            }
        }
        inScene_ = true;
    }
}

void DeleteElementsCommand::redo() {
    if (inScene_ && scene_) {
        // Отсоединяем внешние коннекторы от удаляемых блоков, чтобы не было битых указателей
        for (auto it = externalConnectorConns_.begin(); it != externalConnectorConns_.end(); ++it) {
            ConnectorElement *connector = it.key();
            for (ShapeElement *elem : elements_) {
                if (elem == connector->startBlock()) {
                    connector->setStartConnection(nullptr, PortPosition::None);
                }
                if (elem == connector->endBlock()) {
                    connector->setEndConnection(nullptr, PortPosition::None);
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
            if (auto block = dynamic_cast<MasterBlockElement*>(elem)) {
                scene_->updateConnectedLines(block);
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
            if (auto block = dynamic_cast<MasterBlockElement*>(elem)) {
                scene_->updateConnectedLines(block);
            }
        }
    }
}

// --- ResizeMasterBlockCommand ---
ResizeMasterBlockCommand::ResizeMasterBlockCommand(CanvasScene *scene,
                                                   MasterBlockElement *block,
                                                   const QPointF &oldPos, const QRectF &oldRect,
                                                   const QPointF &newPos, const QRectF &newRect,
                                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , blockItem_(block)
    , oldPos_(oldPos)
    , oldRect_(oldRect)
    , newPos_(newPos)
    , newRect_(newRect)
{
    setText(QStringLiteral("Изменение размера: %1").arg(block ? block->name() : QString()));
}

void ResizeMasterBlockCommand::undo() {
    if (!scene_ || !blockItem_ || blockItem_->scene() != scene_) return;
    blockItem_->setPos(oldPos_);
    blockItem_->setRect(oldRect_);
    scene_->updateConnectedLines(blockItem_);
}

void ResizeMasterBlockCommand::redo() {
    if (!scene_ || !blockItem_ || blockItem_->scene() != scene_) return;
    blockItem_->setPos(newPos_);
    blockItem_->setRect(newRect_);
    scene_->updateConnectedLines(blockItem_);
}

// --- ResizeConnectorCommand ---
ResizeConnectorCommand::ResizeConnectorCommand(CanvasScene *scene,
                                               ConnectorElement *connector,
                                               const QPointF &oldP1, const QPointF &oldP2, const QVector<QPointF> &oldPoints,
                                               const QPointF &newP1, const QPointF &newP2, const QVector<QPointF> &newPoints,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , connectorItem_(connector)
    , oldP1_(oldP1)
    , oldP2_(oldP2)
    , oldPoints_(oldPoints)
    , newP1_(newP1)
    , newP2_(newP2)
    , newPoints_(newPoints)
{
    setText(QStringLiteral("Изменение коннектора: %1").arg(connector ? connector->name() : QString()));
}

void ResizeConnectorCommand::undo() {
    if (!scene_ || !connectorItem_ || connectorItem_->scene() != scene_) return;
    connectorItem_->setP1(oldP1_);
    connectorItem_->setP2(oldP2_);
    connectorItem_->setPoints(oldPoints_);
}

void ResizeConnectorCommand::redo() {
    if (!scene_ || !connectorItem_ || connectorItem_->scene() != scene_) return;
    connectorItem_->setP1(newP1_);
    connectorItem_->setP2(newP2_);
    connectorItem_->setPoints(newPoints_);
}

} // namespace editor
