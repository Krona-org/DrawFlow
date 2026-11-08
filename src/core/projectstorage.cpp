#include "projectstorage.h"

#include "src/elements/connectorElement/connectorelement.h"
#include "src/elements/maskElement/maskelement.h"
#include "src/elements/masterBlockElement/k1804microcode.h"
#include "src/elements/masterBlockElement/masterblockelement.h"

#include <QDataStream>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QHash>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>
#include <algorithm>

namespace editor {

static QString penStyleToString(Qt::PenStyle style) {
  switch (style) {
  case Qt::NoPen:
    return QStringLiteral("NoPen");
  case Qt::DashLine:
    return QStringLiteral("DashLine");
  case Qt::DotLine:
    return QStringLiteral("DotLine");
  case Qt::DashDotLine:
    return QStringLiteral("DashDotLine");
  default:
    return QStringLiteral("SolidLine");
  }
}

static Qt::PenStyle penStyleFromString(const QString &str) {
  if (str == QStringLiteral("NoPen"))
    return Qt::NoPen;
  if (str == QStringLiteral("DashLine"))
    return Qt::DashLine;
  if (str == QStringLiteral("DotLine"))
    return Qt::DotLine;
  if (str == QStringLiteral("DashDotLine"))
    return Qt::DashDotLine;
  return Qt::SolidLine;
}

static QString portToString(PortPosition port) {
  switch (port) {
  case PortPosition::InTop:
    return QStringLiteral("InTop");
  case PortPosition::InMid:
    return QStringLiteral("InMid");
  case PortPosition::InBot:
    return QStringLiteral("InBot");
  case PortPosition::OutTop:
    return QStringLiteral("OutTop");
  case PortPosition::OutMid:
    return QStringLiteral("OutMid");
  case PortPosition::OutBot:
    return QStringLiteral("OutBot");
  default:
    return QStringLiteral("None");
  }
}

static PortPosition portFromString(const QString &str) {
  if (str == QStringLiteral("InTop"))
    return PortPosition::InTop;
  if (str == QStringLiteral("InMid"))
    return PortPosition::InMid;
  if (str == QStringLiteral("InBot"))
    return PortPosition::InBot;
  if (str == QStringLiteral("OutTop"))
    return PortPosition::OutTop;
  if (str == QStringLiteral("OutMid"))
    return PortPosition::OutMid;
  if (str == QStringLiteral("OutBot"))
    return PortPosition::OutBot;
  // Поддержка старых портов
  if (str == QStringLiteral("Left"))
    return PortPosition::InMid;
  if (str == QStringLiteral("Right"))
    return PortPosition::OutMid;
  if (str == QStringLiteral("Top"))
    return PortPosition::InTop;
  if (str == QStringLiteral("Bottom"))
    return PortPosition::InBot;
  return PortPosition::None;
}

bool ProjectStorage::saveToFile(const QString &filePath,
                                const QString &projectName, CanvasScene *scene,
                                QString *errorMessage) {
  return saveToFile(filePath, projectName,
                    QList<PageInfo>{{QStringLiteral("Страница 1"), scene}}, errorMessage);
}

bool ProjectStorage::saveToFile(const QString &filePath,
                                const QString &projectName, const QList<PageInfo> &pages,
                                QString *errorMessage) {
  for (const PageInfo &page : pages) {
    if (!page.scene) {
      if (errorMessage)
        *errorMessage = QStringLiteral("Холст не инициализирован.");
      return false;
    }
  }

  QDomDocument doc;
  QDomProcessingInstruction header = doc.createProcessingInstruction(
      QStringLiteral("xml"),
      QStringLiteral("version=\"1.0\" encoding=\"UTF-8\""));
  doc.appendChild(header);

  QDomElement root = doc.createElement(QStringLiteral("k1804project"));
  root.setAttribute(QStringLiteral("name"), projectName);
  root.setAttribute(QStringLiteral("version"), QStringLiteral("3.0"));
  doc.appendChild(root);

  // Многостраничный формат: содержимое каждой страницы находится в <page>.
  QDomElement pagesNode = doc.createElement(QStringLiteral("pages"));
  root.appendChild(pagesNode);
  for (const PageInfo &page : pages) {
    CanvasScene *scene = page.scene;
    QDomElement pageNode = doc.createElement(QStringLiteral("page"));
    pageNode.setAttribute(QStringLiteral("name"), page.name);
    pagesNode.appendChild(pageNode);

  // 1. Секция настроек холста (<canvas>)
  QDomElement canvasNode = doc.createElement(QStringLiteral("canvas"));
  canvasNode.setAttribute(QStringLiteral("gridSize"),
                          QString::number(scene->gridSize()));
  canvasNode.setAttribute(QStringLiteral("snapToGrid"),
                          scene->isSnapToGrid() ? QStringLiteral("true")
                                                : QStringLiteral("false"));
  canvasNode.setAttribute(QStringLiteral("showGrid"),
                          scene->isGridVisible() ? QStringLiteral("true")
                                                 : QStringLiteral("false"));
  canvasNode.setAttribute(QStringLiteral("pageFormat"),
                          scene->pageFormat() == PageFormat::A4Landscape
                              ? QStringLiteral("A4Landscape")
                          : scene->pageFormat() == PageFormat::A4Portrait
                              ? QStringLiteral("A4Portrait")
                              : QStringLiteral("Infinite"));
  pageNode.appendChild(canvasNode);

  // Собираем элементы по категориям и сортируем по ID для удобного чтения и
  // ручного редактирования
  QList<MasterBlockElement *> blocks;
  QList<ConnectorElement *> connectors;
  QList<MaskElement *> masks;

  for (ShapeElement *element : scene->allElements()) {
    if (!element)
      continue;
    if (auto *b = dynamic_cast<MasterBlockElement *>(element)) {
      blocks.append(b);
    } else if (auto *c = dynamic_cast<ConnectorElement *>(element)) {
      connectors.append(c);
    } else if (auto *m = dynamic_cast<MaskElement *>(element)) {
      masks.append(m);
    }
  }

  auto sortById = [](const auto *a, const auto *b) {
    return a->id() < b->id();
  };
  std::sort(blocks.begin(), blocks.end(), sortById);
  std::sort(connectors.begin(), connectors.end(), sortById);
  std::sort(masks.begin(), masks.end(), sortById);

  // 2. Секция блоков микрокоманд (<blocks>)
  QDomElement blocksNode = doc.createElement(QStringLiteral("blocks"));
  blocksNode.setAttribute(QStringLiteral("count"),
                          QString::number(blocks.size()));
  pageNode.appendChild(blocksNode);

  for (MasterBlockElement *block : blocks) {
    QDomElement bNode = doc.createElement(QStringLiteral("block"));
    bNode.setAttribute(QStringLiteral("id"), QString::number(block->id()));
    bNode.setAttribute(QStringLiteral("name"), block->name());
    bNode.setAttribute(QStringLiteral("step"), block->stepName());
    bNode.setAttribute(QStringLiteral("addressOct"),
                       K1804Microcode::toOctal(block->address(), 3));
    bNode.setAttribute(QStringLiteral("addressDec"),
                       QString::number(block->address()));
    bNode.setAttribute(QStringLiteral("uid"), block->uid().toString());

    QDomElement geomNode = doc.createElement(QStringLiteral("geometry"));
    geomNode.setAttribute(QStringLiteral("x"),
                          QString::number(block->pos().x()));
    geomNode.setAttribute(QStringLiteral("y"),
                          QString::number(block->pos().y()));
    geomNode.setAttribute(QStringLiteral("width"),
                          QString::number(block->rect().width()));
    geomNode.setAttribute(QStringLiteral("height"),
                          QString::number(block->rect().height()));
    geomNode.setAttribute(QStringLiteral("z"),
                          QString::number(block->zValue()));
    bNode.appendChild(geomNode);

    QDomElement styleNode = doc.createElement(QStringLiteral("style"));
    styleNode.setAttribute(QStringLiteral("fill"), block->fillColor().name());
    styleNode.setAttribute(QStringLiteral("stroke"),
                           block->borderColor().name());
    styleNode.setAttribute(QStringLiteral("strokeWidth"),
                           QString::number(block->borderWidth()));
    styleNode.setAttribute(QStringLiteral("strokeStyle"),
                           penStyleToString(block->borderStyle()));
    bNode.appendChild(styleNode);

    QDomElement mcNode = doc.createElement(QStringLiteral("microcode"));
    for (int i = 1; i <= 15; ++i) {
      mcNode.setAttribute(QStringLiteral("M%1").arg(i),
                          QString::number(block->mField(i)));
    }
    bNode.appendChild(mcNode);

    QDomElement linesNode = doc.createElement(QStringLiteral("displayLines"));
    for (int i = 0; i < 6; ++i) {
      QDomElement lineNode = doc.createElement(QStringLiteral("line"));
      lineNode.setAttribute(QStringLiteral("index"), QString::number(i));
      lineNode.appendChild(doc.createTextNode(block->lineText(i)));
      linesNode.appendChild(lineNode);
    }
    bNode.appendChild(linesNode);

    blocksNode.appendChild(bNode);
  }

  // 3. Секция соединительных линий (<connectors>)
  QDomElement connectorsNode = doc.createElement(QStringLiteral("connectors"));
  connectorsNode.setAttribute(QStringLiteral("count"),
                              QString::number(connectors.size()));
  pageNode.appendChild(connectorsNode);

  for (ConnectorElement *conn : connectors) {
    QDomElement cNode = doc.createElement(QStringLiteral("connector"));
    cNode.setAttribute(QStringLiteral("id"), QString::number(conn->id()));
    cNode.setAttribute(QStringLiteral("name"), conn->name());
    cNode.setAttribute(QStringLiteral("uid"), conn->uid().toString());

    QDomElement geomNode = doc.createElement(QStringLiteral("geometry"));
    geomNode.setAttribute(QStringLiteral("x"),
                          QString::number(conn->pos().x()));
    geomNode.setAttribute(QStringLiteral("y"),
                          QString::number(conn->pos().y()));
    geomNode.setAttribute(QStringLiteral("z"), QString::number(conn->zValue()));
    cNode.appendChild(geomNode);

    QDomElement styleNode = doc.createElement(QStringLiteral("style"));
    styleNode.setAttribute(QStringLiteral("stroke"),
                           conn->borderColor().name());
    styleNode.setAttribute(QStringLiteral("strokeWidth"),
                           QString::number(conn->borderWidth()));
    styleNode.setAttribute(QStringLiteral("strokeStyle"),
                           penStyleToString(conn->borderStyle()));
    styleNode.setAttribute(QStringLiteral("beginMarker"),
                           conn->beginMarker() == LineMarkerType::Arrow
                               ? QStringLiteral("arrow")
                               : QStringLiteral("none"));
    styleNode.setAttribute(QStringLiteral("endMarker"),
                           conn->endMarker() == LineMarkerType::Arrow
                               ? QStringLiteral("arrow")
                               : QStringLiteral("none"));
    styleNode.setAttribute(QStringLiteral("isOrthogonal"),
                           conn->isOrthogonal() ? QStringLiteral("true")
                                                : QStringLiteral("false"));
    cNode.appendChild(styleNode);

    QDomElement connInfoNode = doc.createElement(QStringLiteral("connection"));
    if (conn->startBlock()) {
      connInfoNode.setAttribute(QStringLiteral("fromBlockId"),
                                QString::number(conn->startBlock()->id()));
      connInfoNode.setAttribute(QStringLiteral("fromBlockUid"),
                                conn->startBlock()->uid().toString());
      connInfoNode.setAttribute(QStringLiteral("fromPort"),
                                portToString(conn->startPort()));
    }
    if (conn->endBlock()) {
      connInfoNode.setAttribute(QStringLiteral("toBlockId"),
                                QString::number(conn->endBlock()->id()));
      connInfoNode.setAttribute(QStringLiteral("toBlockUid"),
                                conn->endBlock()->uid().toString());
      connInfoNode.setAttribute(QStringLiteral("toPort"),
                                portToString(conn->endPort()));
    }
    cNode.appendChild(connInfoNode);

    QDomElement routeNode = doc.createElement(QStringLiteral("route"));
    if (conn->hasManualOffset())
      routeNode.setAttribute(QStringLiteral("manualMidOffset"),
                             QString::number(conn->manualMidOffset(), 'g', 17));
    for (const QPointF &pt : conn->points()) {
      QDomElement ptNode = doc.createElement(QStringLiteral("point"));
      ptNode.setAttribute(QStringLiteral("x"), QString::number(pt.x()));
      ptNode.setAttribute(QStringLiteral("y"), QString::number(pt.y()));
      routeNode.appendChild(ptNode);
    }
    cNode.appendChild(routeNode);

    connectorsNode.appendChild(cNode);
  }

  // 4. Секция битовых масок (<masks>)
  QDomElement masksNode = doc.createElement(QStringLiteral("masks"));
  masksNode.setAttribute(QStringLiteral("count"),
                         QString::number(masks.size()));
  pageNode.appendChild(masksNode);

  for (MaskElement *mask : masks) {
    QDomElement mNode = doc.createElement(QStringLiteral("mask"));
    mNode.setAttribute(QStringLiteral("id"), QString::number(mask->id()));
    mNode.setAttribute(QStringLiteral("name"), mask->name());
    mNode.setAttribute(QStringLiteral("text"), mask->maskText());
    mNode.setAttribute(QStringLiteral("uid"), mask->uid().toString());

    QDomElement geomNode = doc.createElement(QStringLiteral("geometry"));
    geomNode.setAttribute(QStringLiteral("x"),
                          QString::number(mask->pos().x()));
    geomNode.setAttribute(QStringLiteral("y"),
                          QString::number(mask->pos().y()));
    geomNode.setAttribute(QStringLiteral("width"),
                          QString::number(mask->rect().width()));
    geomNode.setAttribute(QStringLiteral("height"),
                          QString::number(mask->rect().height()));
    geomNode.setAttribute(QStringLiteral("z"), QString::number(mask->zValue()));
    mNode.appendChild(geomNode);

    QDomElement styleNode = doc.createElement(QStringLiteral("style"));
    styleNode.setAttribute(QStringLiteral("fill"), mask->fillColor().name());
    styleNode.setAttribute(QStringLiteral("stroke"),
                           mask->borderColor().name());
    styleNode.setAttribute(QStringLiteral("strokeWidth"),
                           QString::number(mask->borderWidth()));
    styleNode.setAttribute(QStringLiteral("strokeStyle"),
                           penStyleToString(mask->borderStyle()));
    mNode.appendChild(styleNode);

    masksNode.appendChild(mNode);
  }
  }

  QSaveFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qWarning() << "Не удалось открыть файл проекта для записи:" << file.errorString();
    if (errorMessage)
      *errorMessage = file.errorString();
    return false;
  }

  QTextStream out(&file);
  doc.save(out, 4);
  out.flush();
  if (out.status() != QTextStream::Ok || file.error() != QFileDevice::NoError) {
    const QString message = file.error() != QFileDevice::NoError
                                ? file.errorString()
                                : QStringLiteral("Ошибка записи файла проекта.");
    qWarning() << "Не удалось записать файл проекта:" << message;
    if (errorMessage)
      *errorMessage = message;
    return false;
  }
  if (!file.commit()) {
    qWarning() << "Не удалось сохранить файл проекта:" << file.errorString();
    if (errorMessage)
      *errorMessage = file.errorString();
    return false;
  }

  return true;
}

static bool readProject(const QString &filePath, QDomDocument &doc,
                        QString *errorMessage) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (errorMessage)
      *errorMessage = file.errorString();
    return false;
  }

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  auto result = doc.setContent(&file);
  if (!result) {
    if (errorMessage)
      *errorMessage =
          QStringLiteral("Ошибка разбора XML (строка %1, кол. %2): %3")
              .arg(result.errorLine)
              .arg(result.errorColumn)
              .arg(result.errorMessage);
    file.close();
    return false;
  }
#else
  QString errorMsg;
  int errorLine = 0;
  int errorColumn = 0;
  if (!doc.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
    if (errorMessage)
      *errorMessage =
          QStringLiteral("Ошибка разбора XML (строка %1, кол. %2): %3")
              .arg(errorLine)
              .arg(errorColumn)
              .arg(errorMsg);
    file.close();
    return false;
  }
#endif
  file.close();

  QDomElement root = doc.documentElement();
  if (root.tagName() == QStringLiteral("mxfile")) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Формат draw.io не поддерживается.");
    return false;
  }
  if (root.tagName() != QStringLiteral("k1804project") &&
      root.tagName() != QStringLiteral("dcsdiagram")) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Неверный формат файла проекта.");
    return false;
  }

  return true;
}

static bool loadPage(const QDomElement &root, CanvasScene *scene,
                     QString *errorMessage) {
  CanvasScene tempScene;
  // Отсутствующие настройки сохраняют значения текущего холста.
  tempScene.setGridSize(scene->gridSize());
  tempScene.setSnapToGrid(scene->isSnapToGrid());
  tempScene.setGridVisible(scene->isGridVisible());

  // Настройки холста (поддержка как вложенного <canvas>, так и атрибутов root)
  QDomElement canvasNode = root.firstChildElement(QStringLiteral("canvas"));
  const QDomElement &gridAttrSource = canvasNode.isNull() ? root : canvasNode;
  const QString pageFormat = gridAttrSource.attribute(QStringLiteral("pageFormat"));
  tempScene.setPageFormat(pageFormat == QStringLiteral("A4Landscape")
                           ? PageFormat::A4Landscape
                       : pageFormat == QStringLiteral("A4Portrait")
                           ? PageFormat::A4Portrait
                           : PageFormat::Infinite);
  if (gridAttrSource.hasAttribute(QStringLiteral("gridSize"))) {
    tempScene.setGridSize(
        gridAttrSource.attribute(QStringLiteral("gridSize")).toDouble());
  }
  if (gridAttrSource.hasAttribute(QStringLiteral("snapToGrid"))) {
    tempScene.setSnapToGrid(
        gridAttrSource.attribute(QStringLiteral("snapToGrid")) ==
        QStringLiteral("true"));
  }
  if (gridAttrSource.hasAttribute(QStringLiteral("showGrid"))) {
    tempScene.setGridVisible(
        gridAttrSource.attribute(QStringLiteral("showGrid")) ==
        QStringLiteral("true"));
  }

  QHash<QUuid, MasterBlockElement *> uidToBlock;
  QHash<quint32, MasterBlockElement *> idToBlock;

  struct PendingConn {
    ConnectorElement *conn = nullptr;
    QUuid startUid;
    quint32 startId = 0;
    PortPosition startPort = PortPosition::None;
    QUuid endUid;
    quint32 endId = 0;
    PortPosition endPort = PortPosition::None;
    bool hasExplicitRoute = false;
  };
  QList<PendingConn> pendingConnections;

  // =========================================================================
  // А. Чтение нового структурированного формата v3.0 (<blocks>, <connectors>,
  // <masks>)
  // =========================================================================
  QDomElement blocksNode = root.firstChildElement(QStringLiteral("blocks"));
  if (!blocksNode.isNull()) {
    for (QDomElement bNode =
             blocksNode.firstChildElement(QStringLiteral("block"));
         !bNode.isNull();
         bNode = bNode.nextSiblingElement(QStringLiteral("block"))) {
      quint32 id = bNode.attribute(QStringLiteral("id"), QStringLiteral("1"))
                       .toUInt();
      QString name =
          bNode.attribute(QStringLiteral("name"),
                          QStringLiteral("Блок %1").arg(id));
      QUuid uid = QUuid::fromString(bNode.attribute(QStringLiteral("uid")));
      if (uid.isNull())
        uid = QUuid::createUuid();

      auto *block = new MasterBlockElement(id, name, uid);

      if (bNode.hasAttribute(QStringLiteral("step"))) {
        block->setStepName(bNode.attribute(QStringLiteral("step")));
      } else if (bNode.hasAttribute(QStringLiteral("stepName"))) {
        block->setStepName(bNode.attribute(QStringLiteral("stepName")));
      }

      if (bNode.hasAttribute(QStringLiteral("addressDec"))) {
        block->setAddress(static_cast<quint16>(
            bNode.attribute(QStringLiteral("addressDec")).toUInt()));
      } else if (bNode.hasAttribute(QStringLiteral("addressOct"))) {
        block->setAddress(K1804Microcode::fromOctal(
            bNode.attribute(QStringLiteral("addressOct"))));
      } else if (bNode.hasAttribute(QStringLiteral("address"))) {
        block->setAddress(static_cast<quint16>(
            bNode.attribute(QStringLiteral("address")).toUInt()));
      }

      QDomElement geomNode =
          bNode.firstChildElement(QStringLiteral("geometry"));
      if (!geomNode.isNull()) {
        qreal x = geomNode.attribute(QStringLiteral("x"), QStringLiteral("0"))
                      .toDouble();
        qreal y = geomNode.attribute(QStringLiteral("y"), QStringLiteral("0"))
                      .toDouble();
        qreal w =
            geomNode.attribute(QStringLiteral("width"), QStringLiteral("210"))
                .toDouble();
        qreal h =
            geomNode.attribute(QStringLiteral("height"), QStringLiteral("145"))
                .toDouble();
        qreal z = geomNode.attribute(QStringLiteral("z"), QStringLiteral("0"))
                      .toDouble();
        block->setPos(x, y);
        block->setRect(QRectF(0, 0, w, h));
        block->setZValue(z);
      }

      QDomElement styleNode = bNode.firstChildElement(QStringLiteral("style"));
      if (!styleNode.isNull()) {
        block->setFillColor(QColor(styleNode.attribute(
            QStringLiteral("fill"), QStringLiteral("#ffffff"))));
        block->setBorderColor(QColor(styleNode.attribute(
            QStringLiteral("stroke"), QStringLiteral("#18181b"))));
        block->setBorderWidth(
            styleNode
                .attribute(QStringLiteral("strokeWidth"), QStringLiteral("1.5"))
                .toDouble());
        block->setBorderStyle(penStyleFromString(
            styleNode.attribute(QStringLiteral("strokeStyle"))));
      }

      bool anyNonZeroM = false;
      QDomElement mcNode =
          bNode.firstChildElement(QStringLiteral("microcode"));
      if (!mcNode.isNull()) {
        for (int mIdx = 1; mIdx <= 15; ++mIdx) {
          QString keyUpper = QStringLiteral("M%1").arg(mIdx);
          QString keyLower = QStringLiteral("m%1").arg(mIdx);
          if (mcNode.hasAttribute(keyUpper)) {
            quint16 val =
                static_cast<quint16>(mcNode.attribute(keyUpper).toUInt());
            block->setMField(mIdx, val);
            if (val != 0)
              anyNonZeroM = true;
          } else if (mcNode.hasAttribute(keyLower)) {
            quint16 val =
                static_cast<quint16>(mcNode.attribute(keyLower).toUInt());
            block->setMField(mIdx, val);
            if (val != 0)
              anyNonZeroM = true;
          }
        }
      }

      QDomElement linesNode =
          bNode.firstChildElement(QStringLiteral("displayLines"));
      if (!linesNode.isNull()) {
        for (QDomElement lNode =
                 linesNode.firstChildElement(QStringLiteral("line"));
             !lNode.isNull();
             lNode = lNode.nextSiblingElement(QStringLiteral("line"))) {
          int idx = lNode.attribute(QStringLiteral("index"), QStringLiteral("0"))
                        .toInt();
          if (idx >= 0 && idx < 6) {
            block->setLineText(idx, lNode.text());
          }
        }
      } else if (anyNonZeroM) {
        // Если пользователь отредактировал <microcode> вручную и опустил
        // <displayLines>, автоматически сгенерируем строки из M1..M15
        std::array<quint16, 16> mf{};
        for (int i = 1; i <= 15; ++i)
          mf[i] = block->mField(i);
        block->setAllLines(K1804Microcode::generateBlockLines(mf));
      }

      tempScene.addItem(block);
      uidToBlock[uid] = block;
      idToBlock[id] = block;
    }
  }

  QDomElement connectorsNode =
      root.firstChildElement(QStringLiteral("connectors"));
  if (!connectorsNode.isNull()) {
    for (QDomElement cNode =
             connectorsNode.firstChildElement(QStringLiteral("connector"));
         !cNode.isNull();
         cNode = cNode.nextSiblingElement(QStringLiteral("connector"))) {
      quint32 id = cNode.attribute(QStringLiteral("id"), QStringLiteral("1"))
                       .toUInt();
      QString name =
          cNode.attribute(QStringLiteral("name"),
                          QStringLiteral("Линия %1").arg(id));
      QUuid uid = QUuid::fromString(cNode.attribute(QStringLiteral("uid")));
      if (uid.isNull())
        uid = QUuid::createUuid();

      auto *conn = new ConnectorElement(id, name, uid);

      QDomElement geomNode =
          cNode.firstChildElement(QStringLiteral("geometry"));
      if (!geomNode.isNull()) {
        qreal x = geomNode.attribute(QStringLiteral("x"), QStringLiteral("0"))
                      .toDouble();
        qreal y = geomNode.attribute(QStringLiteral("y"), QStringLiteral("0"))
                      .toDouble();
        qreal z = geomNode.attribute(QStringLiteral("z"), QStringLiteral("-1"))
                      .toDouble();
        conn->setPos(x, y);
        conn->setZValue(z);
      }

      QDomElement styleNode = cNode.firstChildElement(QStringLiteral("style"));
      if (!styleNode.isNull()) {
        conn->setBorderColor(QColor(styleNode.attribute(
            QStringLiteral("stroke"), QStringLiteral("#18181b"))));
        conn->setBorderWidth(
            styleNode
                .attribute(QStringLiteral("strokeWidth"), QStringLiteral("1.5"))
                .toDouble());
        conn->setBorderStyle(penStyleFromString(
            styleNode.attribute(QStringLiteral("strokeStyle"))));
        conn->setBeginMarker(
            styleNode.attribute(QStringLiteral("beginMarker")) ==
                    QStringLiteral("arrow")
                ? LineMarkerType::Arrow
                : LineMarkerType::None);
        conn->setEndMarker(styleNode.attribute(QStringLiteral("endMarker")) ==
                                   QStringLiteral("arrow")
                               ? LineMarkerType::Arrow
                               : LineMarkerType::None);
        if (styleNode.hasAttribute(QStringLiteral("isOrthogonal"))) {
          conn->setOrthogonal(
              styleNode.attribute(QStringLiteral("isOrthogonal")) ==
              QStringLiteral("true"));
        }
      }

      bool hasExplicitRoute = false;
      QDomElement routeNode = cNode.firstChildElement(QStringLiteral("route"));
      if (!routeNode.isNull()) {
        if (routeNode.hasAttribute(QStringLiteral("manualMidOffset")))
          conn->setManualMidOffset(routeNode.attribute(QStringLiteral("manualMidOffset")).toDouble());
        QVector<QPointF> pts;
        for (QDomElement ptNode =
                 routeNode.firstChildElement(QStringLiteral("point"));
             !ptNode.isNull();
             ptNode = ptNode.nextSiblingElement(QStringLiteral("point"))) {
          qreal px = ptNode.attribute(QStringLiteral("x")).toDouble();
          qreal py = ptNode.attribute(QStringLiteral("y")).toDouble();
          pts.append(QPointF(px, py));
        }
        if (pts.size() >= 2) {
          conn->setPoints(pts);
          hasExplicitRoute = true;
        }
      }

      QDomElement connInfoNode =
          cNode.firstChildElement(QStringLiteral("connection"));
      if (!connInfoNode.isNull()) {
        PendingConn pc;
        pc.conn = conn;
        pc.startUid = QUuid::fromString(
            connInfoNode.attribute(QStringLiteral("fromBlockUid")));
        pc.startId =
            connInfoNode.attribute(QStringLiteral("fromBlockId")).toUInt();
        pc.startPort =
            portFromString(connInfoNode.attribute(QStringLiteral("fromPort")));
        pc.endUid = QUuid::fromString(
            connInfoNode.attribute(QStringLiteral("toBlockUid")));
        pc.endId = connInfoNode.attribute(QStringLiteral("toBlockId")).toUInt();
        pc.endPort =
            portFromString(connInfoNode.attribute(QStringLiteral("toPort")));
        pc.hasExplicitRoute = hasExplicitRoute;

        if (!pc.startUid.isNull() || pc.startId > 0 || !pc.endUid.isNull() ||
            pc.endId > 0) {
          pendingConnections.append(pc);
        }
      }

      tempScene.addItem(conn);
    }
  }

  QDomElement masksNode = root.firstChildElement(QStringLiteral("masks"));
  if (!masksNode.isNull()) {
    for (QDomElement mNode = masksNode.firstChildElement(QStringLiteral("mask"));
         !mNode.isNull();
         mNode = mNode.nextSiblingElement(QStringLiteral("mask"))) {
      quint32 id = mNode.attribute(QStringLiteral("id"), QStringLiteral("1"))
                       .toUInt();
      QString name =
          mNode.attribute(QStringLiteral("name"),
                          QStringLiteral("Маска %1").arg(id));
      QUuid uid = QUuid::fromString(mNode.attribute(QStringLiteral("uid")));
      if (uid.isNull())
        uid = QUuid::createUuid();

      auto *mask = new MaskElement(id, name, uid);
      if (mNode.hasAttribute(QStringLiteral("text"))) {
        mask->setMaskText(mNode.attribute(QStringLiteral("text")));
      } else if (mNode.hasAttribute(QStringLiteral("maskText"))) {
        mask->setMaskText(mNode.attribute(QStringLiteral("maskText")));
      }

      QDomElement geomNode =
          mNode.firstChildElement(QStringLiteral("geometry"));
      if (!geomNode.isNull()) {
        qreal x = geomNode.attribute(QStringLiteral("x"), QStringLiteral("0"))
                      .toDouble();
        qreal y = geomNode.attribute(QStringLiteral("y"), QStringLiteral("0"))
                      .toDouble();
        qreal w =
            geomNode.attribute(QStringLiteral("width"), QStringLiteral("100"))
                .toDouble();
        qreal h =
            geomNode.attribute(QStringLiteral("height"), QStringLiteral("22"))
                .toDouble();
        qreal z = geomNode.attribute(QStringLiteral("z"), QStringLiteral("1"))
                      .toDouble();
        mask->setPos(x, y);
        mask->setRect(QRectF(0, 0, w, h));
        mask->setZValue(z);
      }

      QDomElement styleNode = mNode.firstChildElement(QStringLiteral("style"));
      if (!styleNode.isNull()) {
        mask->setFillColor(QColor(styleNode.attribute(
            QStringLiteral("fill"), QStringLiteral("#ffffff"))));
        mask->setBorderColor(QColor(styleNode.attribute(
            QStringLiteral("stroke"), QStringLiteral("#18181b"))));
        mask->setBorderWidth(
            styleNode
                .attribute(QStringLiteral("strokeWidth"), QStringLiteral("1.5"))
                .toDouble());
        mask->setBorderStyle(penStyleFromString(
            styleNode.attribute(QStringLiteral("strokeStyle"))));
      }

      tempScene.addItem(mask);
    }
  }

  // =========================================================================
  // Б. Обратная совместимость со старым форматом v1/v2 (<elements><element/>)
  // =========================================================================
  QDomElement legacyElementsNode =
      root.firstChildElement(QStringLiteral("elements"));
  QDomNodeList elemNodes = legacyElementsNode.isNull()
                               ? QDomNodeList()
                               : legacyElementsNode.elementsByTagName(
                                     QStringLiteral("element"));
  for (int i = 0; i < elemNodes.count(); ++i) {
    QDomElement elemNode = elemNodes.at(i).toElement();
    if (elemNode.isNull())
      continue;

    QString type = elemNode.attribute(QStringLiteral("type"));
    quint32 id = elemNode.attribute(QStringLiteral("id")).toUInt();
    QString name = elemNode.attribute(QStringLiteral("name"));
    QUuid uid = QUuid::fromString(elemNode.attribute(QStringLiteral("uid")));
    if (uid.isNull())
      uid = QUuid::createUuid();

    qreal x = elemNode.attribute(QStringLiteral("x")).toDouble();
    qreal y = elemNode.attribute(QStringLiteral("y")).toDouble();
    qreal z = elemNode.attribute(QStringLiteral("z")).toDouble();

    QColor stroke =
        QColor(elemNode.attribute(QStringLiteral("stroke"),
                                  QStringLiteral("#18181b")));
    qreal strokeWidth =
        elemNode.attribute(QStringLiteral("strokeWidth"), QStringLiteral("1.5"))
            .toDouble();
    Qt::PenStyle strokeStyle =
        penStyleFromString(elemNode.attribute(QStringLiteral("strokeStyle")));

    if (type == QStringLiteral("masterblock") ||
        type == QStringLiteral("rectangle")) {
      auto *block = new MasterBlockElement(id, name, uid);
      block->setPos(x, y);
      qreal w =
          elemNode.attribute(QStringLiteral("width"), QStringLiteral("180"))
              .toDouble();
      qreal h =
          elemNode.attribute(QStringLiteral("height"), QStringLiteral("110"))
              .toDouble();
      block->setRect(QRectF(0, 0, w, h));

      if (elemNode.hasAttribute(QStringLiteral("address"))) {
        block->setAddress(static_cast<quint16>(
            elemNode.attribute(QStringLiteral("address")).toUInt()));
      }
      if (elemNode.hasAttribute(QStringLiteral("stepName"))) {
        block->setStepName(elemNode.attribute(QStringLiteral("stepName")));
      }

      for (int lineIdx = 0; lineIdx < 6; ++lineIdx) {
        QString attrName = QStringLiteral("line%1").arg(lineIdx);
        if (elemNode.hasAttribute(attrName)) {
          block->setLineText(lineIdx, elemNode.attribute(attrName));
        }
      }
      for (int mIdx = 1; mIdx <= 15; ++mIdx) {
        QString attrName = QStringLiteral("m%1").arg(mIdx);
        if (elemNode.hasAttribute(attrName)) {
          block->setMField(
              mIdx, static_cast<quint16>(elemNode.attribute(attrName).toUInt()));
        }
      }

      block->setFillColor(QColor(
          elemNode.attribute(QStringLiteral("fill"), QStringLiteral("#ffffff"))));
      block->setBorderColor(stroke);
      block->setBorderWidth(strokeWidth);
      block->setBorderStyle(strokeStyle);
      block->setZValue(z);

      tempScene.addItem(block);
      uidToBlock[uid] = block;
      idToBlock[id] = block;
    } else if (type == QStringLiteral("connector") ||
               type == QStringLiteral("line")) {
      auto *conn = new ConnectorElement(id, name, uid);
      conn->setPos(x, y);
      qreal x1 =
          elemNode.attribute(QStringLiteral("x1"), QStringLiteral("0"))
              .toDouble();
      qreal y1 =
          elemNode.attribute(QStringLiteral("y1"), QStringLiteral("0"))
              .toDouble();
      qreal x2 =
          elemNode.attribute(QStringLiteral("x2"), QStringLiteral("120"))
              .toDouble();
      qreal y2 =
          elemNode.attribute(QStringLiteral("y2"), QStringLiteral("60"))
              .toDouble();
      conn->setP1(QPointF(x1, y1));
      conn->setP2(QPointF(x2, y2));

      bool hasExplicitRoute = false;
      if (elemNode.hasAttribute(QStringLiteral("points"))) {
        QString ptsStr = elemNode.attribute(QStringLiteral("points"));
        QVector<QPointF> pts;
        for (const QString &pair :
             ptsStr.split(QStringLiteral(";"), Qt::SkipEmptyParts)) {
          QStringList xy = pair.split(QStringLiteral(","));
          if (xy.size() == 2) {
            pts.append(QPointF(xy[0].toDouble(), xy[1].toDouble()));
          }
        }
        if (pts.size() >= 2) {
          conn->setPoints(pts);
          hasExplicitRoute = true;
        }
      }

      conn->setBeginMarker(elemNode.attribute(QStringLiteral("beginMarker")) ==
                                   QStringLiteral("arrow")
                               ? LineMarkerType::Arrow
                               : LineMarkerType::None);
      conn->setEndMarker(elemNode.attribute(QStringLiteral("endMarker")) ==
                                 QStringLiteral("arrow")
                             ? LineMarkerType::Arrow
                             : LineMarkerType::None);
      conn->setBorderColor(stroke);
      conn->setBorderWidth(strokeWidth);
      conn->setBorderStyle(strokeStyle);
      conn->setZValue(z);

      QUuid startUid =
          QUuid::fromString(elemNode.attribute(QStringLiteral("startBlockUid")));
      PortPosition startPort =
          portFromString(elemNode.attribute(QStringLiteral("startPort")));
      QUuid endUid =
          QUuid::fromString(elemNode.attribute(QStringLiteral("endBlockUid")));
      PortPosition endPort =
          portFromString(elemNode.attribute(QStringLiteral("endPort")));

      if (!startUid.isNull() || !endUid.isNull()) {
        PendingConn pc;
        pc.conn = conn;
        pc.startUid = startUid;
        pc.startPort = startPort;
        pc.endUid = endUid;
        pc.endPort = endPort;
        pc.hasExplicitRoute = hasExplicitRoute;
        pendingConnections.append(pc);
      }

      tempScene.addItem(conn);
    } else if (type == QStringLiteral("mask")) {
      auto *mask = new MaskElement(id, name, uid);
      mask->setPos(x, y);
      qreal w =
          elemNode.attribute(QStringLiteral("width"), QStringLiteral("100"))
              .toDouble();
      qreal h =
          elemNode.attribute(QStringLiteral("height"), QStringLiteral("22"))
              .toDouble();
      mask->setRect(QRectF(0, 0, w, h));
      mask->setMaskText(elemNode.attribute(QStringLiteral("maskText"),
                                           QStringLiteral("1111111XXX")));
      mask->setFillColor(QColor(
          elemNode.attribute(QStringLiteral("fill"), QStringLiteral("#ffffff"))));
      mask->setBorderColor(stroke);
      mask->setBorderWidth(strokeWidth);
      mask->setBorderStyle(strokeStyle);
      mask->setZValue(z);

      tempScene.addItem(mask);
    }
  }

  // Восстанавливаем привязки коннекторов к блокам (по UID или по целочисленному
  // ID блока)
  for (const auto &pc : pendingConnections) {
    MasterBlockElement *fromBlock = !pc.startUid.isNull()
        ? uidToBlock.value(pc.startUid, nullptr)
        : idToBlock.value(pc.startId, nullptr);
    MasterBlockElement *toBlock = !pc.endUid.isNull()
        ? uidToBlock.value(pc.endUid, nullptr)
        : idToBlock.value(pc.endId, nullptr);

    if (((pc.startId > 0 || !pc.startUid.isNull()) && !fromBlock) ||
        ((pc.endId > 0 || !pc.endUid.isNull()) && !toBlock)) {
      if (errorMessage)
        *errorMessage = QStringLiteral("Не найден блок, связанный с коннектором %1.")
                            .arg(pc.conn->id());
      return false;
    }

    if (fromBlock) {
      pc.conn->setStartConnection(fromBlock, pc.startPort);
    }
    if (toBlock) {
      pc.conn->setEndConnection(toBlock, pc.endPort);
    }
  }

  QHash<ConnectorElement *, QVector<QPointF>> explicitRoutes;
  for (const auto &pc : pendingConnections) {
    if (pc.hasExplicitRoute)
      explicitRoutes.insert(pc.conn, pc.conn->points());
  }
  for (auto *block : uidToBlock)
    tempScene.updateConnectedLines(block);
  for (auto it = explicitRoutes.cbegin(); it != explicitRoutes.cend(); ++it)
    it.key()->setPoints(it.value());

  scene->setGridSize(tempScene.gridSize());
  scene->setSnapToGrid(tempScene.isSnapToGrid());
  scene->setGridVisible(tempScene.isGridVisible());
  scene->setPageFormat(tempScene.pageFormat());
  scene->clearAll();
  const auto elements = tempScene.allElements();
  for (ShapeElement *element : elements) {
    tempScene.removeItem(element);
    scene->addItem(element);
    emit scene->elementAdded(element);
  }
  scene->update();
  return true;
}

bool ProjectStorage::loadFromFile(const QString &filePath, QString &projectName,
                                  CanvasScene *scene, QString *errorMessage) {
  if (!scene) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Холст не инициализирован.");
    return false;
  }
  QDomDocument doc;
  if (!readProject(filePath, doc, errorMessage))
    return false;
  const QDomElement root = doc.documentElement();
  const QDomElement pagesNode = root.firstChildElement(QStringLiteral("pages"));
  QDomElement pageNode = pagesNode.isNull() ? root : pagesNode.firstChildElement(QStringLiteral("page"));
  if (pageNode.isNull()) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Проект не содержит страниц.");
    return false;
  }
  if (!loadPage(pageNode, scene, errorMessage))
    return false;
  if (root.hasAttribute(QStringLiteral("name")))
    projectName = root.attribute(QStringLiteral("name"));
  return true;
}

bool ProjectStorage::loadFromFile(const QString &filePath, QString &projectName,
                                  QList<PageInfo> &pages, QString *errorMessage,
                                  QObject *sceneParent) {
  QDomDocument doc;
  if (!readProject(filePath, doc, errorMessage))
    return false;
  const QDomElement root = doc.documentElement();
  const QDomElement pagesNode = root.firstChildElement(QStringLiteral("pages"));
  QList<PageInfo> loadedPages;
  QDomElement pageNode = pagesNode.isNull() ? root : pagesNode.firstChildElement(QStringLiteral("page"));
  while (!pageNode.isNull()) {
    auto *scene = new CanvasScene();
    if (!loadPage(pageNode, scene, errorMessage)) {
      delete scene;
      for (const PageInfo &page : loadedPages)
        delete page.scene;
      return false;
    }
    loadedPages.append({pagesNode.isNull() ? QStringLiteral("Страница 1")
                                          : pageNode.attribute(QStringLiteral("name")), scene});
    pageNode = pagesNode.isNull() ? QDomElement() : pageNode.nextSiblingElement(QStringLiteral("page"));
  }
  if (loadedPages.isEmpty()) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Проект не содержит страниц.");
    return false;
  }
  for (const PageInfo &page : loadedPages)
    page.scene->setParent(sceneParent);
  pages = loadedPages;
  if (root.hasAttribute(QStringLiteral("name")))
    projectName = root.attribute(QStringLiteral("name"));
  return true;
}

bool ProjectStorage::exportK1804Binary(const QString &filePath,
                                       CanvasScene *scene, int *exportedCount,
                                       QString *errorMessage) {
  if (!scene) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Холст не инициализирован.");
    return false;
  }

  // Собираем все блоки K1804 MasterBlock
  QList<MasterBlockElement *> blocks;
  QSet<quint16> addresses;
  quint16 maxAddress = 0;

  for (ShapeElement *element : scene->allElements()) {
    if (auto *block = dynamic_cast<MasterBlockElement *>(element)) {
      if (block->address() > 0) {
        if (addresses.contains(block->address())) {
          const QString message = QStringLiteral("Обнаружен повторяющийся адрес блока: %1.")
                                      .arg(block->address());
          qWarning() << message;
          if (errorMessage)
            *errorMessage = message;
          return false;
        }
        addresses.insert(block->address());
        blocks.append(block);
        if (block->address() > maxAddress) {
          maxAddress = block->address();
        }
      }
    }
  }

  if (blocks.isEmpty()) {
    if (errorMessage)
      *errorMessage = QStringLiteral(
          "На холсте нет блоков с установленным адресом (Address > 0).");
    return false;
  }

  // Алгоритм SWDataExport из Visio VBA:
  // TmpPos = Val(TempAddress) * 10
  // Размер одной записи микрокоманды: ровно 10 байт (5 слов по 16 бит).
  // Выходной файл адресуется напрямую по адресу микрокоманды:
  const int recordSize = 10;
  const int totalBytes = (maxAddress + 1) * recordSize;
  QByteArray buffer(totalBytes, 0);

  for (MasterBlockElement *block : blocks) {
    int offset = block->address() * recordSize;
    if (offset + recordSize > buffer.size())
      continue;

    // Слово 0: Address (uint16)
    quint16 w0 = block->address();

    // Слово 1: M1 (uint16)
    quint16 w1 = block->mField(1);

    // Слово 2: (M6 << 11) | (M5 << 6) | (M3 << 2) | M2
    quint16 w2 = ((block->mField(6) & 0x1F) << 11) |
                 ((block->mField(5) & 0x1F) << 6) |
                 ((block->mField(3) & 0x0F) << 2) | (block->mField(2) & 0x03);

    // Слово 3: (M13 << 14) | (M10 << 11) | (M9 << 8) | (M8 << 6) | (M7 << 4) |
    // M4
    quint16 w3 = ((block->mField(13) & 0x03) << 14) |
                 ((block->mField(10) & 0x07) << 11) |
                 ((block->mField(9) & 0x07) << 8) |
                 ((block->mField(8) & 0x03) << 6) |
                 ((block->mField(7) & 0x03) << 4) | (block->mField(4) & 0x0F);

    // Слово 4: (M15 << 8) | (M14 << 6) | (M12 << 3) | M11
    quint16 w4 = ((block->mField(15) & 0xFF) << 8) |
                 ((block->mField(14) & 0x03) << 6) |
                 ((block->mField(12) & 0x07) << 3) | (block->mField(11) & 0x07);

    // Запись в формате Little-Endian (как в оригинальном Put #1 binary VBA)
    buffer[offset + 0] = static_cast<char>(w0 & 0xFF);
    buffer[offset + 1] = static_cast<char>((w0 >> 8) & 0xFF);

    buffer[offset + 2] = static_cast<char>(w1 & 0xFF);
    buffer[offset + 3] = static_cast<char>((w1 >> 8) & 0xFF);

    buffer[offset + 4] = static_cast<char>(w2 & 0xFF);
    buffer[offset + 5] = static_cast<char>((w2 >> 8) & 0xFF);

    buffer[offset + 6] = static_cast<char>(w3 & 0xFF);
    buffer[offset + 7] = static_cast<char>((w3 >> 8) & 0xFF);

    buffer[offset + 8] = static_cast<char>(w4 & 0xFF);
    buffer[offset + 9] = static_cast<char>((w4 >> 8) & 0xFF);
  }

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (errorMessage)
      *errorMessage = file.errorString();
    return false;
  }

  qint64 written = file.write(buffer);
  file.close();

  if (written != buffer.size()) {
    if (errorMessage)
      *errorMessage = QStringLiteral(
          "Не удалось записать полный объем данных бинарного файла.");
    return false;
  }

  if (exportedCount) {
    *exportedCount = blocks.size();
  }

  return true;
}

} // namespace editor
