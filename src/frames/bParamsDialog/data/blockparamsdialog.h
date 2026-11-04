#pragma once

#include "src/elements/masterBlockElement/k1804microcode.h"
#include "src/elements/masterBlockElement/masterblockelement.h"

#include <QDialog>
#include <array>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

namespace editor {

class CanvasScene;

class BlockParamsDialog : public QDialog {
  Q_OBJECT

public:
  explicit BlockParamsDialog(MasterBlockElement *block, CanvasScene *scene,
                             QWidget *parent = nullptr);
  ~BlockParamsDialog() override = default;

private slots:
  void onHeaderEdited();
  void onM5ModeOrRegChanged();
  void onM5BitToggled();
  void onM6ModeOrRegChanged();
  void onM6BitToggled();
  void onM11Changed(int index);
  void onM2Changed(int index);
  void onM13Changed(int index);
  void onAnyComboChanged();
  void onResetDefaults();
  void onAcceptClicked();

private:
  void setupUi();
  void loadFromBlock();
  void syncM5WidgetsFromValue(quint16 m5Val);
  void syncM6WidgetsFromValue(quint16 m6Val);
  void populateM7Combo(bool altMode, int keepIndex);
  void populateM3Combo(bool altMode, int keepIndex);
  void rebuildCurrentState();
  bool validateCurrentState(QString *errorOut = nullptr) const;
  void updateLivePreview();

  MasterBlockElement *block_ = nullptr;
  CanvasScene *scene_ = nullptr;
  bool updatingUi_ = false;

  // Текущее редактируемое состояние
  QString currentStepName_;
  quint16 currentAddress_ = 1;
  std::array<quint16, 16> currentMFields_{};
  std::array<QString, 6> currentLines_;

  // Верхняя панель: Метка (Step) и Адрес МК (oct)
  QLineEdit *editStepName_ = nullptr;
  QLineEdit *editAddressOct_ = nullptr;
  QLabel *lblAddressDec_ = nullptr;
  QLabel *lblValidationStatus_ = nullptr;

  // Поля M1 .. M15 в строгом порядке M1 -> M15
  QLineEdit *editM1Oct_ = nullptr;
  QLabel *lblM1Dec_ = nullptr;
  QComboBox *comboM2_ = nullptr;
  QComboBox *comboM3_ = nullptr;
  QComboBox *comboM4_ = nullptr;

  QComboBox *comboM5Mode_ = nullptr;
  QComboBox *comboM5Reg_ = nullptr;
  std::array<QToolButton *, 5> btnM5Bits_{};
  QLabel *lblM5Oct_ = nullptr;

  QComboBox *comboM6Mode_ = nullptr;
  QComboBox *comboM6Reg_ = nullptr;
  std::array<QToolButton *, 5> btnM6Bits_{};
  QLabel *lblM6Oct_ = nullptr;

  QComboBox *comboM7_ = nullptr;
  QComboBox *comboM8_ = nullptr;
  QComboBox *comboM9_ = nullptr;
  QComboBox *comboM10_ = nullptr;
  QComboBox *comboM11_ = nullptr;
  QComboBox *comboM12_ = nullptr;
  QComboBox *comboM13_ = nullptr;
  QComboBox *comboM14_ = nullptr;

  QLineEdit *editM15Oct_ = nullptr;
  QLabel *lblM15Dec_ = nullptr;

  // Кнопки
  QPushButton *btnOk_ = nullptr;
  QPushButton *btnCancel_ = nullptr;
  QPushButton *btnReset_ = nullptr;
};

} // namespace editor
