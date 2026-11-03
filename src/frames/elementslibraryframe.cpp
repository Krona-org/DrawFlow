#include "elementslibraryframe.h"
#include "src/canvas/canvasscene.h"
#include "ui_elementslibraryframe.h"

#include <QApplication>
#include <QDrag>
#include <QIcon>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace editor {

ShapeCard::ShapeCard(ElementType type, const QString &title, const QString &iconPath, QWidget *parent)
    : QFrame(parent)
    , type_(type)
    , title_(title)
{
    setObjectName(QStringLiteral("shapeCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::PointingHandCursor);
    setFrameShape(QFrame::NoFrame);
    setToolTip(QStringLiteral("%1\n(Перетащите на холст или нажмите для выбора)").arg(title));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 6);
    layout->setSpacing(4);
    layout->setAlignment(Qt::AlignCenter);

    iconLabel_ = new QLabel(this);
    iconLabel_->setAlignment(Qt::AlignCenter);
    iconLabel_->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    QIcon icon(iconPath);
    iconLabel_->setPixmap(icon.pixmap(28, 28));

    textLabel_ = new QLabel(title, this);
    textLabel_->setAlignment(Qt::AlignCenter);
    textLabel_->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 500; color: #334155; border: none; background: transparent;"));

    layout->addWidget(iconLabel_);
    layout->addWidget(textLabel_);

    setActive(false);
}

void ShapeCard::setActive(bool active) {
    isActive_ = active;
    if (active) {
        setStyleSheet(QStringLiteral(
            "#shapeCard { background-color: #cbd5e1; border: 1.5px solid #64748b; border-radius: 4px; }"
            "QLabel { border: none; background: transparent; }"
        ));
        if (textLabel_) textLabel_->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600; color: #0f172a; border: none; background: transparent;"));
    } else {
        setStyleSheet(QStringLiteral(
            "#shapeCard { background-color: #ffffff; border: 1px solid #cbd5e1; border-radius: 4px; }"
            "#shapeCard:hover { background-color: #f8fafc; border-color: #94a3b8; }"
            "QLabel { border: none; background: transparent; }"
        ));
        if (textLabel_) textLabel_->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 500; color: #334155; border: none; background: transparent;"));
    }
}

void ShapeCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragStartPos_ = event->pos();
    }
    QFrame::mousePressEvent(event);
}

void ShapeCard::mouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    if ((event->pos() - dragStartPos_).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    auto drag = new QDrag(this);
    auto mimeData = new QMimeData();

    QString typeStr = (type_ == ElementType::Rectangle) ? QStringLiteral("rectangle") : QStringLiteral("line");
    mimeData->setData(QStringLiteral("application/x-dcs-element-type"), typeStr.toUtf8());
    drag->setMimeData(mimeData);

    QPixmap pixmap(56, 40);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);

    if (type_ == ElementType::Rectangle) {
        p.setPen(QPen(QColor(51, 65, 85), 1.5));
        p.setBrush(QColor(241, 245, 249, 220));
        p.drawRoundedRect(3, 3, 50, 34, 3, 3);
    } else {
        p.setPen(QPen(QColor(51, 65, 85), 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(5, 35, 51, 5);
    }
    p.end();

    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

    drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void ShapeCard::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton &&
        (event->pos() - dragStartPos_).manhattanLength() < QApplication::startDragDistance()) {
        emit clicked(type_);
    }
    QFrame::mouseReleaseEvent(event);
}

ElementsLibraryFrame::ElementsLibraryFrame(CanvasScene *scene, QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::ElementsLibraryFrame)
    , scene_(scene)
{
    ui->setupUi(this);

    rectCard_ = new ShapeCard(ElementType::Rectangle, QStringLiteral("Прямоугольник"), QStringLiteral(":/rectangle.svg"), this);
    lineCard_ = new ShapeCard(ElementType::Line, QStringLiteral("Линия"), QStringLiteral(":/line.svg"), this);

    // Вставляем карточки перед пружиной (verticalSpacer)
    ui->mainLayout->insertWidget(0, rectCard_);
    ui->mainLayout->insertWidget(1, lineCard_);

    auto handleCardClick = [this](ElementType t) {
        if (scene_->activeTool() == t) {
            scene_->setActiveTool(ElementType::None);
        } else {
            scene_->setActiveTool(t);
        }
    };

    connect(rectCard_, &ShapeCard::clicked, this, handleCardClick);
    connect(lineCard_, &ShapeCard::clicked, this, handleCardClick);

    connect(scene_, &CanvasScene::activeToolChanged, this, &ElementsLibraryFrame::onToolChanged);
}

ElementsLibraryFrame::~ElementsLibraryFrame() {
    delete ui;
}

void ElementsLibraryFrame::onToolChanged(ElementType tool) {
    if (rectCard_) rectCard_->setActive(tool == ElementType::Rectangle);
    if (lineCard_) lineCard_->setActive(tool == ElementType::Line);
}

} // namespace editor
