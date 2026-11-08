#pragma once

#include "src/elements/connectorElement/connectorelement.h"
#include "src/elements/masterBlockElement/masterblockelement.h"
#include "src/elements/common/portposition.h"

#include <QHash>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QUndoCommand>
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
    struct ConnectorConnInfo {
        MasterBlockElement *startBlock = nullptr;
        PortPosition startPort = PortPosition::None;
        MasterBlockElement *endBlock = nullptr;
        PortPosition endPort = PortPosition::None;
    };

    CanvasScene *scene_;
    QList<ShapeElement*> elements_;
    QHash<ConnectorElement*, ConnectorConnInfo> connectorConns_;
    QHash<ConnectorElement*, ConnectorConnInfo> externalConnectorConns_;
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

// 4. Команда изменения размера K1804 MasterBlock
class ResizeMasterBlockCommand : public QUndoCommand {
public:
    ResizeMasterBlockCommand(CanvasScene *scene,
                             MasterBlockElement *block,
                             const QPointF &oldPos, const QRectF &oldRect,
                             const QPointF &newPos, const QRectF &newRect,
                             QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CanvasScene *scene_;
    MasterBlockElement *blockItem_;
    QPointF oldPos_;
    QRectF oldRect_;
    QPointF newPos_;
    QRectF newRect_;
};

// 5. Команда изменения формы/концов коннектора
class ResizeConnectorCommand : public QUndoCommand {
public:
    ResizeConnectorCommand(CanvasScene *scene,
                           ConnectorElement *connector,
                           const ConnectorElement::RouteState &oldState,
                           const ConnectorElement::RouteState &newState,
                           QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CanvasScene *scene_;
    ConnectorElement *connectorItem_;
    ConnectorElement::RouteState oldState_;
    ConnectorElement::RouteState newState_;
};

// 6. Команда изменения параметров микрокоманды блока K1804 MasterBlock
class ModifyMasterBlockParamsCommand : public QUndoCommand {
public:
    struct BlockState {
        QString stepName;
        quint16 address = 1;
        std::array<quint16, 16> mFields{};
        std::array<QString, 6> lines;
    };

    ModifyMasterBlockParamsCommand(CanvasScene *scene,
                                   MasterBlockElement *block,
                                   const BlockState &oldState,
                                   const BlockState &newState,
                                   QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CanvasScene *scene_;
    MasterBlockElement *blockItem_;
    BlockState oldState_;
    BlockState newState_;
};

} // namespace editor
