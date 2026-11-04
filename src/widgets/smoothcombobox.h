#pragma once

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QEasingCurve>
#include <QFrame>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QWheelEvent>

namespace editor {

/**
 * @brief Выпадающий список, который всегда открывается строго вниз под полем
 *        с плавной анимацией раскрытия и аккуратным ограничением высоты/ширины.
 */
class SmoothComboBox : public QComboBox {
  Q_OBJECT

public:
  explicit SmoothComboBox(QWidget *parent = nullptr) : QComboBox(parent) {
    setMaxVisibleItems(8);
    setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    setMinimumContentsLength(10);
    setFocusPolicy(Qt::StrongFocus);

    if (view()) {
      view()->setTextElideMode(Qt::ElideRight);
      view()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    }

    anim_ = new QPropertyAnimation(this);
    anim_->setPropertyName("geometry");
    anim_->setDuration(140);
    anim_->setEasingCurve(QEasingCurve::OutCubic);
  }

  void setIgnoreWheelWhenUnfocused(bool ignore) {
    ignoreWheelWhenUnfocused_ = ignore;
  }

  void showPopup() override {
    if (anim_->state() == QAbstractAnimation::Running) {
      anim_->stop();
    }

    QComboBox::showPopup();

    QFrame *popup = findChild<QFrame *>();
    if (!popup) {
      return;
    }

    // Вычисляем оптимальную ширину и высоту выпадающего списка
    int count = this->count();
    int visibleRows = qMin(count, maxVisibleItems());
    int rowH = 28;
    if (view() && count > 0) {
      int hintH = view()->sizeHintForRow(0);
      if (hintH > 18) {
        rowH = hintH;
      }
    }
    int targetH = qMax(36, visibleRows * rowH + 8);

    int contentW = view() ? (view()->sizeHintForColumn(0) + 32) : width();
    int targetW = qMax(width(), qMin(contentW, 420));

    // Позиция строго под комбобоксом
    QPoint belowGlobal = mapToGlobal(QPoint(0, height() + 2));
    QScreen *scr = screen();
    if (!scr) {
      scr = QApplication::primaryScreen();
    }
    if (scr) {
      QRect avail = scr->availableGeometry();
      if (belowGlobal.x() + targetW > avail.right() - 4) {
        belowGlobal.setX(qMax(avail.left() + 4, avail.right() - targetW - 4));
      }
      // Если внизу совсем не помещается, уменьшаем высоту, но оставляем
      // открытие вниз
      int spaceBelow = avail.bottom() - belowGlobal.y() - 6;
      if (spaceBelow >= 110 && targetH > spaceBelow) {
        targetH = spaceBelow;
      } else if (spaceBelow < 110) {
        // Крайний случай: сам комбобокс у самого нижнего края экрана
        belowGlobal.setY(mapToGlobal(QPoint(0, -targetH - 2)).y());
      }
    }

    QRect startRect(belowGlobal.x(), belowGlobal.y(), targetW, 2);
    QRect endRect(belowGlobal.x(), belowGlobal.y(), targetW, targetH);

    popup->setMinimumHeight(0);
    popup->setGeometry(startRect);
    anim_->setTargetObject(popup);
    anim_->setStartValue(startRect);
    anim_->setEndValue(endRect);
    anim_->start();
  }

protected:
  void wheelEvent(QWheelEvent *event) override {
    if (ignoreWheelWhenUnfocused_) {
      event->ignore();
      return;
    }
    QComboBox::wheelEvent(event);
  }

private:
  QPropertyAnimation *anim_ = nullptr;
  bool ignoreWheelWhenUnfocused_ = true;
};

} // namespace editor