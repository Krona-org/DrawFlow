#include "elementpropertyframe.h"
#include "src/widgets/smoothcombobox.h"
#include "src/widgets/smoothscrollarea.h"
#include "ui_elementpropertyframe.h"

#include "src/canvas/canvasscene.h"
#include "src/canvas/undocommands.h"
#include "src/elements/connectorElement/connectorelement.h"
#include "src/elements/maskElement/maskelement.h"
#include "src/elements/masterBlockElement/k1804microcode.h"
#include "src/elements/masterBlockElement/masterblockelement.h"

#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

namespace editor {

namespace {

QString portAddressOptionText(int val) {
  QString oct = K1804Microcode::toOctal(static_cast<quint16>(val), 2);
  if (val <= 15) {
    return QStringLiteral("[%1] РОН R%2").arg(oct).arg(val);
  }
  if (val <= 23) {
    return QStringLiteral("[%1] RK[2/0]").arg(oct);
  }
  return QStringLiteral("[%1] RK[8/6]").arg(oct);
}

} // namespace

ElementPropertyFrame::ElementPropertyFrame(CanvasScene *scene, QWidget *parent)
    : QDockWidget(parent), ui(new Ui::ElementPropertyFrame), scene_(scene) {
  ui->setupUi(this);
  setMinimumWidth(265);

  buildInspectorUi();

  if (scene_) {
    connect(scene_, &CanvasScene::sceneSelectionChanged, this,
            &ElementPropertyFrame::updateProperties);
    connect(scene_, &CanvasScene::elementRemoved, this,
            &ElementPropertyFrame::updateProperties);
  }
  updateProperties();
}

ElementPropertyFrame::~ElementPropertyFrame() { delete ui; }

void ElementPropertyFrame::setScene(CanvasScene *scene) {
  if (scene_ != scene) {
    if (scene_) {
      disconnect(scene_, nullptr, this, nullptr);
    }
    scene_ = scene;
    currentElement_ = nullptr;
    if (scene_) {
      connect(scene_, &CanvasScene::sceneSelectionChanged, this,
              &ElementPropertyFrame::updateProperties);
      connect(scene_, &CanvasScene::elementRemoved, this,
              &ElementPropertyFrame::updateProperties);
    }
    updateProperties();
  }
}

void ElementPropertyFrame::buildInspectorUi() {
  // Очищаем старый placeholder из .ui и создаем аккуратную прокручиваемую
  // панель свойств с плавной прокруткой (SmoothScrollArea)
  if (ui->placeholderLabel) {
    ui->placeholderLabel->hide();
  }
  ui->mainLayout->setContentsMargins(0, 0, 0, 0);
  ui->mainLayout->setSpacing(0);

  while (ui->mainLayout->count() > 0) {
    QLayoutItem *it = ui->mainLayout->takeAt(0);
    if (it->widget()) {
      it->widget()->hide();
    }
    delete it;
  }

  auto *scrollArea = new SmoothScrollArea(ui->dockWidgetContents);
  scrollArea->setObjectName(QStringLiteral("propertyScrollArea"));
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  auto *container = new QWidget(scrollArea);
  container->setObjectName(QStringLiteral("inspectorContainer"));
  container->setAttribute(Qt::WA_StyledBackground, true);

  auto *vbox = new QVBoxLayout(container);
  vbox->setContentsMargins(8, 6, 8, 10);
  vbox->setSpacing(6);

  lblNoSelection_ = new QLabel(QStringLiteral("Элемент не выбран"), container);
  lblNoSelection_->setObjectName(QStringLiteral("noSelectionLabel"));
  lblNoSelection_->setAlignment(Qt::AlignCenter);
  vbox->addWidget(lblNoSelection_);

  auto *octValidator16 = new QRegularExpressionValidator(
      QRegularExpression(QStringLiteral("^[0-7]{1,6}$")), this);
  auto *octValidator8 = new QRegularExpressionValidator(
      QRegularExpression(QStringLiteral("^[0-7]{1,3}$")), this);

  // =========================================================================
  // 1. ПАРАМЕТРЫ БЛОКА K1804 MASTERBLOCK (Метка, Адрес)
  // =========================================================================
  groupBlockIdentity_ = new QGroupBox(QStringLiteral("Блок К1804"), container);
  auto *formBlockId = new QFormLayout(groupBlockIdentity_);
  formBlockId->setContentsMargins(8, 8, 8, 8);
  formBlockId->setHorizontalSpacing(6);
  formBlockId->setVerticalSpacing(5);

  editStepName_ = new QLineEdit(groupBlockIdentity_);
  formBlockId->addRow(QStringLiteral("Метка (лев.):"), editStepName_);

  auto *addrRow = new QWidget(groupBlockIdentity_);
  auto *addrLayout = new QHBoxLayout(addrRow);
  addrLayout->setContentsMargins(0, 0, 0, 0);
  addrLayout->setSpacing(6);
  editAddressOct_ = new QLineEdit(addrRow);
  editAddressOct_->setValidator(octValidator16);
  lblAddressDec_ = new QLabel(QStringLiteral("(= 1 dec)"), addrRow);
  lblAddressDec_->setObjectName(QStringLiteral("octDecHintLabel"));
  addrLayout->addWidget(editAddressOct_, 1);
  addrLayout->addWidget(lblAddressDec_);
  formBlockId->addRow(QStringLiteral("Адрес (oct):"), addrRow);

  vbox->addWidget(groupBlockIdentity_);

  // =========================================================================
  // 2. ПАРАМЕТРЫ КОННЕКТОРА / МАСКИ
  // =========================================================================
  groupConnIdentity_ = new QGroupBox(QStringLiteral("Коннектор"), container);
  auto *formConnId = new QFormLayout(groupConnIdentity_);
  formConnId->setContentsMargins(8, 8, 8, 8);
  formConnId->setHorizontalSpacing(6);
  formConnId->setVerticalSpacing(5);

  editConnName_ = new QLineEdit(groupConnIdentity_);
  formConnId->addRow(QStringLiteral("Имя:"), editConnName_);

  lblMaskText_ = new QLabel(QStringLiteral("Маска:"), groupConnIdentity_);
  editMaskText_ = new QLineEdit(groupConnIdentity_);
  editMaskText_->setToolTip(
      QStringLiteral("Текст битовой маски на схеме (например, 1111111XXX)"));
  formConnId->addRow(lblMaskText_, editMaskText_);

  vbox->addWidget(groupConnIdentity_);

  // =========================================================================
  // 3. МИКРОКОМАНДА (M1 .. M15 В СТРОГОМ ПОРЯДКЕ)
  // =========================================================================
  groupMicrocode_ =
      new QGroupBox(QStringLiteral("Микрокоманда (M1 – M15)"), container);
  auto *mcVBox = new QVBoxLayout(groupMicrocode_);
  mcVBox->setContentsMargins(8, 8, 8, 8);
  mcVBox->setSpacing(6);

  auto *formMc = new QFormLayout();
  formMc->setContentsMargins(0, 0, 0, 0);
  formMc->setHorizontalSpacing(6);
  formMc->setVerticalSpacing(4);

  auto makeMLabel = [this](const QString &code, const QString &tip) {
    auto *lbl = new QLabel(code, groupMicrocode_);
    lbl->setObjectName(QStringLiteral("mFieldLabel"));
    lbl->setToolTip(tip);
    return lbl;
  };

  // M1
  editM1Oct_ = new QLineEdit(groupMicrocode_);
  editM1Oct_->setValidator(octValidator16);
  editM1Oct_->setToolTip(
      QStringLiteral("M1: Константа / Адрес перехода (0..177777 oct)"));
  formMc->addRow(
      makeMLabel(QStringLiteral("M1 (Конст.):"), editM1Oct_->toolTip()),
      editM1Oct_);

  // M2
  comboM2_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m2Options()) {
    comboM2_->addItem(
        QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description),
        opt.code);
  }
  formMc->addRow(makeMLabel(QStringLiteral("M2 (Мод.):"),
                            QStringLiteral("Модификация условия")),
                 comboM2_);

  // M3
  comboM3_ = new SmoothComboBox(groupMicrocode_);
  populateM3Combo(false, 0);
  formMc->addRow(makeMLabel(QStringLiteral("M3 (Усл.):"),
                            QStringLiteral("Проверяемое условие TST")),
                 comboM3_);

  // M4
  comboM4_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m4Options()) {
    comboM4_->addItem(
        QStringLiteral("[%1] %2 — %3")
            .arg(opt.octalCode, opt.shortText, opt.description),
        opt.code);
  }
  formMc->addRow(makeMLabel(QStringLiteral("M4 (БМУ):"),
                            QStringLiteral("Команда перехода БМУ")),
                 comboM4_);

  // M5
  comboM5_ = new SmoothComboBox(groupMicrocode_);
  for (int v = 0; v < 32; ++v) {
    comboM5_->addItem(portAddressOptionText(v), v);
  }
  formMc->addRow(
      makeMLabel(QStringLiteral("M5 (Порт A):"),
                 QStringLiteral("Адрес порта A АЛУ (00..37 oct)")),
      comboM5_);

  // M6
  comboM6_ = new SmoothComboBox(groupMicrocode_);
  for (int v = 0; v < 32; ++v) {
    comboM6_->addItem(portAddressOptionText(v), v);
  }
  formMc->addRow(
      makeMLabel(QStringLiteral("M6 (Порт B):"),
                 QStringLiteral("Адрес порта B АЛУ (00..37 oct)")),
      comboM6_);

  // M7
  comboM7_ = new SmoothComboBox(groupMicrocode_);
  populateM7Combo(false, 0);
  formMc->addRow(makeMLabel(QStringLiteral("M7 (Сдвиг):"),
                            QStringLiteral("Цепь сдвига регистров АЛУ")),
                 comboM7_);

  // M8
  comboM8_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m8Options()) {
    comboM8_->addItem(
        QStringLiteral("[%1] C0 = %2").arg(opt.octalCode, opt.shortText),
        opt.code);
  }
  formMc->addRow(makeMLabel(QStringLiteral("M8 (Пер. C0):"),
                            QStringLiteral("Входной перенос C0")),
                 comboM8_);

  // M9
  comboM9_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m9Options()) {
    comboM9_->addItem(
        QStringLiteral("[%1] %2").arg(opt.octalCode, opt.shortText),
        opt.code);
  }
  formMc->addRow(makeMLabel(QStringLiteral("M9 (Опер.):"),
                            QStringLiteral("Источники операндов R и S")),
                 comboM9_);

  // M10
  comboM10_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m10Options()) {
    comboM10_->addItem(
        QStringLiteral("[%1] F = %2").arg(opt.octalCode, opt.shortText),
        opt.code);
  }
  formMc->addRow(makeMLabel(QStringLiteral("M10 (АЛУ):"),
                            QStringLiteral("Операция АЛУ")),
                 comboM10_);

  // M11
  comboM11_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m11Options()) {
    comboM11_->addItem(QStringLiteral("[%1] РОН:%2(%3) | RQ:%4(%5) | Y:%6")
                           .arg(opt.octalCode, opt.writeRon, opt.shiftRon,
                                opt.writeRq, opt.shiftRq, opt.outputY),
                       opt.code);
  }
  formMc->addRow(makeMLabel(QStringLiteral("M11 (Приём.):"),
                            QStringLiteral("Приёмник результата АЛУ")),
                 comboM11_);

  // M12
  comboM12_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m12Options()) {
    comboM12_->addItem(
        QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description),
        opt.code);
  }
  formMc->addRow(makeMLabel(QStringLiteral("M12 (Шина D):"),
                            QStringLiteral("Коммутация шины D")),
                 comboM12_);

  // M13
  comboM13_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m13Options()) {
    comboM13_->addItem(
        QStringLiteral("[%1] %2").arg(opt.octalCode, opt.shortText),
        opt.code);
  }
  formMc->addRow(
      makeMLabel(QStringLiteral("M13 (PSW):"),
                 QStringLiteral("Загрузка регистра состояния PSW")),
      comboM13_);

  // M14
  comboM14_ = new SmoothComboBox(groupMicrocode_);
  for (const auto &opt : K1804Microcode::m14Options()) {
    comboM14_->addItem(
        QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description),
        opt.code);
  }
  formMc->addRow(makeMLabel(QStringLiteral("M14 (Вых.):"),
                            QStringLiteral("Выходная шина")),
                 comboM14_);

  // M15
  editM15Oct_ = new QLineEdit(groupMicrocode_);
  editM15Oct_->setValidator(octValidator8);
  editM15Oct_->setToolTip(
      QStringLiteral("M15: Маска константы (0..377 oct)"));
  formMc->addRow(
      makeMLabel(QStringLiteral("M15 (Маска):"), editM15Oct_->toolTip()),
      editM15Oct_);

  mcVBox->addLayout(formMc);

  btnOpenDialog_ =
      new QPushButton(QStringLiteral("Открыть в окне..."), groupMicrocode_);
  btnOpenDialog_->setObjectName(QStringLiteral("openDialogBtn"));
  btnOpenDialog_->setCursor(Qt::PointingHandCursor);
  mcVBox->addWidget(btnOpenDialog_);

  vbox->addWidget(groupMicrocode_);
  vbox->addStretch(1);

  scrollArea->setWidget(container);
  ui->mainLayout->addWidget(scrollArea);

  // Подключаем сигналы редактирования
  connect(editStepName_, &QLineEdit::editingFinished, this,
          &ElementPropertyFrame::onBlockHeaderEdited);
  connect(editAddressOct_, &QLineEdit::editingFinished, this,
          &ElementPropertyFrame::onBlockHeaderEdited);

  connect(editConnName_, &QLineEdit::textEdited, this,
          &ElementPropertyFrame::onConnectorNameEdited);
  connect(editMaskText_, &QLineEdit::textEdited, this,
          [this](const QString &text) {
            if (auto *mask = dynamic_cast<MaskElement *>(currentElement_)) {
              if (!isUpdating_) {
                mask->setMaskText(text);
              }
            }
          });

  connect(editM1Oct_, &QLineEdit::editingFinished, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(editM15Oct_, &QLineEdit::editingFinished, this,
          &ElementPropertyFrame::onBlockMFieldEdited);

  connect(comboM2_, &QComboBox::currentIndexChanged, this, [this](int idx) {
    if (isUpdating_)
      return;
    populateM3Combo(idx == 1, comboM3_->currentIndex());
    onBlockMFieldEdited();
  });
  connect(comboM3_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM4_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM5_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM6_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM7_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM8_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM9_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM10_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM11_, &QComboBox::currentIndexChanged, this, [this](int idx) {
    if (isUpdating_)
      return;
    bool altM7 = (idx == 1 || idx == 3 || idx == 5 || idx == 7);
    populateM7Combo(altM7, comboM7_->currentIndex());
    onBlockMFieldEdited();
  });
  connect(comboM12_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM13_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);
  connect(comboM14_, &QComboBox::currentIndexChanged, this,
          &ElementPropertyFrame::onBlockMFieldEdited);

  connect(btnOpenDialog_, &QPushButton::clicked, this, [this]() {
    if (!scene_)
      return;
    if (auto *block = dynamic_cast<MasterBlockElement *>(currentElement_)) {
      scene_->openBlockParamsDialog(block);
    }
  });
}

void ElementPropertyFrame::populateM3Combo(bool altMode, int keepIndex) {
  bool prev = isUpdating_;
  isUpdating_ = true;
  comboM3_->clear();
  for (const auto &opt : K1804Microcode::m3Options(altMode)) {
    comboM3_->addItem(
        QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description),
        opt.code);
  }
  comboM3_->setCurrentIndex(qBound(0, keepIndex, comboM3_->count() - 1));
  isUpdating_ = prev;
}

void ElementPropertyFrame::populateM7Combo(bool altMode, int keepIndex) {
  bool prev = isUpdating_;
  isUpdating_ = true;
  comboM7_->clear();
  for (const auto &opt : K1804Microcode::m7Options(altMode)) {
    comboM7_->addItem(
        QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description),
        opt.code);
  }
  comboM7_->setCurrentIndex(qBound(0, keepIndex, comboM7_->count() - 1));
  isUpdating_ = prev;
}

void ElementPropertyFrame::updateProperties() {
  ShapeElement *sel = scene_ ? scene_->selectedElement() : nullptr;

  if (currentElement_ != sel) {
    if (currentElement_) {
      disconnect(currentElement_, nullptr, this, nullptr);
    }
    currentElement_ = sel;
    if (currentElement_) {
      connect(currentElement_, &ShapeElement::propertyChanged, this,
              &ElementPropertyFrame::updateProperties);
    }
  }

  if (!currentElement_) {
    lblNoSelection_->show();
    groupBlockIdentity_->hide();
    groupConnIdentity_->hide();
    groupMicrocode_->hide();
    return;
  }

  isUpdating_ = true;
  lblNoSelection_->hide();

  if (auto *block = dynamic_cast<MasterBlockElement *>(currentElement_)) {
    groupBlockIdentity_->show();
    groupMicrocode_->show();
    groupConnIdentity_->hide();

    editStepName_->setText(block->stepName());
    editAddressOct_->setText(K1804Microcode::toOctal(block->address()));
    lblAddressDec_->setText(QStringLiteral("(= %1 dec)").arg(block->address()));

    editM1Oct_->setText(K1804Microcode::toOctal(block->mField(1)));
    int m2 =
        qBound(0, static_cast<int>(block->mField(2)), comboM2_->count() - 1);
    comboM2_->setCurrentIndex(m2);
    populateM3Combo(m2 == 1, static_cast<int>(block->mField(3)));
    comboM4_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(4)), comboM4_->count() - 1));
    comboM5_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(5)), comboM5_->count() - 1));
    comboM6_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(6)), comboM6_->count() - 1));

    int m11 =
        qBound(0, static_cast<int>(block->mField(11)), comboM11_->count() - 1);
    comboM11_->setCurrentIndex(m11);
    bool altM7 = (m11 == 1 || m11 == 3 || m11 == 5 || m11 == 7);
    populateM7Combo(altM7, static_cast<int>(block->mField(7)));

    comboM8_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(8)), comboM8_->count() - 1));
    comboM9_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(9)), comboM9_->count() - 1));
    comboM10_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(10)), comboM10_->count() - 1));
    comboM12_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(12)), comboM12_->count() - 1));
    comboM13_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(13)), comboM13_->count() - 1));
    comboM14_->setCurrentIndex(
        qBound(0, static_cast<int>(block->mField(14)), comboM14_->count() - 1));
    editM15Oct_->setText(K1804Microcode::toOctal(block->mField(15)));
  } else if (auto *conn = dynamic_cast<ConnectorElement *>(currentElement_)) {
    groupBlockIdentity_->hide();
    groupMicrocode_->hide();
    groupConnIdentity_->setTitle(QStringLiteral("Коннектор"));
    groupConnIdentity_->show();
    lblMaskText_->hide();
    editMaskText_->hide();

    editConnName_->setText(conn->name());
  } else if (auto *mask = dynamic_cast<MaskElement *>(currentElement_)) {
    groupBlockIdentity_->hide();
    groupMicrocode_->hide();
    groupConnIdentity_->setTitle(QStringLiteral("Маска (Mask)"));
    groupConnIdentity_->show();
    lblMaskText_->show();
    editMaskText_->show();

    editConnName_->setText(mask->name());
    editMaskText_->setText(mask->maskText());
  }

  isUpdating_ = false;
}

void ElementPropertyFrame::applyMicrocodeFromInspector() {
  auto *block = dynamic_cast<MasterBlockElement *>(currentElement_);
  if (!block || isUpdating_)
    return;

  ModifyMasterBlockParamsCommand::BlockState oldState;
  oldState.stepName = block->stepName();
  oldState.address = block->address();
  for (int i = 1; i <= 15; ++i) {
    oldState.mFields[i] = block->mField(i);
  }
  oldState.lines = block->allLines();

  ModifyMasterBlockParamsCommand::BlockState newState = oldState;
  QString step = editStepName_->text().trimmed();
  if (!step.isEmpty()) {
    newState.stepName = step;
  }

  quint16 addr = K1804Microcode::fromOctal(editAddressOct_->text());
  if (addr >= 1 && addr <= 64) {
    newState.address = addr;
  }
  lblAddressDec_->setText(QStringLiteral("(= %1 dec)").arg(newState.address));

  newState.mFields[1] = K1804Microcode::fromOctal(editM1Oct_->text());
  newState.mFields[2] = static_cast<quint16>(qMax(0, comboM2_->currentIndex()));
  newState.mFields[3] = static_cast<quint16>(qMax(0, comboM3_->currentIndex()));
  newState.mFields[4] = static_cast<quint16>(qMax(0, comboM4_->currentIndex()));
  newState.mFields[5] = static_cast<quint16>(qMax(0, comboM5_->currentIndex()));
  newState.mFields[6] = static_cast<quint16>(qMax(0, comboM6_->currentIndex()));
  newState.mFields[7] = static_cast<quint16>(qMax(0, comboM7_->currentIndex()));
  newState.mFields[8] = static_cast<quint16>(qMax(0, comboM8_->currentIndex()));
  newState.mFields[9] = static_cast<quint16>(qMax(0, comboM9_->currentIndex()));
  newState.mFields[10] =
      static_cast<quint16>(qMax(0, comboM10_->currentIndex()));
  newState.mFields[11] =
      static_cast<quint16>(qMax(0, comboM11_->currentIndex()));
  newState.mFields[12] =
      static_cast<quint16>(qMax(0, comboM12_->currentIndex()));
  newState.mFields[13] =
      static_cast<quint16>(qMax(0, comboM13_->currentIndex()));
  newState.mFields[14] =
      static_cast<quint16>(qMax(0, comboM14_->currentIndex()));
  newState.mFields[15] = static_cast<quint16>(
      K1804Microcode::fromOctal(editM15Oct_->text()) & 0xFF);

  newState.lines = K1804Microcode::generateBlockLines(newState.mFields);

  bool changed = (oldState.stepName != newState.stepName) ||
                 (oldState.address != newState.address) ||
                 (oldState.mFields != newState.mFields) ||
                 (oldState.lines != newState.lines);

  if (changed) {
    if (scene_ && scene_->undoStack()) {
      scene_->undoStack()->push(new ModifyMasterBlockParamsCommand(
          scene_, block, oldState, newState));
    } else {
      block->setStepName(newState.stepName);
      block->setAddress(newState.address);
      for (int i = 1; i <= 15; ++i) {
        block->setMField(i, newState.mFields[i]);
      }
      block->setAllLines(newState.lines);
    }
  }
}

void ElementPropertyFrame::onBlockHeaderEdited() {
  auto *block = dynamic_cast<MasterBlockElement *>(currentElement_);
  if (!block || isUpdating_)
    return;

  ModifyMasterBlockParamsCommand::BlockState oldState;
  oldState.stepName = block->stepName();
  oldState.address = block->address();
  for (int i = 1; i <= 15; ++i) {
    oldState.mFields[i] = block->mField(i);
  }
  oldState.lines = block->allLines();

  ModifyMasterBlockParamsCommand::BlockState newState = oldState;
  QString step = editStepName_->text().trimmed();
  if (!step.isEmpty()) {
    newState.stepName = step;
  }

  quint16 addr = K1804Microcode::fromOctal(editAddressOct_->text());
  if (addr >= 1 && addr <= 64) {
    newState.address = addr;
  }
  lblAddressDec_->setText(QStringLiteral("(= %1 dec)").arg(newState.address));

  if (oldState.stepName != newState.stepName ||
      oldState.address != newState.address) {
    if (scene_ && scene_->undoStack()) {
      scene_->undoStack()->push(new ModifyMasterBlockParamsCommand(
          scene_, block, oldState, newState));
    } else {
      block->setStepName(newState.stepName);
      block->setAddress(newState.address);
    }
  }
}

void ElementPropertyFrame::onBlockMFieldEdited() {
  applyMicrocodeFromInspector();
}

void ElementPropertyFrame::onConnectorNameEdited(const QString &text) {
  if (currentElement_ && !isUpdating_) {
    currentElement_->setName(text);
  }
}

} // namespace editor
