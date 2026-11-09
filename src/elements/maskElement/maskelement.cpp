#include "maskelement.h"
#include "src/themes/canvascolors.h"

#include <QFont>
#include <QPainter>

namespace editor {

MaskElement::MaskElement(quint32 id, const QString &name, const QUuid &uid,
                         QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    rect_ = QRectF(0.0, 0.0, 100.0, 22.0);
    borderColor_ = QColor(33, 37, 41);
    borderWidth_ = 1.0;
    // В Visio фигура Mask по умолчанию не имеет видимой рамки (LinePattern = 0)
    borderStyle_ = Qt::NoPen;
    fillStyle_ = Qt::SolidPattern;
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

void MaskElement::setRect(const QRectF &r) {
    QRectF norm = r.normalized();
    if (norm.width() < 30.0) norm.setWidth(30.0);
    if (norm.height() < 14.0) norm.setHeight(14.0);
    if (rect_ != norm) {
        prepareGeometryChange();
        rect_ = norm;
        update();
        emit geometryChanged();
    }
}

void MaskElement::setRectSize(qreal w, qreal h) {
    setRect(QRectF(rect_.topLeft(), QSizeF(qMax(30.0, w), qMax(14.0, h))));
}

void MaskElement::setMaskText(const QString &text) {
    if (maskText_ != text) {
        maskText_ = text;
        update();
        emit propertyChanged();
    }
}

void MaskElement::drawShape(QPainter *painter) {
    const QRectF r = rect_.normalized();

    if (fillStyle_ != Qt::NoBrush) {
        painter->setBrush(QBrush(canvasFillColor(fillColor_), fillStyle_));
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    if (borderStyle_ != Qt::NoPen) {
        QPen pen(borderColor_, borderWidth_, borderStyle_, Qt::SquareCap, Qt::MiterJoin);
        painter->setPen(pen);
    } else {
        // Тонкий ненавязчивый пунктирный контур-подсказка, чтобы плашку было видно на белом фоне
        QPen guidePen(QColor(203, 213, 225), 1.0, Qt::DashLine);
        painter->setPen(guidePen);
    }

    painter->drawRect(r);

    // Отрисовка текста битовой маски ("1111111XXX")
    const QColor textColor = isDarkCanvasTheme() && fillStyle_ == Qt::NoBrush &&
                                     borderColor_ == QColor(33, 37, 41)
                                 ? QColor(220, 220, 220) : borderColor_;
    painter->setPen(textColor);
    static const QFont font = []() {
        QFont f(QStringLiteral("Arial"));
        f.setPixelSize(11);
        return f;
    }();
    painter->setFont(font);
    painter->drawText(r.adjusted(3.0, 1.0, -3.0, -1.0), Qt::AlignCenter, maskText_);
}

} // namespace editor

