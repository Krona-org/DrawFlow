#include "smoothscrollarea.h"

#include <QEasingCurve>
#include <QScrollBar>
#include <QWheelEvent>
#include <QtGlobal>

namespace editor {

SmoothScrollArea::SmoothScrollArea(QWidget *parent) : QScrollArea(parent) {
  setObjectName("smoothScrollArea");
  m_animation = new QPropertyAnimation(verticalScrollBar(), "value", this);
  m_animation->setDuration(250);
  m_animation->setEasingCurve(QEasingCurve::OutCubic);
}

void SmoothScrollArea::wheelEvent(QWheelEvent *event) {
  QScrollBar *bar = verticalScrollBar();
  if (!bar || !bar->isVisible()) {
    QScrollArea::wheelEvent(event);
    return;
  }

  const int delta = event->angleDelta().y();
  if (delta == 0) {
    QScrollArea::wheelEvent(event);
    return;
  }

  if (m_animation->state() == QAbstractAnimation::Running) {
    m_animation->stop();
  } else {
    m_targetValue = bar->value();
  }

  const int step = bar->singleStep() * 6;
  m_targetValue -= (delta > 0 ? step : -step);
  m_targetValue = qBound(bar->minimum(), m_targetValue, bar->maximum());

  m_animation->setStartValue(bar->value());
  m_animation->setEndValue(m_targetValue);
  m_animation->start();
  event->accept();
}

} // namespace editor

