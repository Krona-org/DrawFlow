#include "src/canvas/canvasscene.h"
#include "src/core/projectstorage.h"
#include "src/frames/wElementsFrame/data/windowelementsframe.h"

#include <QApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <cstdio>

using namespace editor;

static bool check(bool condition, const char *message) {
  if (!condition) std::fprintf(stderr, "FAIL: %s\n", message);
  return condition;
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QTemporaryDir dir;
  if (!check(dir.isValid(), "temporary directory")) return 1;
  CanvasScene scene;
  WindowElementsFrame tree(&scene);
  auto *widget = tree.findChild<QTreeWidget *>();
  if (!check(widget, "object tree exists")) return 1;
  auto *first = dynamic_cast<MasterBlockElement *>(scene.addElementAt(ElementType::MasterBlock, QPointF(0, 0)));
  auto *second = dynamic_cast<MasterBlockElement *>(scene.addElementAt(ElementType::MasterBlock, QPointF(200, 0)));
  if (!check(first && second, "blocks created")) return 1;
  first->setAddress(5);
  second->setAddress(5);
  const QString path = dir.filePath(QStringLiteral("output.bin"));
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly) || file.write("original") != 8) return 1;
  file.close();
  QString error;
  if (!check(!ProjectStorage::exportK1804Binary(path, &scene, nullptr, &error), "duplicate rejected") ||
      !check(error.contains(QStringLiteral("5")), "duplicate error contains address") ||
      !file.open(QIODevice::ReadOnly) || !check(file.readAll() == QByteArray("original"), "existing output preserved")) return 1;
  file.close();
  second->setAddress(6);
  int count = 0;
  if (!check(ProjectStorage::exportK1804Binary(path, &scene, &count, &error), "unique addresses exported") ||
      !check(count == 2, "export count") || !file.open(QIODevice::ReadOnly)) return 1;
  const QByteArray bytes = file.readAll();
  if (!check(bytes.size() == 70 && bytes[50] == 5 && bytes[60] == 6, "records at addresses")) return 1;
  file.close();
  if (!check(widget->topLevelItemCount() == 2, "tree populated")) return 1;
  scene.clearAll();
  if (!check(widget->topLevelItemCount() == 0, "tree cleared")) return 1;
  scene.addElementAt(ElementType::MasterBlock, QPointF());
  if (!check(widget->topLevelItemCount() == 1, "tree repopulated")) return 1;
  scene.clearAll();
  if (!check(widget->topLevelItemCount() == 0, "tree cleared again")) return 1;
  auto *oldScene = new CanvasScene;
  tree.setScene(oldScene);
  oldScene->addElementAt(ElementType::MasterBlock, QPointF());
  if (!check(widget->topLevelItemCount() == 1, "old scene populated")) return 1;
  delete oldScene;
  if (!check(widget->topLevelItemCount() == 0, "destroyed scene removed from tree")) return 1;
  tree.setScene(&scene);
  scene.addElementAt(ElementType::MasterBlock, QPointF());
  return check(widget->topLevelItemCount() == 1, "new scene after destruction") ? 0 : 1;
}
