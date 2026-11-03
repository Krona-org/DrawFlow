#include "projectstorage.h"

#include "src/elements/lineelement.h"
#include "src/elements/rectangleelement.h"

#include <QDomDocument>
#include <QFile>
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

bool ProjectStorage::saveToFile(const QString &filePath, const QString &projectName, CanvasScene *scene, QString *errorMessage) {
    if (!scene) {
        if (errorMessage) *errorMessage = QStringLiteral("Холст не инициализирован.");
        return false;
    }

    QDomDocument doc;
    QDomProcessingInstruction header = doc.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\""));
    doc.appendChild(header);

    QDomElement root = doc.createElement(QStringLiteral("dcsdiagram"));
    root.setAttribute(QStringLiteral("name"), projectName);
    root.setAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
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

        if (auto rect = dynamic_cast<RectangleElement*>(element)) {
            elemNode.setAttribute(QStringLiteral("type"), QStringLiteral("rectangle"));
            elemNode.setAttribute(QStringLiteral("width"), QString::number(rect->rect().width()));
            elemNode.setAttribute(QStringLiteral("height"), QString::number(rect->rect().height()));
            elemNode.setAttribute(QStringLiteral("rounding"), QString::number(rect->rounding()));
            elemNode.setAttribute(QStringLiteral("fill"), rect->fillColor().name());
        } else if (auto line = dynamic_cast<LineElement*>(element)) {
            elemNode.setAttribute(QStringLiteral("type"), QStringLiteral("line"));
            elemNode.setAttribute(QStringLiteral("x1"), QString::number(line->p1().x()));
            elemNode.setAttribute(QStringLiteral("y1"), QString::number(line->p1().y()));
            elemNode.setAttribute(QStringLiteral("x2"), QString::number(line->p2().x()));
            elemNode.setAttribute(QStringLiteral("y2"), QString::number(line->p2().y()));
            elemNode.setAttribute(QStringLiteral("beginMarker"), line->beginMarker() == LineMarkerType::Arrow ? QStringLiteral("arrow") : QStringLiteral("none"));
            elemNode.setAttribute(QStringLiteral("endMarker"), line->endMarker() == LineMarkerType::Arrow ? QStringLiteral("arrow") : QStringLiteral("none"));
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
    if (root.tagName() != QStringLiteral("dcsdiagram") && root.tagName() != QStringLiteral("mxfile")) {
        if (errorMessage) *errorMessage = QStringLiteral("Неверный формат файла: отсутствует корневой тег dcsdiagram.");
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

    scene->clear();

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

        QColor stroke = QColor(elemNode.attribute(QStringLiteral("stroke"), QStringLiteral("#212529")));
        qreal strokeWidth = elemNode.attribute(QStringLiteral("strokeWidth"), QStringLiteral("1.5")).toDouble();
        Qt::PenStyle strokeStyle = penStyleFromString(elemNode.attribute(QStringLiteral("strokeStyle")));

        if (type == QStringLiteral("rectangle")) {
            auto rect = new RectangleElement(id, name, uid);
            rect->setPos(x, y);
            qreal w = elemNode.attribute(QStringLiteral("width"), QStringLiteral("120")).toDouble();
            qreal h = elemNode.attribute(QStringLiteral("height"), QStringLiteral("80")).toDouble();
            rect->setRect(QRectF(0, 0, w, h));
            rect->setRounding(elemNode.attribute(QStringLiteral("rounding")).toDouble());
            rect->setFillColor(QColor(elemNode.attribute(QStringLiteral("fill"), QStringLiteral("#ffffff"))));
            rect->setBorderColor(stroke);
            rect->setBorderWidth(strokeWidth);
            rect->setBorderStyle(strokeStyle);
            rect->setZValue(z);

            scene->addItem(rect);
            emit scene->elementAdded(rect);
        } else if (type == QStringLiteral("line")) {
            auto line = new LineElement(id, name, uid);
            line->setPos(x, y);
            qreal x1 = elemNode.attribute(QStringLiteral("x1"), QStringLiteral("0")).toDouble();
            qreal y1 = elemNode.attribute(QStringLiteral("y1"), QStringLiteral("0")).toDouble();
            qreal x2 = elemNode.attribute(QStringLiteral("x2"), QStringLiteral("120")).toDouble();
            qreal y2 = elemNode.attribute(QStringLiteral("y2"), QStringLiteral("60")).toDouble();
            line->setP1(QPointF(x1, y1));
            line->setP2(QPointF(x2, y2));
            line->setBeginMarker(elemNode.attribute(QStringLiteral("beginMarker")) == QStringLiteral("arrow") ? LineMarkerType::Arrow : LineMarkerType::None);
            line->setEndMarker(elemNode.attribute(QStringLiteral("endMarker")) == QStringLiteral("arrow") ? LineMarkerType::Arrow : LineMarkerType::None);
            line->setBorderColor(stroke);
            line->setBorderWidth(strokeWidth);
            line->setBorderStyle(strokeStyle);
            line->setZValue(z);

            scene->addItem(line);
            emit scene->elementAdded(line);
        }
    }

    return true;
}

} // namespace editor
