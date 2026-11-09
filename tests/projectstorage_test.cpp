#include "src/core/projectstorage.h"
#include "src/elements/connectorElement/connectorelement.h"
#include "src/elements/masterBlockElement/masterblockelement.h"
#include "src/elements/textElement/textelement.h"

#include <QApplication>
#include <QFile>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>
#include <cstdlib>
#include <iostream>

using namespace editor;

static void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << std::endl;
    std::exit(1);
  }
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QTemporaryDir dir;
  check(dir.isValid(), "temporary directory");
  const QString path = dir.filePath("project.xml");
  const QUuid uid1 = QUuid::createUuid(), uid2 = QUuid::createUuid();
  QFile file(path);
  check(file.open(QIODevice::WriteOnly | QIODevice::Text), "open fixture");
  QTextStream out(&file);
  out << "<k1804project name=\"fixture\"><canvas pageFormat=\"A4Portrait\"/>"
      << "<blocks><block id=\"1\" uid=\"" << uid1.toString() << "\"/>"
      << "<block id=\"1\" uid=\"" << uid2.toString() << "\"/></blocks>"
      << "<connectors><connector id=\"3\"><connection fromBlockId=\"1\" fromBlockUid=\""
      << uid1.toString() << "\" fromPort=\"OutMid\" toBlockId=\"1\" toBlockUid=\""
      << uid2.toString() << "\" toPort=\"InMid\"/><route>"
      << "<point x=\"10\" y=\"20\"/><point x=\"35\" y=\"20\"/>"
      << "<point x=\"35\" y=\"70\"/><point x=\"100\" y=\"70\"/>"
      << "</route></connector></connectors></k1804project>";
  out.flush();
  file.close();
  CanvasScene scene;
  QString name, error;
  check(ProjectStorage::loadFromFile(path, name, &scene, &error), "load fixture");
  check(scene.pageFormat() == PageFormat::A4Portrait, "page format load");
  ConnectorElement *conn = nullptr;
  for (auto *element : scene.allElements())
    if (auto *c = dynamic_cast<ConnectorElement *>(element)) conn = c;
  check(conn && conn->startBlock() && conn->endBlock(), "connections restored");
  check(conn->startBlock()->uid() == uid1 && conn->endBlock()->uid() == uid2,
        "UID priority");
  const QVector<QPointF> route{{10,20},{35,20},{35,70},{100,70}};
  check(conn->points() == route, "manual route retained");
  conn->setManualMidOffset(17.5);
  auto *added = scene.addElementAt(ElementType::Mask, QPointF(300, 300));
  check(added && added->id() != 1 && added->id() != 3, "new ID unique");
  const QUuid textUid = QUuid::createUuid();
  auto *text = new TextElement(42, QStringLiteral("Имя & текст"), textUid);
  text->setPos(12.5, 24.25);
  text->setRect(QRectF(0, 0, 200.5, 45.75));
  text->setText(QStringLiteral("Строка <one> & \"two\"\nВторая"));
  text->setFontSize(19);
  text->setTextColor(QColor(20, 40, 60, 128));
  text->setFontFamily(QStringLiteral("Arial"));
  scene.addItem(text);
  check(ProjectStorage::saveToFile(path, name, &scene, &error), "save fixture");
  CanvasScene reloaded;
  check(ProjectStorage::loadFromFile(path, name, &reloaded, &error), "reload fixture");
  check(reloaded.pageFormat() == PageFormat::A4Portrait, "page format round trip");
  for (auto *element : reloaded.allElements())
    if (auto *c = dynamic_cast<ConnectorElement *>(element)) {
      check(c->points() == route, "route round trip");
      check(c->hasManualOffset() && c->manualMidOffset() == 17.5,
            "manual offset round trip");
    }
  TextElement *loadedText = nullptr;
  for (auto *element : reloaded.allElements())
    if (auto *t = dynamic_cast<TextElement *>(element)) loadedText = t;
  check(loadedText && loadedText->id() == 42 && loadedText->uid() == textUid &&
        loadedText->name() == text->name() && loadedText->pos() == text->pos() &&
        loadedText->rect() == text->rect() && loadedText->text() == text->text() &&
        loadedText->fontSize() == text->fontSize() &&
        loadedText->textColor() == text->textColor() &&
        loadedText->fontFamily() == text->fontFamily(), "text round trip");
  QFile legacy(dir.filePath("legacy.xml"));
  check(legacy.open(QIODevice::WriteOnly), "open legacy fixture");
  legacy.write("<k1804project><elements><element type='text' id='7' name='old' "
               "x='10' y='20' width='150' height='40' content='legacy' "
               "fontSize='14' textColor='#123456' fontFamily='Arial'/></elements></k1804project>");
  legacy.close();
  CanvasScene legacyScene;
  check(ProjectStorage::loadFromFile(legacy.fileName(), name, &legacyScene, &error), "load legacy text");
  auto *legacyText = legacyScene.allElements().isEmpty() ? nullptr :
      dynamic_cast<TextElement *>(legacyScene.allElements().first());
  check(legacyText && legacyText->id() == 7 && legacyText->text() == QStringLiteral("legacy") &&
        legacyText->pos() == QPointF(10, 20) && legacyText->rect().size() == QSizeF(150, 40) &&
        legacyText->fontSize() == 14 && legacyText->textColor() == QColor("#123456") &&
        legacyText->fontFamily() == QStringLiteral("Arial"), "legacy text properties");
  QFile drawio(dir.filePath("unsupported.drawio"));
  check(drawio.open(QIODevice::WriteOnly), "open drawio fixture");
  drawio.write("<mxfile><diagram/></mxfile>");
  drawio.close();
  const int count = reloaded.allElements().size();
  const QString oldName = name;
  check(!ProjectStorage::loadFromFile(drawio.fileName(), name, &reloaded, &error),
        "reject drawio");
  check(!error.isEmpty() && name == oldName && reloaded.allElements().size() == count,
        "rejected load must not modify project");
  QFile empty(dir.filePath("empty.xml"));
  check(empty.open(QIODevice::WriteOnly), "open empty project fixture");
  empty.write("<k1804project name=\"empty\"><pages/></k1804project>");
  empty.close();
  QList<PageInfo> existingPages{{QStringLiteral("existing"), &reloaded}};
  check(!ProjectStorage::loadFromFile(empty.fileName(), name, existingPages, &error),
        "reject project with no pages");
  check(name == oldName && existingPages.size() == 1 && existingPages[0].scene == &reloaded,
        "empty project rejection must preserve old pages");
  return 0;
}
