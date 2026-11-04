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
 *        с плавной анимацией раскрытия и строго по ширине самого поля.
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
      if (QWidget * container = view()->parentWidget()) {
        container->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint |
                                  Qt::NoDropShadowWindowHint);
      }
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

    // Вычисляем высоту и фиксируем ширину ровно по ширине самого комбобокса,
    // чтобы выпадающий список никогда не выходил за пределы панели или окна
    int count = this->count();
    int visibleRows = qMin(count, maxVisibleItems());
    int rowH = 26;
    if (view() && count > 0) {
      int hintH = view()->sizeHintForRow(0);
      if (hintH > 18) {
        rowH = hintH;
      }
    }
    int targetH = qMax(32, visibleRows * rowH + 6);
    int targetW = width();

    // Позиция строго под комбобоксом
    QPoint belowGlobal = mapToGlobal(QPoint(0, height() + 2));
    QScreen *scr = screen();
    if (!scr) {
      scr = QApplication::primaryScreen();
    }
    if (scr) {
      QRect avail = scr->availableGeometry();
      if (belowGlobal.x() + targetW > avail.right() - 2) {
        belowGlobal.setX(qMax(avail.left() + 2, avail.right() - targetW - 2));
      }
      int spaceBelow = avail.bottom() - belowGlobal.y() - 6;
      if (spaceBelow >= 110 && targetH > spaceBelow) {
        targetH = spaceBelow;
      } else if (spaceBelow < 110) {
        belowGlobal.setY(mapToGlobal(QPoint(0, -targetH - 2)).y());
      }
    }

    QRect startRect(belowGlobal.x(), belowGlobal.y(), targetW, 2);
    QRect endRect(belowGlobal.x(), belowGlobal.y(), targetW, targetH);

    popup->setMinimumWidth(targetW);
    popup->setMaximumWidth(targetW);
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
