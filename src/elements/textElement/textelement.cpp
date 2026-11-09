#include "textelement.h"
#include "src/themes/canvascolors.h"

#include <QFont>
#include <QPainter>

namespace editor {

TextElement::TextElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    rect_ = QRectF(0, 0, 120, 30);
    borderColor_ = QColor(33, 37, 41);
    borderWidth_ = 1.0;
    borderStyle_ = Qt::NoPen;
    fillStyle_ = Qt::NoBrush;
}

void TextElement::setRect(const QRectF &rect) {
    QRectF normalized = rect.normalized();
    if (normalized.width() < 40.0) normalized.setWidth(40.0);
    if (normalized.height() < 20.0) normalized.setHeight(20.0);
    if (rect_ != normalized) {
        prepareGeometryChange();
        rect_ = normalized;
        update();
        emit geometryChanged();
    }
}

void TextElement::setText(const QString &text) {
    if (text_ != text) {
        text_ = text;
        update();
        emit propertyChanged();
    }
}

void TextElement::setFontSize(int size) {
    size = qMax(1, size);
    if (fontSize_ != size) {
        fontSize_ = size;
        update();
        emit propertyChanged();
    }
}

void TextElement::setTextColor(const QColor &color) {
    if (textColor_ != color) {
        textColor_ = color;
        update();
        emit propertyChanged();
    }
}

void TextElement::setFontFamily(const QString &family) {
    if (fontFamily_ != family) {
        fontFamily_ = family;
        update();
        emit propertyChanged();
    }
}

void TextElement::drawShape(QPainter *painter) {
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
        QColor guideColor = isDarkCanvasTheme() ? QColor(120, 130, 145) : QColor(180, 190, 205);
        QPen guidePen(guideColor, 1.0, Qt::DashLine);
        painter->setPen(guidePen);
    }
    painter->drawRect(r);

    QFont font = painter->font();
    if (!fontFamily_.isEmpty()) {
        font.setFamily(fontFamily_);
    } else {
        font.setFamily(QStringLiteral("Segoe UI"));
    }
    font.setPointSize(fontSize_);
    painter->setFont(font);

    QColor effectiveTextColor = textColor_;
    if (isDarkCanvasTheme() && (effectiveTextColor == Qt::black || effectiveTextColor == QColor(33, 37, 41))) {
        effectiveTextColor = QColor(235, 240, 245);
    }
    painter->setPen(effectiveTextColor);
    painter->save();
    painter->setClipRect(r, Qt::IntersectClip);
    painter->drawText(r.adjusted(6.0, 2.0, -6.0, -2.0), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text_);
    painter->restore();
}

} // namespace editor
