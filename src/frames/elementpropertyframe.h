#pragma once

#include "src/elements/shapeelement.h"

#include <QDockWidget>
#include <array>

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class ElementPropertyFrame;
}
QT_END_NAMESPACE

namespace editor {

class CanvasScene;
class MasterBlockElement;
class ConnectorElement;

class ElementPropertyFrame : public QDockWidget {
  Q_OBJECT

public:
  explicit ElementPropertyFrame(CanvasScene *scene, QWidget *parent = nullptr);
  ~ElementPropertyFrame() override;

  void setScene(CanvasScene *scene);

public slots:
  void updateProperties();

private slots:
  void onBlockHeaderEdited();
  void onBlockMFieldEdited();
  void onConnectorNameEdited(const QString &text);

private:
  void buildInspectorUi();
  void populateM3Combo(bool altMode, int keepIndex);
  void populateM7Combo(bool altMode, int keepIndex);
  void applyMicrocodeFromInspector();

  Ui::ElementPropertyFrame *ui = nullptr;
  CanvasScene *scene_ = nullptr;
  ShapeElement *currentElement_ = nullptr;
  bool isUpdating_ = false;

  QLabel *lblNoSelection_ = nullptr;

  // 1. Идентификация блока MasterBlock
  QGroupBox *groupBlockIdentity_ = nullptr;
  QLineEdit *editStepName_ = nullptr;
  QLineEdit *editAddressOct_ = nullptr;
  QLabel *lblAddressDec_ = nullptr;

  // 2. Идентификация коннектора / маски
  QGroupBox *groupConnIdentity_ = nullptr;
  QLineEdit *editConnName_ = nullptr;
  QLabel *lblMaskText_ = nullptr;
  QLineEdit *editMaskText_ = nullptr;

  // 3. Микрокоманда M1..M15 (редактирование прямо в панели Свойства)
  QGroupBox *groupMicrocode_ = nullptr;
  QLineEdit *editM1Oct_ = nullptr;
  QComboBox *comboM2_ = nullptr;
  QComboBox *comboM3_ = nullptr;
  QComboBox *comboM4_ = nullptr;
  QComboBox *comboM5_ = nullptr;
  QComboBox *comboM6_ = nullptr;
  QComboBox *comboM7_ = nullptr;
  QComboBox *comboM8_ = nullptr;
  QComboBox *comboM9_ = nullptr;
  QComboBox *comboM10_ = nullptr;
  QComboBox *comboM11_ = nullptr;
  QComboBox *comboM12_ = nullptr;
  QComboBox *comboM13_ = nullptr;
  QComboBox *comboM14_ = nullptr;
  QLineEdit *editM15Oct_ = nullptr;
  QPushButton *btnOpenDialog_ = nullptr;
};

} // namespace editor
