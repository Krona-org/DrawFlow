#pragma once

#include <QApplication>
#include <QColor>

namespace editor {

// Document colors remain untouched; only the standard light-theme colors get
// an alternative representation while the dark theme is active.
inline bool isDarkCanvasTheme() {
  return qApp && qApp->property("drawflowDarkTheme").toBool();
}

inline QColor canvasFillColor(const QColor &color) {
  return isDarkCanvasTheme() && color == QColor(242, 244, 247)
             ? QColor(190, 190, 190) : color;
}

inline QColor canvasConnectorColor(const QColor &color) {
  return isDarkCanvasTheme() && color == QColor(24, 24, 27)
             ? QColor(220, 220, 220) : color;
}

} // namespace editor
