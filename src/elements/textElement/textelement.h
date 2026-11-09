#pragma once

#include "src/elements/shapeElement/shapeelement.h"

class QGraphicsTextItem;

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

    void startEditing(bool selectAll = false);
    void startEditingAt(const QPointF &scenePos);
    void finishEditing();
    bool isEditing() const { return isEditing_; }

    void moveHandle(ResizeHandle handle, const QPointF &currentScenePos,
                    bool snapToGrid = false, qreal gridSize = 20.0) override;

protected:
    void drawShape(QPainter *painter) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void syncTextItemStyle();
    void adjustHeightToDocument();

    QString text_ = QStringLiteral("Текст");
    int fontSize_ = 12;
    QColor textColor_ = Qt::black;
    QString fontFamily_;
    QGraphicsTextItem *textItem_ = nullptr;
    bool isEditing_ = false;
    bool updatingText_ = false;
    QString editingStartText_;
    QRectF editingStartRect_;
};

} // namespace editor
