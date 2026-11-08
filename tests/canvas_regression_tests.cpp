#include "src/canvas/canvasscene.h"
#include "src/canvas/undocommands.h"
#include <QApplication>
#include <QDebug>

using namespace editor;

#define CHECK(x) do { if (!(x)) { qCritical() << "Failed:" << #x << "line" << __LINE__; return 1; } } while (false)

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    CanvasScene scene;
    scene.setSnapToGrid(false);
    auto *connector = new ConnectorElement(1, QStringLiteral("connector"), QUuid::createUuid());
    connector->setPos(7, 9);
    connector->setPoints({QPointF(10, 20), QPointF(30, 20), QPointF(30, 50)});
    connector->setManualMidOffset(12);
    scene.addItem(connector);
    connector->setSelected(true);
    scene.copySelection();
    scene.pasteClipboard();
    auto *copy = dynamic_cast<ConnectorElement *>(scene.selectedItems().first());
    CHECK(copy && copy != connector);
    CHECK(copy->mapToScene(copy->points().first()) == QPointF(37, 49));
    CHECK(copy->persistentZValue() == 2.0);
    CHECK(copy->hasManualOffset() && copy->manualMidOffset() == 12);
    scene.clearSelection();
    CHECK(copy->zValue() == 2.0);
    scene.undoStack()->undo();
    scene.undoStack()->redo();
    CHECK(copy->mapToScene(copy->points().first()) == QPointF(37, 49));

    auto *block = new MasterBlockElement(2, QStringLiteral("block"), QUuid::createUuid());
    scene.addItem(block);
    const auto before = copy->routeState();
    copy->setStartConnection(block, PortPosition::InMid);
    copy->clearManualOffset();
    copy->setPoints({QPointF(1, 2), QPointF(3, 4)});
    const auto after = copy->routeState();
    scene.undoStack()->push(new ResizeConnectorCommand(&scene, copy, before, after));
    scene.undoStack()->undo();
    CHECK(copy->routeState() == before);
    scene.undoStack()->redo();
    CHECK(copy->routeState() == after);
    return 0;
}
