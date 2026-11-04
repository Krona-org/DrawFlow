#include "canvastabwidget.h"
#include "canvasview.h"
#include "src/themes/thememanager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace editor {

CanvasTabWidget::CanvasTabWidget(CanvasView *view, QWidget *parent)
    : QWidget(parent), view_(view) {
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

  auto barLayout = new QHBoxLayout(bottomBar_);
  barLayout->setContentsMargins(8, 2, 8, 2);
  barLayout->setSpacing(4);

  pagesLayout_ = new QHBoxLayout();
  pagesLayout_->setContentsMargins(0, 0, 0, 0);
  pagesLayout_->setSpacing(3);
  barLayout->addLayout(pagesLayout_);

  // Первая страница по умолчанию
  auto firstBtn = new QPushButton(QStringLiteral("Страница 1"), bottomBar_);
  firstBtn->setObjectName(QStringLiteral("pageTabBtn"));
  firstBtn->setFixedHeight(20);
  connect(firstBtn, &QPushButton::clicked, this,
          &CanvasTabWidget::onPageButtonClicked);
  pageButtons_.append(firstBtn);
  pagesLayout_->addWidget(firstBtn);

  // Кнопка добавления новой страницы "+"
  btnAddPage_ = new QPushButton(QStringLiteral("+"), bottomBar_);
  btnAddPage_->setObjectName(QStringLiteral("addPageBtn"));
  btnAddPage_->setFixedSize(20, 20);
  btnAddPage_->setToolTip(QStringLiteral("Добавить страницу"));
  connect(btnAddPage_, &QPushButton::clicked, this,
          &CanvasTabWidget::onAddPageClicked);
  barLayout->addWidget(btnAddPage_);

  barLayout->addStretch();

  // Индикатор масштаба справа
  zoomLabel_ = new QLabel(QStringLiteral("100%"), bottomBar_);
  zoomLabel_->setObjectName(QStringLiteral("zoomLabel"));
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
  auto btn = new QPushButton(QStringLiteral("Страница %1").arg(newIndex),
                             bottomBar_);
  btn->setObjectName(QStringLiteral("pageTabBtn"));
  btn->setFixedHeight(20);
  connect(btn, &QPushButton::clicked, this,
          &CanvasTabWidget::onPageButtonClicked);
  pageButtons_.append(btn);
  pagesLayout_->addWidget(btn);

  currentPageIndex_ = pageButtons_.size() - 1;
  updatePageButtonsStyle();
  emit pageAdded();
  emit pageChanged(currentPageIndex_);
}

void CanvasTabWidget::onPageButtonClicked() {
  auto btn = qobject_cast<QPushButton *>(sender());
  if (!btn)
    return;
  int idx = pageButtons_.indexOf(btn);
  if (idx >= 0 && idx != currentPageIndex_) {
    currentPageIndex_ = idx;
    updatePageButtonsStyle();
    emit pageChanged(currentPageIndex_);
  }
}

void CanvasTabWidget::updatePageButtonsStyle() {
  for (int i = 0; i < pageButtons_.size(); ++i) {
    QPushButton *btn = pageButtons_[i];
    btn->setProperty("active", i == currentPageIndex_);
    ThemeManager::refreshWidgetStyle(btn);
  }
}

} // namespace editor
