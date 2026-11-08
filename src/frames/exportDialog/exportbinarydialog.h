#pragma once

#include "src/core/projectstorage.h"

#include <QDialog>

class QComboBox;
class QLineEdit;

namespace editor {

class ExportBinaryDialog : public QDialog {
  Q_OBJECT

public:
  explicit ExportBinaryDialog(QList<PageInfo> pages, int defaultPageIndex,
                              const QString &defaultDirectory = QString(),
                              QWidget *parent = nullptr);

  int selectedPageIndex() const;
  QString selectedDirectory() const;
  QString selectedFileName() const;
  QString selectedFilePath() const;

public slots:
  void accept() override;

private:
  void updateFileNameForPage(int index);

  QList<PageInfo> pages_;
  QComboBox *pageCombo_ = nullptr;
  QLineEdit *dirEdit_ = nullptr;
  QLineEdit *fileNameEdit_ = nullptr;
};

} // namespace editor
