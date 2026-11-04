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
    iconLabel_->setPixmap(icon.pixmap(30, 30));

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

    QString typeStr = QStringLiteral("connector");
    if (type_ == ElementType::MasterBlock) {
        typeStr = QStringLiteral("masterblock");
    } else if (type_ == ElementType::Mask) {
        typeStr = QStringLiteral("mask");
    }
    mimeData->setData(QStringLiteral("application/x-drawflow-element-type"), typeStr.toUtf8());
    drag->setMimeData(mimeData);

    QPixmap pixmap(60, 44);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);

    if (type_ == ElementType::MasterBlock) {
        // Отрисовка мини-блока с бейджами
        p.setPen(QPen(QColor(51, 65, 85), 1.2));
        p.setBrush(QColor(241, 245, 249, 230));
        p.drawRect(4, 10, 52, 30);

        // Верхние бейджи
        p.drawRect(8, 6, 12, 6);
        p.drawRect(40, 6, 12, 6);

        // Порты
        p.setPen(QPen(QColor(37, 99, 235), 1.5));
        p.drawPoint(4, 16);
        p.drawPoint(4, 25);
        p.drawPoint(4, 34);
        p.drawPoint(56, 16);
        p.drawPoint(56, 25);
        p.drawPoint(56, 34);
    } else if (type_ == ElementType::Mask) {
        // Отрисовка плашки битовой маски (1111111XXX)
        p.setPen(QPen(QColor(100, 116, 139), 1.0, Qt::DashLine));
        p.setBrush(QColor(255, 255, 255, 235));
        QRectF r(4, 14, 52, 16);
        p.drawRect(r);
        p.setPen(QColor(15, 23, 42));
        QFont f(QStringLiteral("Consolas"));
        f.setPixelSize(8);
        f.setBold(true);
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, QStringLiteral("1111111XXX"));
    } else {
        // Отрисовка ступени коннектора
        p.setPen(QPen(QColor(37, 99, 235), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(6, 12, 28, 12);
        p.drawLine(28, 12, 28, 32);
        p.drawLine(28, 32, 54, 32);
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

    masterBlockCard_ = new ShapeCard(ElementType::MasterBlock, QStringLiteral("K1804 MasterBlock"), QStringLiteral(":/masterblock.svg"), this);
    connectorCard_ = new ShapeCard(ElementType::Connector, QStringLiteral("Connector"), QStringLiteral(":/connector.svg"), this);
    maskCard_ = new ShapeCard(ElementType::Mask, QStringLiteral("Mask"), QStringLiteral(":/mask.svg"), this);

    // Вставляем карточки перед пружиной (verticalSpacer)
    ui->mainLayout->insertWidget(0, masterBlockCard_);
    ui->mainLayout->insertWidget(1, connectorCard_);
    ui->mainLayout->insertWidget(2, maskCard_);

    auto handleCardClick = [this](ElementType t) {
        if (scene_->activeTool() == t) {
            scene_->setActiveTool(ElementType::None);
        } else {
            scene_->setActiveTool(t);
        }
    };

    connect(masterBlockCard_, &ShapeCard::clicked, this, handleCardClick);
    connect(connectorCard_, &ShapeCard::clicked, this, handleCardClick);
    connect(maskCard_, &ShapeCard::clicked, this, handleCardClick);

    connect(scene_, &CanvasScene::activeToolChanged, this, &ElementsLibraryFrame::onToolChanged);
}

ElementsLibraryFrame::~ElementsLibraryFrame() {
    delete ui;
}

void ElementsLibraryFrame::setScene(CanvasScene *scene) {
    if (scene_ != scene) {
        if (scene_) {
            disconnect(scene_, nullptr, this, nullptr);
        }
        scene_ = scene;
        if (scene_) {
            connect(scene_, &CanvasScene::activeToolChanged, this, &ElementsLibraryFrame::onToolChanged);
            onToolChanged(scene_->activeTool());
        }
    }
}

void ElementsLibraryFrame::onToolChanged(ElementType tool) {
    if (masterBlockCard_) masterBlockCard_->setActive(tool == ElementType::MasterBlock);
    if (connectorCard_) connectorCard_->setActive(tool == ElementType::Connector);
    if (maskCard_) maskCard_->setActive(tool == ElementType::Mask);
}

} // namespace editor
