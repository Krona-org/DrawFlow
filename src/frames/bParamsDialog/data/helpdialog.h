#pragma once

#include <QDialog>

class QTabWidget;

namespace editor {

/**
 * @brief Справочное диалоговое окно со структурированным описанием
 *        архитектуры и всех микрокоманд К1804 по вкладкам.
 */
class HelpDialog : public QDialog {
  Q_OBJECT

public:
  explicit HelpDialog(QWidget *parent = nullptr);
  ~HelpDialog() override = default;

private:
  void setupUi();
  QWidget *createOverviewTab();
  QWidget *createTransitionsTab();
  QWidget *createAluTab();
  QWidget *createPswTab();
  QWidget *createBusesTab();
  QWidget *createExamplesTab();
};

} // namespace editor

