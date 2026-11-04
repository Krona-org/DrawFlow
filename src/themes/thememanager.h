#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QList>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>
#include <QWidget>

namespace editor {

enum class Theme { Light };

class ThemeManager {
public:
  static void apply(QWidget *root, Theme theme = Theme::Light) {
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette pal = QApplication::palette();
    pal.setColor(QPalette::Window, QColor("#E5E9F0"));
    pal.setColor(QPalette::WindowText, QColor("#1E242B"));
    pal.setColor(QPalette::Base, QColor("#F4F6F9"));
    pal.setColor(QPalette::AlternateBase, QColor("#ECEFF4"));
    pal.setColor(QPalette::ToolTipBase, QColor("#1E242B"));
    pal.setColor(QPalette::ToolTipText, QColor("#F8FAFC"));
    pal.setColor(QPalette::Text, QColor("#1E242B"));
    pal.setColor(QPalette::Button, QColor("#D8DEE9"));
    pal.setColor(QPalette::ButtonText, QColor("#1E242B"));
    pal.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    pal.setColor(QPalette::Highlight, QColor("#3B82F6"));
    pal.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    QApplication::setPalette(pal);

    clearInlineStyles(root);
    enableStyledBackground(root);

    QString path;
    switch (theme) {
    case Theme::Light:
    default:
      path = ":/themes/light.qss";
      break;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
      return;

    QTextStream in(&f);
    const QString qss = in.readAll();
    qApp->setStyleSheet(qss);
  }

  static void refreshWidgetStyle(QWidget *w) {
    if (!w)
      return;
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
  }

private:
  static void clearInlineStyles(QWidget *root) {
    if (!root)
      return;
    root->setStyleSheet(QString());
    const auto children = root->findChildren<QWidget *>();
    for (QWidget *w : children) {
      w->setStyleSheet(QString());
    }
  }

  static void enableStyledBackground(QWidget *root) {
    if (!root)
      return;
    root->setAttribute(Qt::WA_StyledBackground, true);
    const auto children = root->findChildren<QWidget *>();
    for (QWidget *w : children) {
      w->setAttribute(Qt::WA_StyledBackground, true);
    }
  }
};

} // namespace editor

using editor::Theme;
using editor::ThemeManager;

#endif // THEMEMANAGER_H

