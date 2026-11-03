#include "canvastabwidget.h"
#include "canvasview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace editor {

CanvasTabWidget::CanvasTabWidget(CanvasView *view, QWidget *parent)
    : QWidget(parent)
    , view_(view)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Холст занимает всю доступную высоту и ширину
    mainLayout->addWidget(view_, 1);

    // Компактная нижняя полоса страниц фиксированной высоты 26px (как в draw.io)
    bottomBar_ = new QWidget(this);
    bottomBar_->setObjectName(QStringLiteral("canvasBottomBar"));
    bottomBar_->setAttribute(Qt::WA_StyledBackground, true);
    bottomBar_->setFixedHeight(26);
    bottomBar_->setStyleSheet(QStringLiteral(
        "#canvasBottomBar { background-color: #eaedf0; border-top: 1px solid #cbd5e1; }"
    ));

    auto barLayout = new QHBoxLayout(bottomBar_);
    barLayout->setContentsMargins(8, 2, 8, 2);
    barLayout->setSpacing(4);

    pagesLayout_ = new QHBoxLayout();
    pagesLayout_->setContentsMargins(0, 0, 0, 0);
    pagesLayout_->setSpacing(3);
    barLayout->addLayout(pagesLayout_);

    // Первая страница по умолчанию
    auto firstBtn = new QPushButton(QStringLiteral("Страница 1"), bottomBar_);
    firstBtn->setFixedHeight(20);
    connect(firstBtn, &QPushButton::clicked, this, &CanvasTabWidget::onPageButtonClicked);
    pageButtons_.append(firstBtn);
    pagesLayout_->addWidget(firstBtn);

    // Кнопка добавления новой страницы "+"
    btnAddPage_ = new QPushButton(QStringLiteral("+"), bottomBar_);
    btnAddPage_->setFixedSize(20, 20);
    btnAddPage_->setToolTip(QStringLiteral("Добавить страницу"));
    btnAddPage_->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: transparent; border: 1px solid transparent; border-radius: 3px; font-size: 13px; font-weight: bold; color: #64748b; padding: 0; }"
        "QPushButton:hover { background-color: #cbd5e1; color: #0f172a; }"
        "QPushButton:pressed { background-color: #94a3b8; }"
    ));
    connect(btnAddPage_, &QPushButton::clicked, this, &CanvasTabWidget::onAddPageClicked);
    barLayout->addWidget(btnAddPage_);

    barLayout->addStretch();

    // Индикатор масштаба справа
    zoomLabel_ = new QLabel(QStringLiteral("100%"), bottomBar_);
    zoomLabel_->setStyleSheet(QStringLiteral("font-size: 11px; color: #64748b; padding-right: 4px;"));
    barLayout->addWidget(zoomLabel_);

    mainLayout->addWidget(bottomBar_, 0);

    updatePageButtonsStyle();
}

void CanvasTabWidget::setZoomText(const QString &text) {
    if (zoomLabel_) {
        zoomLabel_->setText(text);
    }
}

void CanvasTabWidget::onAddPageClicked() {
    int newIndex = pageButtons_.size() + 1;
    auto btn = new QPushButton(QStringLiteral("Страница %1").arg(newIndex), bottomBar_);
    btn->setFixedHeight(20);
    connect(btn, &QPushButton::clicked, this, &CanvasTabWidget::onPageButtonClicked);
    pageButtons_.append(btn);
    pagesLayout_->addWidget(btn);

    currentPageIndex_ = pageButtons_.size() - 1;
    updatePageButtonsStyle();
    emit pageChanged(currentPageIndex_);
}

void CanvasTabWidget::onPageButtonClicked() {
    auto btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int idx = pageButtons_.indexOf(btn);
    if (idx >= 0 && idx != currentPageIndex_) {
        currentPageIndex_ = idx;
        updatePageButtonsStyle();
        emit pageChanged(currentPageIndex_);
    }
}

void CanvasTabWidget::updatePageButtonsStyle() {
    const QString activeStyle = QStringLiteral(
        "QPushButton { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-bottom: none; font-weight: 600; font-size: 11px; padding: 1px 10px; border-radius: 3px; }"
    );
    const QString inactiveStyle = QStringLiteral(
        "QPushButton { background-color: transparent; color: #64748b; border: 1px solid transparent; font-size: 11px; padding: 1px 10px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #cbd5e1; color: #0f172a; }"
    );

    for (int i = 0; i < pageButtons_.size(); ++i) {
        pageButtons_[i]->setStyleSheet(i == currentPageIndex_ ? activeStyle : inactiveStyle);
    }
}

} // namespace editor
