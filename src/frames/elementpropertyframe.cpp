#include "elementpropertyframe.h"
#include "src/canvas/canvasscene.h"
#include "src/elements/lineelement.h"
#include "src/elements/rectangleelement.h"
#include "ui_elementpropertyframe.h"

#include <QColorDialog>
#include <QPainter>
#include <QPixmap>

namespace editor {

ElementPropertyFrame::ElementPropertyFrame(CanvasScene *scene, QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::ElementPropertyFrame)
    , scene_(scene)
{
    ui->setupUi(this);

    // Инициализация выпадающих списков
    ui->comboBorderStyle->addItem(QStringLiteral("Сплошная"), static_cast<int>(Qt::SolidLine));
    ui->comboBorderStyle->addItem(QStringLiteral("Штриховая"), static_cast<int>(Qt::DashLine));
    ui->comboBorderStyle->addItem(QStringLiteral("Точечная"), static_cast<int>(Qt::DotLine));
    ui->comboBorderStyle->addItem(QStringLiteral("Штрих-пунктирная"), static_cast<int>(Qt::DashDotLine));

    ui->comboFillStyle->addItem(QStringLiteral("Сплошная"), static_cast<int>(Qt::SolidPattern));
    ui->comboFillStyle->addItem(QStringLiteral("Без заливки"), static_cast<int>(Qt::NoBrush));

    ui->comboBeginMarker->addItem(QStringLiteral("Нет"), static_cast<int>(LineMarkerType::None));
    ui->comboBeginMarker->addItem(QStringLiteral("Стрелка"), static_cast<int>(LineMarkerType::Arrow));

    ui->comboEndMarker->addItem(QStringLiteral("Нет"), static_cast<int>(LineMarkerType::None));
    ui->comboEndMarker->addItem(QStringLiteral("Стрелка"), static_cast<int>(LineMarkerType::Arrow));

    setupUiConnections();

    connect(scene_, &CanvasScene::sceneSelectionChanged, this, &ElementPropertyFrame::updateProperties);
    connect(scene_, &CanvasScene::elementRemoved, this, &ElementPropertyFrame::updateProperties);

    updateProperties();
}

ElementPropertyFrame::~ElementPropertyFrame() {
    delete ui;
}

void ElementPropertyFrame::setupUiConnections() {
    connect(ui->editName, &QLineEdit::textEdited, this, &ElementPropertyFrame::onNameChanged);

    connect(ui->spinZValue, &QDoubleSpinBox::valueChanged, this, [this](double val) {
        if (currentElement_ && !isUpdating_) {
            currentElement_->setZValue(val);
        }
    });

    // Геометрия прямоугольника
    connect(ui->spinX, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onGeometryChanged);
    connect(ui->spinY, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onGeometryChanged);
    connect(ui->spinWidth, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onGeometryChanged);
    connect(ui->spinHeight, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onGeometryChanged);

    // Геометрия линии
    connect(ui->spinX1, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onGeometryChanged);
    connect(ui->spinY1, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onGeometryChanged);
    connect(ui->spinX2, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onGeometryChanged);
    connect(ui->spinY2, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onGeometryChanged);

    // Контур
    connect(ui->btnBorderColor, &QPushButton::clicked, this, &ElementPropertyFrame::onBorderColorClicked);
    connect(ui->spinBorderWidth, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onBorderWidthChanged);
    connect(ui->comboBorderStyle, &QComboBox::currentIndexChanged, this, &ElementPropertyFrame::onBorderStyleChanged);

    // Заливка
    connect(ui->btnFillColor, &QPushButton::clicked, this, &ElementPropertyFrame::onFillColorClicked);
    connect(ui->comboFillStyle, &QComboBox::currentIndexChanged, this, &ElementPropertyFrame::onFillStyleChanged);
    connect(ui->spinRounding, &QDoubleSpinBox::valueChanged, this, &ElementPropertyFrame::onRoundingChanged);

    // Маркеры
    connect(ui->comboBeginMarker, &QComboBox::currentIndexChanged, this, &ElementPropertyFrame::onMarkerChanged);
    connect(ui->comboEndMarker, &QComboBox::currentIndexChanged, this, &ElementPropertyFrame::onMarkerChanged);
}

void ElementPropertyFrame::updateColorButton(QPushButton *btn, const QColor &color) {
    btn->setText(QStringLiteral("  %1").arg(color.name().toUpper()));

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    {
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(180, 190, 205), 1.0));
        p.setBrush(color);
        p.drawRoundedRect(QRectF(0.5, 0.5, 15, 15), 3.0, 3.0);
    }
    btn->setIcon(QIcon(pixmap));
    btn->setIconSize(QSize(16, 16));
    btn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #FFFFFF; color: #0F172A; border: 1px solid #CBD5E1; border-radius: 4px; font-weight: 600; padding: 3px 8px; font-size: 11px; text-align: left; }"
        "QPushButton:hover { background-color: #F8FAFC; border-color: #94A3B8; }"
        "QPushButton:pressed { background-color: #E2E8F0; }"
    ));
}

void ElementPropertyFrame::updateProperties() {
    ShapeElement *sel = scene_->selectedElement();

    if (currentElement_ != sel) {
        if (currentElement_) {
            disconnect(currentElement_, nullptr, this, nullptr);
        }
        currentElement_ = sel;
        if (currentElement_) {
            connect(currentElement_, &ShapeElement::geometryChanged, this, &ElementPropertyFrame::updateProperties);
            connect(currentElement_, &ShapeElement::propertyChanged, this, &ElementPropertyFrame::updateProperties);
        }
    }

    if (!currentElement_) {
        ui->noSelectionLabel->show();
        ui->groupGeneral->hide();
        ui->groupGeometry->hide();
        ui->groupLinePoints->hide();
        ui->groupBorder->hide();
        ui->groupFill->hide();
        ui->groupMarkers->hide();
        return;
    }

    isUpdating_ = true;
    ui->noSelectionLabel->hide();
    ui->groupGeneral->show();
    ui->groupBorder->show();

    ui->editName->setText(currentElement_->name());
    ui->spinZValue->setValue(currentElement_->zValue());

    updateColorButton(ui->btnBorderColor, currentElement_->borderColor());
    ui->spinBorderWidth->setValue(currentElement_->borderWidth());
    int bIndex = ui->comboBorderStyle->findData(static_cast<int>(currentElement_->borderStyle()));
    if (bIndex >= 0) ui->comboBorderStyle->setCurrentIndex(bIndex);

    if (auto rect = dynamic_cast<RectangleElement*>(currentElement_)) {
        ui->groupGeometry->show();
        ui->groupLinePoints->hide();
        ui->groupFill->show();
        ui->groupMarkers->hide();

        ui->spinX->setValue(rect->pos().x());
        ui->spinY->setValue(rect->pos().y());
        ui->spinWidth->setValue(rect->rect().width());
        ui->spinHeight->setValue(rect->rect().height());

        updateColorButton(ui->btnFillColor, rect->fillColor());
        int fIndex = ui->comboFillStyle->findData(static_cast<int>(rect->fillStyle()));
        if (fIndex >= 0) ui->comboFillStyle->setCurrentIndex(fIndex);

        ui->spinRounding->setValue(rect->rounding());
    } else if (auto line = dynamic_cast<LineElement*>(currentElement_)) {
        ui->groupGeometry->hide();
        ui->groupLinePoints->show();
        ui->groupFill->hide();
        ui->groupMarkers->show();

        ui->spinX1->setValue(line->pos().x() + line->p1().x());
        ui->spinY1->setValue(line->pos().y() + line->p1().y());
        ui->spinX2->setValue(line->pos().x() + line->p2().x());
        ui->spinY2->setValue(line->pos().y() + line->p2().y());

        int bMarkerIdx = ui->comboBeginMarker->findData(static_cast<int>(line->beginMarker()));
        if (bMarkerIdx >= 0) ui->comboBeginMarker->setCurrentIndex(bMarkerIdx);

        int eMarkerIdx = ui->comboEndMarker->findData(static_cast<int>(line->endMarker()));
        if (eMarkerIdx >= 0) ui->comboEndMarker->setCurrentIndex(eMarkerIdx);
    }

    isUpdating_ = false;
}

void ElementPropertyFrame::onNameChanged(const QString &text) {
    if (currentElement_ && !isUpdating_) {
        currentElement_->setName(text);
    }
}

void ElementPropertyFrame::onGeometryChanged() {
    if (!currentElement_ || isUpdating_) return;

    if (auto rect = dynamic_cast<RectangleElement*>(currentElement_)) {
        rect->setPos(ui->spinX->value(), ui->spinY->value());
        rect->setRectSize(ui->spinWidth->value(), ui->spinHeight->value());
    } else if (auto line = dynamic_cast<LineElement*>(currentElement_)) {
        QPointF p1(ui->spinX1->value(), ui->spinY1->value());
        QPointF p2(ui->spinX2->value(), ui->spinY2->value());
        line->setPos(p1);
        line->setP1(QPointF(0, 0));
        line->setP2(p2 - p1);
    }
}

void ElementPropertyFrame::onBorderColorClicked() {
    if (!currentElement_) return;
    QColor c = QColorDialog::getColor(currentElement_->borderColor(), this, QStringLiteral("Цвет контура"));
    if (c.isValid()) {
        currentElement_->setBorderColor(c);
        updateColorButton(ui->btnBorderColor, c);
    }
}

void ElementPropertyFrame::onBorderWidthChanged(double val) {
    if (currentElement_ && !isUpdating_) {
        currentElement_->setBorderWidth(val);
    }
}

void ElementPropertyFrame::onBorderStyleChanged(int index) {
    if (currentElement_ && !isUpdating_ && index >= 0) {
        auto style = static_cast<Qt::PenStyle>(ui->comboBorderStyle->currentData().toInt());
        currentElement_->setBorderStyle(style);
    }
}

void ElementPropertyFrame::onFillColorClicked() {
    auto rect = dynamic_cast<RectangleElement*>(currentElement_);
    if (!rect) return;
    QColor c = QColorDialog::getColor(rect->fillColor(), this, QStringLiteral("Цвет заливки"));
    if (c.isValid()) {
        rect->setFillColor(c);
        updateColorButton(ui->btnFillColor, c);
    }
}

void ElementPropertyFrame::onFillStyleChanged(int index) {
    auto rect = dynamic_cast<RectangleElement*>(currentElement_);
    if (rect && !isUpdating_ && index >= 0) {
        auto style = static_cast<Qt::BrushStyle>(ui->comboFillStyle->currentData().toInt());
        rect->setFillStyle(style);
    }
}

void ElementPropertyFrame::onRoundingChanged(double val) {
    auto rect = dynamic_cast<RectangleElement*>(currentElement_);
    if (rect && !isUpdating_) {
        rect->setRounding(val);
    }
}

void ElementPropertyFrame::onMarkerChanged() {
    auto line = dynamic_cast<LineElement*>(currentElement_);
    if (line && !isUpdating_) {
        auto bMarker = static_cast<LineMarkerType>(ui->comboBeginMarker->currentData().toInt());
        auto eMarker = static_cast<LineMarkerType>(ui->comboEndMarker->currentData().toInt());
        line->setBeginMarker(bMarker);
        line->setEndMarker(eMarker);
    }
}

} // namespace editor
