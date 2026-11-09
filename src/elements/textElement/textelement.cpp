#include "textelement.h"

#include <QFont>
#include <QPainter>

namespace editor {

TextElement::TextElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    rect_ = QRectF(0, 0, 120, 30);
}

void TextElement::setRect(const QRectF &rect) {
    const QRectF normalized = rect.normalized();
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
    QFont font = painter->font();
    if (!fontFamily_.isEmpty()) {
        font.setFamily(fontFamily_);
    }
    font.setPointSize(fontSize_);
    painter->setFont(font);
    painter->setPen(textColor_);
    painter->save();
    painter->setClipRect(rect_.normalized(), Qt::IntersectClip);
    painter->drawText(rect_.normalized(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text_);
    painter->restore();
}

} // namespace editor
