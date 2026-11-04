#include "projectstorage.h"

#include "src/elements/connectorelement.h"
#include "src/elements/masterblockelement.h"

#include <QDataStream>
#include <QDomDocument>
#include <QFile>
#include <QHash>
#include <QTextStream>

namespace editor {

static QString penStyleToString(Qt::PenStyle style) {
    switch (style) {
    case Qt::DashLine: return QStringLiteral("DashLine");
    case Qt::DotLine: return QStringLiteral("DotLine");
    case Qt::DashDotLine: return QStringLiteral("DashDotLine");
    default: return QStringLiteral("SolidLine");
    }
}

static Qt::PenStyle penStyleFromString(const QString &str) {
    if (str == QStringLiteral("DashLine")) return Qt::DashLine;
    if (str == QStringLiteral("DotLine")) return Qt::DotLine;
    if (str == QStringLiteral("DashDotLine")) return Qt::DashDotLine;
    return Qt::SolidLine;
}

static QString portToString(PortPosition port) {
    switch (port) {
    case PortPosition::InTop: return QStringLiteral("InTop");
    case PortPosition::InMid: return QStringLiteral("InMid");
    case PortPosition::InBot: return QStringLiteral("InBot");
    case PortPosition::OutTop: return QStringLiteral("OutTop");
    case PortPosition::OutMid: return QStringLiteral("OutMid");
    case PortPosition::OutBot: return QStringLiteral("OutBot");
    default: return QStringLiteral("None");
    }
}

static PortPosition portFromString(const QString &str) {
    if (str == QStringLiteral("InTop")) return PortPosition::InTop;
    if (str == QStringLiteral("InMid")) return PortPosition::InMid;
    if (str == QStringLiteral("InBot")) return PortPosition::InBot;
    if (str == QStringLiteral("OutTop")) return PortPosition::OutTop;
    if (str == QStringLiteral("OutMid")) return PortPosition::OutMid;
    if (str == QStringLiteral("OutBot")) return PortPosition::OutBot;
    // Поддержка старых портов
    if (str == QStringLiteral("Left")) return PortPosition::InMid;
    if (str == QStringLiteral("Right")) return PortPosition::OutMid;
    if (str == QStringLiteral("Top")) return PortPosition::InTop;
    if (str == QStringLiteral("Bottom")) return PortPosition::InBot;
    return PortPosition::None;
}

bool ProjectStorage::saveToFile(const QString &filePath, const QString &projectName, CanvasScene *scene, QString *errorMessage) {
    if (!scene) {
        if (errorMessage) *errorMessage = QStringLiteral("Холст не инициализирован.");
        return false;
    }

    QDomDocument doc;
    QDomProcessingInstruction header = doc.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\""));
    doc.appendChild(header);

    QDomElement root = doc.createElement(QStringLiteral("k1804project"));
    root.setAttribute(QStringLiteral("name"), projectName);
    root.setAttribute(QStringLiteral("version"), QStringLiteral("2.0"));
    root.setAttribute(QStringLiteral("gridSize"), QString::number(scene->gridSize()));
    root.setAttribute(QStringLiteral("snapToGrid"), scene->isSnapToGrid() ? QStringLiteral("true") : QStringLiteral("false"));
    root.setAttribute(QStringLiteral("showGrid"), scene->isGridVisible() ? QStringLiteral("true") : QStringLiteral("false"));
    doc.appendChild(root);

    QDomElement elementsNode = doc.createElement(QStringLiteral("elements"));
    root.appendChild(elementsNode);

    const auto elements = scene->allElements();
    for (ShapeElement *element : elements) {
        if (!element) continue;

        QDomElement elemNode = doc.createElement(QStringLiteral("element"));
        elemNode.setAttribute(QStringLiteral("id"), QString::number(element->id()));
        elemNode.setAttribute(QStringLiteral("name"), element->name());
        elemNode.setAttribute(QStringLiteral("uid"), element->uid().toString());
        elemNode.setAttribute(QStringLiteral("x"), QString::number(element->pos().x()));
        elemNode.setAttribute(QStringLiteral("y"), QString::number(element->pos().y()));
        elemNode.setAttribute(QStringLiteral("z"), QString::number(element->zValue()));
        elemNode.setAttribute(QStringLiteral("stroke"), element->borderColor().name());
        elemNode.setAttribute(QStringLiteral("strokeWidth"), QString::number(element->borderWidth()));
        elemNode.setAttribute(QStringLiteral("strokeStyle"), penStyleToString(element->borderStyle()));

        if (auto block = dynamic_cast<MasterBlockElement*>(element)) {
            elemNode.setAttribute(QStringLiteral("type"), QStringLiteral("masterblock"));
            elemNode.setAttribute(QStringLiteral("width"), QString::number(block->rect().width()));
            elemNode.setAttribute(QStringLiteral("height"), QString::number(block->rect().height()));
            elemNode.setAttribute(QStringLiteral("address"), QString::number(block->address()));
            elemNode.setAttribute(QStringLiteral("stepName"), block->stepName());
            elemNode.setAttribute(QStringLiteral("fill"), block->fillColor().name());

            // Сохраняем 6 строк микрокоманды
            for (int i = 0; i < 6; ++i) {
                elemNode.setAttribute(QStringLiteral("line%1").arg(i), block->lineText(i));
            }
            // Сохраняем поля M1..M15
            for (int i = 1; i <= 15; ++i) {
                elemNode.setAttribute(QStringLiteral("m%1").arg(i), QString::number(block->mField(i)));
            }
        } else if (auto conn = dynamic_cast<ConnectorElement*>(element)) {
            elemNode.setAttribute(QStringLiteral("type"), QStringLiteral("connector"));
            elemNode.setAttribute(QStringLiteral("x1"), QString::number(conn->p1().x()));
            elemNode.setAttribute(QStringLiteral("y1"), QString::number(conn->p1().y()));
            elemNode.setAttribute(QStringLiteral("x2"), QString::number(conn->p2().x()));
            elemNode.setAttribute(QStringLiteral("y2"), QString::number(conn->p2().y()));
            elemNode.setAttribute(QStringLiteral("isOrthogonal"), conn->isOrthogonal() ? QStringLiteral("true") : QStringLiteral("false"));
            elemNode.setAttribute(QStringLiteral("beginMarker"), conn->beginMarker() == LineMarkerType::Arrow ? QStringLiteral("arrow") : QStringLiteral("none"));
            elemNode.setAttribute(QStringLiteral("endMarker"), conn->endMarker() == LineMarkerType::Arrow ? QStringLiteral("arrow") : QStringLiteral("none"));

            if (conn->startBlock()) {
                elemNode.setAttribute(QStringLiteral("startBlockUid"), conn->startBlock()->uid().toString());
                elemNode.setAttribute(QStringLiteral("startPort"), portToString(conn->startPort()));
            }
            if (conn->endBlock()) {
                elemNode.setAttribute(QStringLiteral("endBlockUid"), conn->endBlock()->uid().toString());
                elemNode.setAttribute(QStringLiteral("endPort"), portToString(conn->endPort()));
            }

            // Сохраняем точки полилинии
            QString ptsStr;
            for (const QPointF &pt : conn->points()) {
                if (!ptsStr.isEmpty()) ptsStr.append(QStringLiteral(";"));
                ptsStr.append(QStringLiteral("%1,%2").arg(pt.x()).arg(pt.y()));
            }
            elemNode.setAttribute(QStringLiteral("points"), ptsStr);
        }

        elementsNode.appendChild(elemNode);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }

    QTextStream out(&file);
    doc.save(out, 4);
    file.close();

    return true;
}

bool ProjectStorage::loadFromFile(const QString &filePath, QString &projectName, CanvasScene *scene, QString *errorMessage) {
    if (!scene) {
        if (errorMessage) *errorMessage = QStringLiteral("Холст не инициализирован.");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }

    QDomDocument doc;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto result = doc.setContent(&file);
    if (!result) {
        if (errorMessage) *errorMessage = QStringLiteral("Ошибка разбора XML (строка %1, кол. %2): %3").arg(result.errorLine).arg(result.errorColumn).arg(result.errorMessage);
        file.close();
        return false;
    }
#else
    QString errorMsg;
    int errorLine = 0;
    int errorColumn = 0;
    if (!doc.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        if (errorMessage) *errorMessage = QStringLiteral("Ошибка разбора XML (строка %1, кол. %2): %3").arg(errorLine).arg(errorColumn).arg(errorMsg);
        file.close();
        return false;
    }
#endif
    file.close();

    QDomElement root = doc.documentElement();
    if (root.tagName() != QStringLiteral("k1804project") &&
        root.tagName() != QStringLiteral("dcsdiagram") &&
        root.tagName() != QStringLiteral("mxfile")) {
        if (errorMessage) *errorMessage = QStringLiteral("Неверный формат файла проекта.");
        return false;
    }

    if (root.hasAttribute(QStringLiteral("name"))) {
        projectName = root.attribute(QStringLiteral("name"));
    }
    if (root.hasAttribute(QStringLiteral("gridSize"))) {
        scene->setGridSize(root.attribute(QStringLiteral("gridSize")).toDouble());
    }
    if (root.hasAttribute(QStringLiteral("snapToGrid"))) {
        scene->setSnapToGrid(root.attribute(QStringLiteral("snapToGrid")) == QStringLiteral("true"));
    }
    if (root.hasAttribute(QStringLiteral("showGrid"))) {
        scene->setGridVisible(root.attribute(QStringLiteral("showGrid")) == QStringLiteral("true"));
    }

    scene->clearAll();

    QHash<QUuid, MasterBlockElement*> uidToBlock;
    struct PendingConn {
        ConnectorElement *conn;
        QUuid startUid;
        PortPosition startPort;
        QUuid endUid;
        PortPosition endPort;
    };
    QList<PendingConn> pendingConnections;

    QDomNodeList elemNodes = root.elementsByTagName(QStringLiteral("element"));
    for (int i = 0; i < elemNodes.count(); ++i) {
        QDomElement elemNode = elemNodes.at(i).toElement();
        if (elemNode.isNull()) continue;

        QString type = elemNode.attribute(QStringLiteral("type"));
        quint32 id = elemNode.attribute(QStringLiteral("id")).toUInt();
        QString name = elemNode.attribute(QStringLiteral("name"));
        QUuid uid = QUuid::fromString(elemNode.attribute(QStringLiteral("uid")));
        if (uid.isNull()) uid = QUuid::createUuid();

        qreal x = elemNode.attribute(QStringLiteral("x")).toDouble();
        qreal y = elemNode.attribute(QStringLiteral("y")).toDouble();
        qreal z = elemNode.attribute(QStringLiteral("z")).toDouble();

        QColor stroke = QColor(elemNode.attribute(QStringLiteral("stroke"), QStringLiteral("#18181b")));
        qreal strokeWidth = elemNode.attribute(QStringLiteral("strokeWidth"), QStringLiteral("1.5")).toDouble();
        Qt::PenStyle strokeStyle = penStyleFromString(elemNode.attribute(QStringLiteral("strokeStyle")));

        if (type == QStringLiteral("masterblock") || type == QStringLiteral("rectangle")) {
            auto block = new MasterBlockElement(id, name, uid);
            block->setPos(x, y);
            qreal w = elemNode.attribute(QStringLiteral("width"), QStringLiteral("180")).toDouble();
            qreal h = elemNode.attribute(QStringLiteral("height"), QStringLiteral("110")).toDouble();
            block->setRect(QRectF(0, 0, w, h));

            if (elemNode.hasAttribute(QStringLiteral("address"))) {
                block->setAddress(static_cast<quint16>(elemNode.attribute(QStringLiteral("address")).toUInt()));
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
                    block->setMField(mIdx, static_cast<quint16>(elemNode.attribute(attrName).toUInt()));
                }
            }

            block->setFillColor(QColor(elemNode.attribute(QStringLiteral("fill"), QStringLiteral("#ffffff"))));
            block->setBorderColor(stroke);
            block->setBorderWidth(strokeWidth);
            block->setBorderStyle(strokeStyle);
            block->setZValue(z);

            scene->addItem(block);
            uidToBlock[uid] = block;
            emit scene->elementAdded(block);
        } else if (type == QStringLiteral("connector") || type == QStringLiteral("line")) {
            auto conn = new ConnectorElement(id, name, uid);
            conn->setPos(x, y);
            qreal x1 = elemNode.attribute(QStringLiteral("x1"), QStringLiteral("0")).toDouble();
            qreal y1 = elemNode.attribute(QStringLiteral("y1"), QStringLiteral("0")).toDouble();
            qreal x2 = elemNode.attribute(QStringLiteral("x2"), QStringLiteral("120")).toDouble();
            qreal y2 = elemNode.attribute(QStringLiteral("y2"), QStringLiteral("60")).toDouble();
            conn->setP1(QPointF(x1, y1));
            conn->setP2(QPointF(x2, y2));

            if (elemNode.hasAttribute(QStringLiteral("points"))) {
                QString ptsStr = elemNode.attribute(QStringLiteral("points"));
                QVector<QPointF> pts;
                for (const QString &pair : ptsStr.split(QStringLiteral(";"), Qt::SkipEmptyParts)) {
                    QStringList xy = pair.split(QStringLiteral(","));
                    if (xy.size() == 2) {
                        pts.append(QPointF(xy[0].toDouble(), xy[1].toDouble()));
                    }
                }
                if (pts.size() >= 2) {
                    conn->setPoints(pts);
                }
            }

            conn->setBeginMarker(elemNode.attribute(QStringLiteral("beginMarker")) == QStringLiteral("arrow") ? LineMarkerType::Arrow : LineMarkerType::None);
            conn->setEndMarker(elemNode.attribute(QStringLiteral("endMarker")) == QStringLiteral("arrow") ? LineMarkerType::Arrow : LineMarkerType::None);
            conn->setBorderColor(stroke);
            conn->setBorderWidth(strokeWidth);
            conn->setBorderStyle(strokeStyle);
            conn->setZValue(z);

            QUuid startUid = QUuid::fromString(elemNode.attribute(QStringLiteral("startBlockUid")));
            PortPosition startPort = portFromString(elemNode.attribute(QStringLiteral("startPort")));
            QUuid endUid = QUuid::fromString(elemNode.attribute(QStringLiteral("endBlockUid")));
            PortPosition endPort = portFromString(elemNode.attribute(QStringLiteral("endPort")));

            if (!startUid.isNull() || !endUid.isNull()) {
                pendingConnections.append({conn, startUid, startPort, endUid, endPort});
            }

            scene->addItem(conn);
            emit scene->elementAdded(conn);
        }
    }

    // Восстанавливаем привязки коннекторов к блокам
    for (const auto &pc : pendingConnections) {
        if (!pc.startUid.isNull() && uidToBlock.contains(pc.startUid)) {
            pc.conn->setStartConnection(uidToBlock[pc.startUid], pc.startPort);
        }
        if (!pc.endUid.isNull() && uidToBlock.contains(pc.endUid)) {
            pc.conn->setEndConnection(uidToBlock[pc.endUid], pc.endPort);
        }
    }

    // Обновляем маршрутизацию коннекторов
    for (auto block : uidToBlock) {
        scene->updateConnectedLines(block);
    }

    return true;
}

bool ProjectStorage::exportK1804Binary(const QString &filePath, CanvasScene *scene, int *exportedCount, QString *errorMessage) {
    if (!scene) {
        if (errorMessage) *errorMessage = QStringLiteral("Холст не инициализирован.");
        return false;
    }

    // Собираем все блоки K1804 MasterBlock
    QList<MasterBlockElement*> blocks;
    quint16 maxAddress = 0;

    for (ShapeElement *element : scene->allElements()) {
        if (auto block = dynamic_cast<MasterBlockElement*>(element)) {
            if (block->address() > 0) {
                blocks.append(block);
                if (block->address() > maxAddress) {
                    maxAddress = block->address();
                }
            }
        }
    }

    if (blocks.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("На холсте нет блоков с установленным адресом (Address > 0).");
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
        if (offset + recordSize > buffer.size()) continue;

        // Слово 0: Address (uint16)
        quint16 w0 = block->address();

        // Слово 1: M1 (uint16)
        quint16 w1 = block->mField(1);

        // Слово 2: (M6 << 11) | (M5 << 6) | (M3 << 2) | M2
        quint16 w2 = ((block->mField(6) & 0x1F) << 11) |
                     ((block->mField(5) & 0x1F) << 6) |
                     ((block->mField(3) & 0x0F) << 2) |
                     (block->mField(2) & 0x03);

        // Слово 3: (M13 << 14) | (M10 << 11) | (M9 << 8) | (M8 << 6) | (M7 << 4) | M4
        quint16 w3 = ((block->mField(13) & 0x03) << 14) |
                     ((block->mField(10) & 0x07) << 11) |
                     ((block->mField(9)  & 0x07) << 8) |
                     ((block->mField(8)  & 0x03) << 6) |
                     ((block->mField(7)  & 0x03) << 4) |
                     (block->mField(4)  & 0x0F);

        // Слово 4: (M15 << 8) | (M14 << 6) | (M12 << 3) | M11
        quint16 w4 = ((block->mField(15) & 0xFF) << 8) |
                     ((block->mField(14) & 0x03) << 6) |
                     ((block->mField(12) & 0x07) << 3) |
                     (block->mField(11) & 0x07);

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
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }

    qint64 written = file.write(buffer);
    file.close();

    if (written != buffer.size()) {
        if (errorMessage) *errorMessage = QStringLiteral("Не удалось записать полный объем данных бинарного файла.");
        return false;
    }

    if (exportedCount) {
        *exportedCount = blocks.size();
    }

    return true;
}

} // namespace editor
