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
    const QSet<ShapeElement *> removed(elements_.cbegin(), elements_.cend());
    if (scene_) {
        for (QGraphicsItem *item : scene_->items()) {
            if (auto connector = dynamic_cast<ConnectorElement*>(item)) {
                if (removed.contains(connector)) continue;
                if (removed.contains(connector->startBlock()) ||
                    removed.contains(connector->endBlock())) {
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
        scene_->beginSelectionBatch();
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
        QSet<MasterBlockElement *> restoredBlocks;
        for (ShapeElement *elem : elements_) {
            if (auto block = dynamic_cast<MasterBlockElement*>(elem))
                restoredBlocks.insert(block);
        }
        if (!restoredBlocks.isEmpty())
            scene_->updateConnectedLinesForBlocks(restoredBlocks);
        inScene_ = true;
        scene_->endSelectionBatch();
    }
}

void DeleteElementsCommand::redo() {
    if (inScene_ && scene_) {
        scene_->beginSelectionBatch();
        const QSet<ShapeElement *> removed(elements_.cbegin(), elements_.cend());
        // Отсоединяем внешние коннекторы от удаляемых блоков.
        for (auto it = externalConnectorConns_.begin(); it != externalConnectorConns_.end(); ++it) {
            ConnectorElement *connector = it.key();
            if (removed.contains(connector->startBlock()))
                connector->setStartConnection(nullptr, PortPosition::None);
            if (removed.contains(connector->endBlock()))
                connector->setEndConnection(nullptr, PortPosition::None);
        }
        for (ShapeElement *elem : elements_) {
            scene_->removeItem(elem);
            emit scene_->elementRemoved(elem);
        }
        inScene_ = false;
        scene_->endSelectionBatch();
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
                                               const ConnectorElement::RouteState &oldState,
                                               const ConnectorElement::RouteState &newState,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , connectorItem_(connector)
    , oldState_(oldState)
    , newState_(newState)
{
    setText(QStringLiteral("Изменение коннектора: %1").arg(connector ? connector->name() : QString()));
}

void ResizeConnectorCommand::undo() {
    if (!scene_ || !connectorItem_ || connectorItem_->scene() != scene_) return;
    connectorItem_->setRouteState(oldState_);
}

void ResizeConnectorCommand::redo() {
    if (!scene_ || !connectorItem_ || connectorItem_->scene() != scene_) return;
    connectorItem_->setRouteState(newState_);
}

// --- ModifyMasterBlockParamsCommand ---
ModifyMasterBlockParamsCommand::ModifyMasterBlockParamsCommand(CanvasScene *scene,
                                                               MasterBlockElement *block,
                                                               const BlockState &oldState,
                                                               const BlockState &newState,
                                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , scene_(scene)
    , blockItem_(block)
    , oldState_(oldState)
    , newState_(newState)
{
    setText(QStringLiteral("Параметры микрокоманды: %1").arg(block ? block->name() : QString()));
}

void ModifyMasterBlockParamsCommand::undo() {
    if (!scene_ || !blockItem_ || blockItem_->scene() != scene_) return;
    blockItem_->setStepName(oldState_.stepName);
    blockItem_->setAddress(oldState_.address);
    blockItem_->setAllMFields(oldState_.mFields);
    blockItem_->setAllLines(oldState_.lines);
}

void ModifyMasterBlockParamsCommand::redo() {
    if (!scene_ || !blockItem_ || blockItem_->scene() != scene_) return;
    blockItem_->setStepName(newState_.stepName);
    blockItem_->setAddress(newState_.address);
    blockItem_->setAllMFields(newState_.mFields);
    blockItem_->setAllLines(newState_.lines);
}

} // namespace editor
