#include "src/core/mainwindow.h"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QApplication::setOrganizationName(QStringLiteral("Krona"));
  QApplication::setApplicationName(QStringLiteral("DrawFlow"));
  app.setWindowIcon(QIcon(QStringLiteral(":/app_icon.png")));

  // Использовать чистый современный шрифт
  QFont font(QStringLiteral("Segoe UI"), 9);
  app.setFont(font);

  // Использовать стиль Fusion
  app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

  editor::MainWindow window;
  window.show();

  for (int i = 1; i < argc; ++i) {
    if (QString::fromLocal8Bit(argv[i]) == QStringLiteral("--screenshot") &&
        i + 1 < argc) {
      QString outPath = QString::fromLocal8Bit(argv[i + 1]);
      window.resize(1200, 780);
      app.processEvents();
      QPixmap pix = window.grab();
      pix.save(outPath);
      return 0;
    }
  }

  return app.exec();
}
