#pragma once

#include "shapeelement.h"

namespace editor {

class MaskElement : public ShapeElement {
    Q_OBJECT

public:
    explicit MaskElement(quint32 id, const QString &name, const QUuid &uid,
                         QGraphicsItem *parent = nullptr);
    ~MaskElement() override = default;

    ElementType elementType() const override { return ElementType::Mask; }

    QRectF rect() const { return rect_; }
    void setRect(const QRectF &r);
    void setRectSize(qreal w, qreal h);

    QString maskText() const { return maskText_; }
    void setMaskText(const QString &text);

protected:
    void drawShape(QPainter *painter) override;

private:
    QString maskText_ = QStringLiteral("1111111XXX");
};

} // namespace editor

