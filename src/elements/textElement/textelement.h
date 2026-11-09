#pragma once

#include "src/elements/shapeElement/shapeelement.h"

namespace editor {

class TextElement : public ShapeElement {
    Q_OBJECT

public:
    explicit TextElement(quint32 id, const QString &name, const QUuid &uid,
                         QGraphicsItem *parent = nullptr);

    ElementType elementType() const override { return ElementType::Text; }

    QString text() const { return text_; }
    void setText(const QString &text);

    QRectF rect() const { return rect_; }
    void setRect(const QRectF &rect);

    int fontSize() const { return fontSize_; }
    void setFontSize(int size);

    QColor textColor() const { return textColor_; }
    void setTextColor(const QColor &color);

    QString fontFamily() const { return fontFamily_; }
    void setFontFamily(const QString &family);

protected:
    void drawShape(QPainter *painter) override;

private:
    QString text_ = QStringLiteral("Текст");
    int fontSize_ = 12;
    QColor textColor_ = Qt::black;
    QString fontFamily_;
};

} // namespace editor
