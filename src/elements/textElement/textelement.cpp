#include "textelement.h"
#include "src/themes/canvascolors.h"

#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QPainter>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <cmath>

namespace editor {
namespace {
class EditableTextItem : public QGraphicsTextItem {
public:
    explicit EditableTextItem(TextElement *owner) : QGraphicsTextItem(owner), owner_(owner) {}

protected:
    bool sceneEvent(QEvent *event) override {
        if (owner_->isEditing() && event->type() == QEvent::ShortcutOverride) {
            event->accept();
            return true;
        }
        return QGraphicsTextItem::sceneEvent(event);
    }
    void focusOutEvent(QFocusEvent *event) override {
        QGraphicsTextItem::focusOutEvent(event);
        owner_->finishEditing();
    }
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape) {
            owner_->finishEditing();
            event->accept();
            return;
        }
        QGraphicsTextItem::keyPressEvent(event);
    }
private:
    TextElement *owner_;
};
}

TextElement::TextElement(quint32 id, const QString &name, const QUuid &uid, QGraphicsItem *parent)
    : ShapeElement(id, name, uid, parent)
{
    rect_ = QRectF(0, 0, 120, 30);
    borderColor_ = QColor(33, 37, 41);
    borderWidth_ = 1.0;
    borderStyle_ = Qt::NoPen;
    fillStyle_ = Qt::NoBrush;
    textItem_ = new EditableTextItem(this);
    textItem_->setAcceptedMouseButtons(Qt::NoButton);
    textItem_->document()->setDocumentMargin(0);
    QTextOption option = textItem_->document()->defaultTextOption();
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textItem_->document()->setDefaultTextOption(option);
    textItem_->setPlainText(text_);
    connect(textItem_->document(), &QTextDocument::contentsChanged, this, [this] {
        if (updatingText_) return;
        const QString text = textItem_->toPlainText();
        if (text_ != text) {
            text_ = text;
            emit propertyChanged();
        }
        adjustHeightToDocument();
    });
    syncTextItemStyle();
    adjustHeightToDocument();
}

void TextElement::setRect(const QRectF &rect) {
    QRectF normalized = rect.normalized();
    if (normalized.width() < 40.0) normalized.setWidth(40.0);
    if (normalized.height() < 20.0) normalized.setHeight(20.0);
    if (rect_ != normalized) {
        prepareGeometryChange();
        rect_ = normalized;
        syncTextItemStyle();
        adjustHeightToDocument();
        update();
        emit geometryChanged();
    }
}

void TextElement::setText(const QString &text) {
    if (text_ != text) {
        text_ = text;
        updatingText_ = true;
        textItem_->setPlainText(text_);
        updatingText_ = false;
        adjustHeightToDocument();
        emit propertyChanged();
    }
}

void TextElement::setFontSize(int size) {
    size = qMax(1, size);
    if (fontSize_ != size) {
        fontSize_ = size;
        syncTextItemStyle();
        adjustHeightToDocument();
        emit propertyChanged();
    }
}

void TextElement::setTextColor(const QColor &color) {
    if (textColor_ != color) {
        textColor_ = color;
        syncTextItemStyle();
        emit propertyChanged();
    }
}

void TextElement::setFontFamily(const QString &family) {
    if (fontFamily_ != family) {
        fontFamily_ = family;
        syncTextItemStyle();
        adjustHeightToDocument();
        emit propertyChanged();
    }
}

void TextElement::syncTextItemStyle() {
    QFont font = textItem_->font();
    font.setFamily(fontFamily_.isEmpty() ? QStringLiteral("Segoe UI") : fontFamily_);
    font.setPointSize(fontSize_);
    if (textItem_->font() != font) textItem_->setFont(font);
    QColor color = textColor_;
    if (isDarkCanvasTheme() && (color == Qt::black || color == QColor(33, 37, 41)))
        color = QColor(235, 240, 245);
    if (textItem_->defaultTextColor() != color) textItem_->setDefaultTextColor(color);
    textItem_->setPos(rect_.left() + 6.0, rect_.top() + 2.0);
    const qreal width = qMax(1.0, rect_.width() - 12.0);
    if (!qFuzzyCompare(textItem_->textWidth(), width)) textItem_->setTextWidth(width);
}

void TextElement::adjustHeightToDocument() {
    const qreal height = qMax(20.0, textItem_->document()->size().height() + 4.0);
    if (rect_.height() < height) {
        prepareGeometryChange();
        rect_.setHeight(height);
        update();
        emit geometryChanged();
    }
}

void TextElement::startEditing(bool selectAll) {
    if (!isEditing_) {
        isEditing_ = true;
        textItem_->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
        textItem_->setTextInteractionFlags(Qt::TextEditorInteraction);
        textItem_->setFocus(Qt::MouseFocusReason);
    }
    QTextCursor cursor = textItem_->textCursor();
    if (selectAll) {
        cursor.select(QTextCursor::Document);
    } else {
        cursor.movePosition(QTextCursor::End);
    }
    textItem_->setTextCursor(cursor);
    update();
}

void TextElement::finishEditing() {
    if (!isEditing_) return;
    isEditing_ = false;
    QTextCursor cursor = textItem_->textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        textItem_->setTextCursor(cursor);
    }
    textItem_->clearFocus();
    textItem_->setTextInteractionFlags(Qt::NoTextInteraction);
    textItem_->setAcceptedMouseButtons(Qt::NoButton);
    update();
}

void TextElement::moveHandle(ResizeHandle handle, const QPointF &currentScenePos,
                             bool snapToGrid, qreal gridSize) {
    QPointF scenePos = currentScenePos;
    if (snapToGrid && gridSize > 1.0) {
        scenePos.setX(std::round(scenePos.x() / gridSize) * gridSize);
        scenePos.setY(std::round(scenePos.y() / gridSize) * gridSize);
    }
    const QPointF p = mapFromScene(scenePos) + (pos() - initialPos_);
    QRectF r = initialRect_;
    switch (handle) {
    case ResizeHandle::TopLeft: case ResizeHandle::Left: case ResizeHandle::BottomLeft:
        r.setLeft(qMin(p.x(), r.right() - 40.0)); break;
    case ResizeHandle::TopRight: case ResizeHandle::Right: case ResizeHandle::BottomRight:
        r.setRight(qMax(p.x(), r.left() + 40.0)); break;
    default: break;
    }
    switch (handle) {
    case ResizeHandle::TopLeft: case ResizeHandle::Top: case ResizeHandle::TopRight:
        r.setTop(qMin(p.y(), r.bottom() - 20.0)); break;
    case ResizeHandle::BottomLeft: case ResizeHandle::Bottom: case ResizeHandle::BottomRight:
        r.setBottom(qMax(p.y(), r.top() + 20.0)); break;
    default: break;
    }
    const bool topHandle = handle == ResizeHandle::TopLeft || handle == ResizeHandle::Top || handle == ResizeHandle::TopRight;
    const QPointF newPos = initialPos_ + r.topLeft();
    setRect(QRectF(0, 0, r.width(), r.height()));
    QPointF adjustedPos = newPos;
    if (topHandle) adjustedPos.setY(initialPos_.y() + initialRect_.bottom() - rect_.height());
    setPos(adjustedPos);
}

QVariant TextElement::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemSelectedChange && !value.toBool() && isEditing_)
        finishEditing();
    return ShapeElement::itemChange(change, value);
}

void TextElement::drawShape(QPainter *painter) {
    syncTextItemStyle();
    const QRectF r = rect_.normalized();
    if (fillStyle_ != Qt::NoBrush)
        painter->setBrush(QBrush(canvasFillColor(fillColor_), fillStyle_));
    else
        painter->setBrush(Qt::NoBrush);
    if (borderStyle_ != Qt::NoPen) {
        painter->setPen(QPen(borderColor_, borderWidth_, borderStyle_, Qt::SquareCap, Qt::MiterJoin));
    } else {
        QColor guideColor = isDarkCanvasTheme() ? QColor(120, 130, 145) : QColor(180, 190, 205);
        painter->setPen(QPen(guideColor, 1.0, Qt::DashLine));
    }
    painter->drawRect(r);
}

} // namespace editor
