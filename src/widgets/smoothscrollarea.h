#ifndef SMOOTHSCROLLAREA_H
#define SMOOTHSCROLLAREA_H

#include <QPropertyAnimation>
#include <QScrollArea>

namespace editor {

class SmoothScrollArea : public QScrollArea {
  Q_OBJECT
public:
  explicit SmoothScrollArea(QWidget *parent = nullptr);

protected:
  void wheelEvent(QWheelEvent *event) override;

private:
  QPropertyAnimation *m_animation = nullptr;
  int m_targetValue = 0;
};

} // namespace editor

using editor::SmoothScrollArea;

#endif // SMOOTHSCROLLAREA_H

