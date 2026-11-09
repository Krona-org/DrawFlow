#include "src/canvas/canvasscene.h"
#include "src/canvas/undocommands.h"
#include "src/themes/canvascolors.h"
#include "src/elements/textElement/textelement.h"
#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QTextOption>
#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QPainter>

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

    CanvasScene bulk;
    for (int i = 1; i <= 150; ++i)
        bulk.addItem(new MaskElement(i, QStringLiteral("Mask %1").arg(i), QUuid::createUuid()));
    int selectionNotifications = 0;
    QObject::connect(&bulk, &CanvasScene::sceneSelectionChanged, [&] { ++selectionNotifications; });
    bulk.selectAllElements();
    CHECK(bulk.selectedItems().size() == 150);
    CHECK(selectionNotifications == 1);
    bulk.copySelection();
    bulk.pasteClipboard();
    CHECK(bulk.allElements().size() == 300);
    CHECK(bulk.selectedItems().size() == 150);
    CHECK(selectionNotifications == 2);
    QSet<quint32> ids;
    QSet<QString> names;
    for (auto *element : bulk.allElements()) {
        ids.insert(element->id());
        names.insert(element->name());
    }
    CHECK(ids.size() == 300);
    CHECK(names.size() == 300);
    bulk.deleteSelectedElements();
    CHECK(bulk.allElements().size() == 150);
    bulk.undoStack()->undo();
    CHECK(bulk.allElements().size() == 300);
    CHECK(bulk.selectedItems().size() == 150);
    bulk.undoStack()->redo();
    CHECK(bulk.allElements().size() == 150);

    CanvasScene themeScene;
    themeScene.setGridVisible(false);
    auto *themeMask = new MaskElement(1, QStringLiteral("Mask 1"), QUuid::createUuid());
    const QColor originalFill = themeMask->fillColor();
    themeScene.addItem(themeMask);
    QImage lightImage(20, 20, QImage::Format_ARGB32);
    QImage darkImage(20, 20, QImage::Format_ARGB32);
    app.setProperty("drawflowDarkTheme", false);
    { QPainter painter(&lightImage); themeScene.render(&painter, QRectF(0, 0, 20, 20), QRectF(500, 500, 20, 20)); }
    app.setProperty("drawflowDarkTheme", true);
    { QPainter painter(&darkImage); themeScene.render(&painter, QRectF(0, 0, 20, 20), QRectF(500, 500, 20, 20)); }
    CHECK(darkImage.pixelColor(10, 10).lightness() < lightImage.pixelColor(10, 10).lightness());
    CHECK(darkImage.pixelColor(10, 10) == QColor(76, 76, 76));
    CHECK(canvasFillColor(QColor(242, 244, 247)) == QColor(190, 190, 190));
    CHECK(canvasConnectorColor(QColor(24, 24, 27)) == QColor(220, 220, 220));
    CHECK(canvasFillColor(QColor(123, 45, 67)) == QColor(123, 45, 67));
    CHECK(canvasConnectorColor(QColor(123, 45, 67)) == QColor(123, 45, 67));
    CHECK(themeMask->fillColor() == originalFill);
    app.setProperty("drawflowDarkTheme", false);

    CanvasScene textScene;
    textScene.setSnapToGrid(false);
    auto *text = new TextElement(1, QStringLiteral("text"), QUuid::createUuid());
    textScene.addItem(text);
    auto *child = text->childItems().isEmpty() ? nullptr :
        dynamic_cast<QGraphicsTextItem *>(text->childItems().first());
    CHECK(child && child->parentItem() == text);
    CHECK(child->document()->defaultTextOption().wrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere);
    int changes = 0;
    QObject::connect(text, &TextElement::propertyChanged, [&] { ++changes; });
    text->setRect(QRectF(0, 0, 60, 20));
    text->setText(QString(100, QLatin1Char('x')));
    CHECK(text->rect().height() > 30);
    CHECK(changes == 1);
    child->setPlainText(QStringLiteral("edited"));
    CHECK(text->text() == QStringLiteral("edited") && changes == 2);
    text->startEditing(true);
    CHECK(text->isEditing() && child->textInteractionFlags() == Qt::TextEditorInteraction);
    text->finishEditing();
    CHECK(!text->isEditing() && child->textInteractionFlags() == Qt::NoTextInteraction);
    text->setRect(QRectF(0, 0, 120, 50));
    text->setPos(100, 100);
    text->startHandleDrag(ResizeHandle::TopLeft, QPointF(100, 100));
    text->moveHandle(ResizeHandle::TopLeft, QPointF(80, 90));
    CHECK(text->pos() == QPointF(80, 90));
    CHECK(text->rect() == QRectF(0, 0, 140, 60));
    return 0;
}
