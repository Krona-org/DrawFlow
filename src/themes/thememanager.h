#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QToolBar>
#include <QColor>
#include <QFile>
#include <QList>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>
#include <QWidget>

namespace editor {

enum class Theme { Light, Dark };

class ThemeManager {
public:
  static void apply(QWidget *root, Theme theme = Theme::Light) {
    QFile lightFile(":/themes/light.qss");
    if (!lightFile.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString qss = QString::fromUtf8(lightFile.readAll());
    if (theme == Theme::Dark) {
      QFile darkFile(":/themes/dark.qss");
      if (!darkFile.open(QIODevice::ReadOnly | QIODevice::Text)) return;
      qss += QLatin1Char('\n') + QString::fromUtf8(darkFile.readAll());
    }

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette pal = QApplication::style()->standardPalette();
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
    if (theme == Theme::Dark) {
      pal.setColor(QPalette::Window, QColor("#1b2028"));
      pal.setColor(QPalette::WindowText, QColor("#dbe2ec"));
      pal.setColor(QPalette::Base, QColor("#171d25"));
      pal.setColor(QPalette::AlternateBase, QColor("#252e3a"));
      pal.setColor(QPalette::Text, QColor("#e2e8f0"));
      pal.setColor(QPalette::Button, QColor("#344253"));
      pal.setColor(QPalette::ButtonText, QColor("#e2e8f0"));
      pal.setColor(QPalette::Highlight, QColor("#42648b"));
      pal.setColor(QPalette::Disabled, QPalette::Text, QColor("#8894a3"));
      pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#8894a3"));
      pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#8894a3"));
    }
    QApplication::setPalette(pal);
    qApp->setProperty("drawflowDarkTheme", theme == Theme::Dark);

    enableStyledBackground(root);
    qApp->setStyleSheet(qss);
    if (root) {
      for (QToolBar *toolbar : root->findChildren<QToolBar *>()) {
        for (QAction *action : toolbar->actions()) {
          if (!action->property("drawflowOriginalIcon").isValid() && !action->icon().isNull())
            action->setProperty("drawflowOriginalIcon", QVariant::fromValue(action->icon()));
          const QIcon original = action->property("drawflowOriginalIcon").value<QIcon>();
          if (original.isNull()) continue;
          if (theme == Theme::Light) {
            action->setIcon(original);
          } else {
            QPixmap pixmap = original.pixmap(64, 64);
            QPainter painter(&pixmap);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(pixmap.rect(), QColor("#cbd5e1"));
            painter.end();
            action->setIcon(QIcon(pixmap));
          }
        }
      }
    }
  }

  static void refreshWidgetStyle(QWidget *w) {
    if (!w)
      return;
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
  }

private:
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

