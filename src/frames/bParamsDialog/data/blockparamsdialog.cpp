#include "blockparamsdialog.h"
#include "src/widgets/smoothcombobox.h"

#include "src/canvas/canvasscene.h"
#include "src/canvas/undocommands.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

namespace editor {

namespace {

QLabel *createMBadge(const QString &code, QWidget *parent) {
    auto *lbl = new QLabel(code, parent);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setFixedSize(36, 22);
    lbl->setStyleSheet(QStringLiteral(
        "background-color: #eff6ff; color: #1d4ed8; border: 1px solid #bfdbfe; "
        "border-radius: 4px; font-family: 'Consolas', monospace; font-size: 11px; font-weight: 700;"));
    return lbl;
}

QLabel *createFieldTitle(const QString &title, QWidget *parent) {
    auto *lbl = new QLabel(title, parent);
    lbl->setStyleSheet(QStringLiteral("color: #334155; font-size: 11px; font-weight: 500;"));
    return lbl;
}

} // namespace

BlockParamsDialog::BlockParamsDialog(MasterBlockElement *block,
                                     CanvasScene *scene, QWidget *parent)
    : QDialog(parent), block_(block), scene_(scene)
{
    setWindowTitle(QStringLiteral("Редактор микрокоманды — %1")
                       .arg(block ? block->name() : QStringLiteral("MasterBlock")));
    setMinimumSize(640, 480);
    resize(690, 560);

    setupUi();
    loadFromBlock();
}

void BlockParamsDialog::setupUi() {
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #f8fafc; color: #1e293b; }"
        "QFrame#cardFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }"
        "QLabel { font-size: 11px; color: #334155; }"
        "QLineEdit { background-color: #ffffff; border: 1px solid #cbd5e1; border-radius: 5px; "
        "            padding: 3px 8px; font-size: 11px; color: #0f172a; min-height: 20px; }"
        "QLineEdit:focus { border: 1.5px solid #2563eb; }"
        "QComboBox { combobox-popup: 0; background-color: #ffffff; border: 1px solid #cbd5e1; "
        "            border-radius: 5px; padding: 3px 8px; font-size: 11px; color: #0f172a; min-height: 20px; }"
        "QComboBox:hover { border-color: #94a3b8; }"
        "QComboBox:focus { border: 1.5px solid #2563eb; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 20px; border: none; }"
        "QComboBox QAbstractItemView { background-color: #ffffff; border: 1px solid #94a3b8; "
        "                              selection-background-color: #eff6ff; selection-color: #1d4ed8; "
        "                              color: #0f172a; font-size: 11px; padding: 2px; outline: 0px; }"
        "QComboBox QAbstractItemView::item { min-height: 24px; padding: 2px 6px; }"
        "QToolButton#bitBtn { font-family: 'Consolas', monospace; font-size: 11px; font-weight: 700; "
        "                     padding: 0px; margin: 0px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "                     background-color: #f8fafc; color: #334155; "
        "                     min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; }"
        "QToolButton#bitBtn:hover { border-color: #64748b; background-color: #f1f5f9; }"
        "QToolButton#bitBtn:checked { background-color: #2563eb; color: #ffffff; border-color: #1d4ed8; }"
        "QPushButton { border: 1px solid #cbd5e1; border-radius: 5px; padding: 5px 14px; font-size: 11px; "
        "              font-weight: 600; background-color: #ffffff; color: #1e293b; }"
        "QPushButton:hover { background-color: #f1f5f9; border-color: #94a3b8; }"
        "QPushButton#primaryBtn { background-color: #2563eb; color: #ffffff; border-color: #1d4ed8; }"
        "QPushButton#primaryBtn:hover { background-color: #1d4ed8; }"
        "QPushButton#primaryBtn:disabled { background-color: #94a3b8; border-color: #94a3b8; color: #e2e8f0; }"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    auto *octValidator16 = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^[0-7]{1,6}$")), this);
    auto *octValidator8 = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^[0-7]{1,3}$")), this);

    // =========================================================================
    // 1. КОМПАКТНАЯ ШАПКА: МЕТКА (STEP) И АДРЕС МК В ПЗУ
    // =========================================================================
    auto *headerCard = new QFrame(this);
    headerCard->setObjectName(QStringLiteral("cardFrame"));
    auto *headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(10, 7, 10, 7);
    headerLayout->setSpacing(8);

    auto *lblStep = new QLabel(QStringLiteral("Метка (лев.):"), headerCard);
    lblStep->setStyleSheet(QStringLiteral("font-weight: 600;"));
    editStepName_ = new QLineEdit(headerCard);
    editStepName_->setFixedWidth(58);
    editStepName_->setAlignment(Qt::AlignCenter);

    auto *lblAddr = new QLabel(QStringLiteral("Адрес МК (oct):"), headerCard);
    lblAddr->setStyleSheet(QStringLiteral("font-weight: 600;"));
    editAddressOct_ = new QLineEdit(headerCard);
    editAddressOct_->setValidator(octValidator16);
    editAddressOct_->setFixedWidth(58);
    editAddressOct_->setAlignment(Qt::AlignCenter);

    lblAddressDec_ = new QLabel(QStringLiteral("(= 1 dec)"), headerCard);
    lblAddressDec_->setStyleSheet(QStringLiteral("color: #64748b; font-family: 'Consolas', monospace;"));

    lblValidationStatus_ = new QLabel(headerCard);
    lblValidationStatus_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(lblStep);
    headerLayout->addWidget(editStepName_);
    headerLayout->addSpacing(6);
    headerLayout->addWidget(lblAddr);
    headerLayout->addWidget(editAddressOct_);
    headerLayout->addWidget(lblAddressDec_);
    headerLayout->addStretch(1);
    headerLayout->addWidget(lblValidationStatus_);

    mainLayout->addWidget(headerCard);

    // =========================================================================
    // 2. СПИСОК ПОЛЕЙ M1 .. M15 В СТРОГОМ ПОРЯДКЕ
    // =========================================================================
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    auto *fieldsCard = new QFrame(scrollArea);
    fieldsCard->setObjectName(QStringLiteral("cardFrame"));
    auto *grid = new QGridLayout(fieldsCard);
    grid->setContentsMargins(10, 10, 10, 10);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);
    grid->setColumnStretch(2, 1);

    int row = 0;

    // --- M1: Константа / Адрес перехода (16 бит, oct) ---
    {
        auto *w = new QWidget(fieldsCard);
        auto *hl = new QHBoxLayout(w);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(6);
        editM1Oct_ = new QLineEdit(w);
        editM1Oct_->setValidator(octValidator16);
        editM1Oct_->setFixedWidth(90);
        lblM1Dec_ = new QLabel(QStringLiteral("(= 1 dec, 0..177777 oct)"), w);
        lblM1Dec_->setStyleSheet(QStringLiteral("color: #64748b; font-family: 'Consolas', monospace;"));
        hl->addWidget(editM1Oct_);
        hl->addWidget(lblM1Dec_);
        hl->addStretch(1);

        grid->addWidget(createMBadge(QStringLiteral("M1"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Константа / Адрес (oct):"), fieldsCard), row, 1);
        grid->addWidget(w, row, 2);
        row++;
    }

    // --- M2: Модификация условия ---
    {
        comboM2_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m2Options()) {
            comboM2_->addItem(QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description), opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M2"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Модификация условия:"), fieldsCard), row, 1);
        grid->addWidget(comboM2_, row, 2);
        row++;
    }

    // --- M3: Проверяемое условие TST ---
    {
        comboM3_ = new SmoothComboBox(fieldsCard);
        populateM3Combo(false, 0);
        grid->addWidget(createMBadge(QStringLiteral("M3"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Проверяемое условие TST:"), fieldsCard), row, 1);
        grid->addWidget(comboM3_, row, 2);
        row++;
    }

    // --- M4: Команда перехода БМУ ---
    {
        comboM4_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m4Options()) {
            comboM4_->addItem(QStringLiteral("[%1] %2 — %3").arg(opt.octalCode, opt.shortText, opt.description), opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M4"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Команда перехода БМУ:"), fieldsCard), row, 1);
        grid->addWidget(comboM4_, row, 2);
        row++;
    }

    // --- M5: Адрес порта A (АЛУ) ---
    {
        auto *w = new QWidget(fieldsCard);
        auto *hl = new QHBoxLayout(w);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(4);

        comboM5Mode_ = new SmoothComboBox(w);
        comboM5Mode_->addItem(QStringLiteral("РОН R0..R15"), 0);
        comboM5Mode_->addItem(QStringLiteral("RK[2/0]"), 1);
        comboM5Mode_->addItem(QStringLiteral("RK[8/6]"), 2);
        comboM5Mode_->setFixedWidth(105);

        comboM5Reg_ = new SmoothComboBox(w);
        for (int r = 0; r < 16; ++r) {
            comboM5Reg_->addItem(QStringLiteral("R%1 (%2)").arg(r).arg(K1804Microcode::toOctal(r, 2)), r);
        }
        comboM5Reg_->setFixedWidth(84);

        hl->addWidget(comboM5Mode_);
        hl->addWidget(comboM5Reg_);
        hl->addSpacing(4);

        for (int b = 0; b < 5; ++b) {
            auto *btn = new QToolButton(w);
            btn->setObjectName(QStringLiteral("bitBtn"));
            btn->setCheckable(true);
            btn->setText(QStringLiteral("0"));
            btn->setToolTip(QStringLiteral("Разряд 2^%1 поля M5").arg(4 - b));
            btnM5Bits_[b] = btn;
            hl->addWidget(btn);
            connect(btn, &QToolButton::clicked, this, &BlockParamsDialog::onM5BitToggled);
        }

        lblM5Oct_ = new QLabel(QStringLiteral("00 oct"), w);
        lblM5Oct_->setStyleSheet(QStringLiteral("font-family: 'Consolas', monospace; font-weight: 700; color: #1d4ed8;"));
        hl->addSpacing(4);
        hl->addWidget(lblM5Oct_);
        hl->addStretch(1);

        grid->addWidget(createMBadge(QStringLiteral("M5"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Адрес порта A (АЛУ):"), fieldsCard), row, 1);
        grid->addWidget(w, row, 2);
        row++;
    }

    // --- M6: Адрес порта B (АЛУ) ---
    {
        auto *w = new QWidget(fieldsCard);
        auto *hl = new QHBoxLayout(w);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(4);

        comboM6Mode_ = new SmoothComboBox(w);
        comboM6Mode_->addItem(QStringLiteral("РОН R0..R15"), 0);
        comboM6Mode_->addItem(QStringLiteral("RK[2/0]"), 1);
        comboM6Mode_->addItem(QStringLiteral("RK[8/6]"), 2);
        comboM6Mode_->setFixedWidth(105);

        comboM6Reg_ = new SmoothComboBox(w);
        for (int r = 0; r < 16; ++r) {
            comboM6Reg_->addItem(QStringLiteral("R%1 (%2)").arg(r).arg(K1804Microcode::toOctal(r, 2)), r);
        }
        comboM6Reg_->setFixedWidth(84);

        hl->addWidget(comboM6Mode_);
        hl->addWidget(comboM6Reg_);
        hl->addSpacing(4);

        for (int b = 0; b < 5; ++b) {
            auto *btn = new QToolButton(w);
            btn->setObjectName(QStringLiteral("bitBtn"));
            btn->setCheckable(true);
            btn->setText(QStringLiteral("0"));
            btn->setToolTip(QStringLiteral("Разряд 2^%1 поля M6").arg(4 - b));
            btnM6Bits_[b] = btn;
            hl->addWidget(btn);
            connect(btn, &QToolButton::clicked, this, &BlockParamsDialog::onM6BitToggled);
        }

        lblM6Oct_ = new QLabel(QStringLiteral("00 oct"), w);
        lblM6Oct_->setStyleSheet(QStringLiteral("font-family: 'Consolas', monospace; font-weight: 700; color: #1d4ed8;"));
        hl->addSpacing(4);
        hl->addWidget(lblM6Oct_);
        hl->addStretch(1);

        grid->addWidget(createMBadge(QStringLiteral("M6"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Адрес порта B (АЛУ):"), fieldsCard), row, 1);
        grid->addWidget(w, row, 2);
        row++;
    }

    // --- M7: Цепь сдвига регистров ---
    {
        comboM7_ = new SmoothComboBox(fieldsCard);
        populateM7Combo(false, 0);
        grid->addWidget(createMBadge(QStringLiteral("M7"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Цепь сдвига регистров:"), fieldsCard), row, 1);
        grid->addWidget(comboM7_, row, 2);
        row++;
    }

    // --- M8: Входной перенос C0 ---
    {
        comboM8_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m8Options()) {
            comboM8_->addItem(QStringLiteral("[%1] C0 = %2").arg(opt.octalCode, opt.shortText), opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M8"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Входной перенос C0:"), fieldsCard), row, 1);
        grid->addWidget(comboM8_, row, 2);
        row++;
    }

    // --- M9: Источники операндов R и S ---
    {
        comboM9_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m9Options()) {
            comboM9_->addItem(QStringLiteral("[%1] R :: S = %2").arg(opt.octalCode, opt.shortText), opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M9"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Источники операндов R, S:"), fieldsCard), row, 1);
        grid->addWidget(comboM9_, row, 2);
        row++;
    }

    // --- M10: Операция АЛУ ---
    {
        comboM10_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m10Options()) {
            comboM10_->addItem(QStringLiteral("[%1] F = %2").arg(opt.octalCode, opt.shortText), opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M10"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Операция АЛУ:"), fieldsCard), row, 1);
        grid->addWidget(comboM10_, row, 2);
        row++;
    }

    // --- M11: Приёмник результата АЛУ ---
    {
        comboM11_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m11Options()) {
            comboM11_->addItem(
                QStringLiteral("[%1] РОН: %2 (%3) | RQ: %4 (%5) | Вых Y: %6")
                    .arg(opt.octalCode, opt.writeRon, opt.shiftRon, opt.writeRq, opt.shiftRq, opt.outputY),
                opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M11"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Приёмник результата АЛУ:"), fieldsCard), row, 1);
        grid->addWidget(comboM11_, row, 2);
        row++;
    }

    // --- M12: Шина данных D ---
    {
        comboM12_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m12Options()) {
            comboM12_->addItem(QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description), opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M12"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Шина данных D:"), fieldsCard), row, 1);
        grid->addWidget(comboM12_, row, 2);
        row++;
    }

    // --- M13: Загрузка регистра состояния PSW ---
    {
        comboM13_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m13Options()) {
            comboM13_->addItem(QStringLiteral("[%1] %2 (%3)").arg(opt.octalCode, opt.description, opt.shortText), opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M13"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Регистр состояния PSW:"), fieldsCard), row, 1);
        grid->addWidget(comboM13_, row, 2);
        row++;
    }

    // --- M14: Выходная шина ---
    {
        comboM14_ = new SmoothComboBox(fieldsCard);
        for (const auto &opt : K1804Microcode::m14Options()) {
            comboM14_->addItem(QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description), opt.code);
        }
        grid->addWidget(createMBadge(QStringLiteral("M14"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Выходная шина:"), fieldsCard), row, 1);
        grid->addWidget(comboM14_, row, 2);
        row++;
    }

    // --- M15: Маска константы (8 бит, oct) ---
    {
        auto *w = new QWidget(fieldsCard);
        auto *hl = new QHBoxLayout(w);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(6);
        editM15Oct_ = new QLineEdit(w);
        editM15Oct_->setValidator(octValidator8);
        editM15Oct_->setFixedWidth(90);
        lblM15Dec_ = new QLabel(QStringLiteral("(= 0 dec, 0..377 oct)"), w);
        lblM15Dec_->setStyleSheet(QStringLiteral("color: #64748b; font-family: 'Consolas', monospace;"));
        hl->addWidget(editM15Oct_);
        hl->addWidget(lblM15Dec_);
        hl->addStretch(1);

        grid->addWidget(createMBadge(QStringLiteral("M15"), fieldsCard), row, 0);
        grid->addWidget(createFieldTitle(QStringLiteral("Маска константы (oct):"), fieldsCard), row, 1);
        grid->addWidget(w, row, 2);
        row++;
    }

    scrollArea->setWidget(fieldsCard);
    mainLayout->addWidget(scrollArea, 1);

    // =========================================================================
    // 3. КНОПКИ УПРАВЛЕНИЯ
    // =========================================================================
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 2, 0, 0);
    btnReset_ = new QPushButton(QStringLiteral("Сбросить"), this);
    btnCancel_ = new QPushButton(QStringLiteral("Отмена"), this);
    btnOk_ = new QPushButton(QStringLiteral("Применить"), this);
    btnOk_->setObjectName(QStringLiteral("primaryBtn"));
    btnOk_->setDefault(true);

    btnLayout->addWidget(btnReset_);
    btnLayout->addStretch(1);
    btnLayout->addWidget(btnCancel_);
    btnLayout->addWidget(btnOk_);
    mainLayout->addLayout(btnLayout);

    // Сигналы
    connect(editStepName_, &QLineEdit::textEdited, this, &BlockParamsDialog::onHeaderEdited);
    connect(editAddressOct_, &QLineEdit::textEdited, this, &BlockParamsDialog::onHeaderEdited);
    connect(editM1Oct_, &QLineEdit::textEdited, this, &BlockParamsDialog::onHeaderEdited);
    connect(editM15Oct_, &QLineEdit::textEdited, this, &BlockParamsDialog::onHeaderEdited);

    connect(comboM5Mode_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onM5ModeOrRegChanged);
    connect(comboM5Reg_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onM5ModeOrRegChanged);
    connect(comboM6Mode_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onM6ModeOrRegChanged);
    connect(comboM6Reg_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onM6ModeOrRegChanged);

    connect(comboM11_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onM11Changed);
    connect(comboM2_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onM2Changed);
    connect(comboM13_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onM13Changed);

    connect(comboM12_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onAnyComboChanged);
    connect(comboM14_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onAnyComboChanged);
    connect(comboM9_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onAnyComboChanged);
    connect(comboM10_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onAnyComboChanged);
    connect(comboM7_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onAnyComboChanged);
    connect(comboM8_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onAnyComboChanged);
    connect(comboM4_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onAnyComboChanged);
    connect(comboM3_, &QComboBox::currentIndexChanged, this, &BlockParamsDialog::onAnyComboChanged);

    connect(btnReset_, &QPushButton::clicked, this, &BlockParamsDialog::onResetDefaults);
    connect(btnCancel_, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk_, &QPushButton::clicked, this, &BlockParamsDialog::onAcceptClicked);
}

void BlockParamsDialog::populateM7Combo(bool altMode, int keepIndex) {
    bool prev = updatingUi_;
    updatingUi_ = true;
    comboM7_->clear();
    for (const auto &opt : K1804Microcode::m7Options(altMode)) {
        comboM7_->addItem(QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description), opt.code);
    }
    comboM7_->setCurrentIndex(qBound(0, keepIndex, comboM7_->count() - 1));
    updatingUi_ = prev;
}

void BlockParamsDialog::populateM3Combo(bool altMode, int keepIndex) {
    bool prev = updatingUi_;
    updatingUi_ = true;
    comboM3_->clear();
    for (const auto &opt : K1804Microcode::m3Options(altMode)) {
        comboM3_->addItem(QStringLiteral("[%1] %2").arg(opt.octalCode, opt.description), opt.code);
    }
    comboM3_->setCurrentIndex(qBound(0, keepIndex, comboM3_->count() - 1));
    updatingUi_ = prev;
}

void BlockParamsDialog::loadFromBlock() {
    if (!block_) return;
    updatingUi_ = true;

    currentStepName_ = block_->stepName();
    currentAddress_ = block_->address();
    for (int i = 1; i <= 15; ++i) {
        currentMFields_[i] = block_->mField(i);
    }

    editStepName_->setText(currentStepName_);
    editAddressOct_->setText(K1804Microcode::toOctal(currentAddress_));
    editM1Oct_->setText(K1804Microcode::toOctal(currentMFields_[1]));
    editM15Oct_->setText(K1804Microcode::toOctal(currentMFields_[15]));

    comboM12_->setCurrentIndex(qBound(0, static_cast<int>(currentMFields_[12]), comboM12_->count() - 1));
    comboM14_->setCurrentIndex(qBound(0, static_cast<int>(currentMFields_[14]), comboM14_->count() - 1));

    syncM5WidgetsFromValue(currentMFields_[5]);
    syncM6WidgetsFromValue(currentMFields_[6]);

    comboM9_->setCurrentIndex(qBound(0, static_cast<int>(currentMFields_[9]), comboM9_->count() - 1));
    comboM10_->setCurrentIndex(qBound(0, static_cast<int>(currentMFields_[10]), comboM10_->count() - 1));

    int m11 = qBound(0, static_cast<int>(currentMFields_[11]), comboM11_->count() - 1);
    comboM11_->setCurrentIndex(m11);
    bool altM7 = (m11 == 1 || m11 == 3 || m11 == 5 || m11 == 7);
    populateM7Combo(altM7, static_cast<int>(currentMFields_[7]));

    comboM8_->setCurrentIndex(qBound(0, static_cast<int>(currentMFields_[8]), comboM8_->count() - 1));
    comboM4_->setCurrentIndex(qBound(0, static_cast<int>(currentMFields_[4]), comboM4_->count() - 1));

    int m2 = qBound(0, static_cast<int>(currentMFields_[2]), comboM2_->count() - 1);
    comboM2_->setCurrentIndex(m2);
    bool altM3 = (m2 == 1);
    populateM3Combo(altM3, static_cast<int>(currentMFields_[3]));

    int m13 = qBound(0, static_cast<int>(currentMFields_[13]), comboM13_->count() - 1);
    comboM13_->setCurrentIndex(m13);
    comboM13_->setToolTip(K1804Microcode::m13HelpText(m13));

    updatingUi_ = false;
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::syncM5WidgetsFromValue(quint16 m5Val) {
    bool prev = updatingUi_;
    updatingUi_ = true;

    quint16 v = m5Val & 0x1F;
    for (int b = 0; b < 5; ++b) {
        int bitPos = 4 - b;
        bool on = ((v >> bitPos) & 1) != 0;
        btnM5Bits_[b]->setChecked(on);
        btnM5Bits_[b]->setText(on ? QStringLiteral("1") : QStringLiteral("0"));
    }

    if (v <= 15) {
        comboM5Mode_->setCurrentIndex(0);
        comboM5Reg_->setEnabled(true);
        comboM5Reg_->setCurrentIndex(v);
    } else if (v <= 23) {
        comboM5Mode_->setCurrentIndex(1);
        comboM5Reg_->setEnabled(false);
    } else {
        comboM5Mode_->setCurrentIndex(2);
        comboM5Reg_->setEnabled(false);
    }

    lblM5Oct_->setText(QStringLiteral("%1 oct").arg(K1804Microcode::toOctal(v, 2)));
    updatingUi_ = prev;
}

void BlockParamsDialog::syncM6WidgetsFromValue(quint16 m6Val) {
    bool prev = updatingUi_;
    updatingUi_ = true;

    quint16 v = m6Val & 0x1F;
    for (int b = 0; b < 5; ++b) {
        int bitPos = 4 - b;
        bool on = ((v >> bitPos) & 1) != 0;
        btnM6Bits_[b]->setChecked(on);
        btnM6Bits_[b]->setText(on ? QStringLiteral("1") : QStringLiteral("0"));
    }

    if (v <= 15) {
        comboM6Mode_->setCurrentIndex(0);
        comboM6Reg_->setEnabled(true);
        comboM6Reg_->setCurrentIndex(v);
    } else if (v <= 23) {
        comboM6Mode_->setCurrentIndex(1);
        comboM6Reg_->setEnabled(false);
    } else {
        comboM6Mode_->setCurrentIndex(2);
        comboM6Reg_->setEnabled(false);
    }

    lblM6Oct_->setText(QStringLiteral("%1 oct").arg(K1804Microcode::toOctal(v, 2)));
    updatingUi_ = prev;
}

void BlockParamsDialog::onM5ModeOrRegChanged() {
    if (updatingUi_) return;
    int mode = comboM5Mode_->currentIndex();
    quint16 val = 0;
    if (mode == 0) {
        val = static_cast<quint16>(qBound(0, comboM5Reg_->currentIndex(), 15));
    } else if (mode == 1) {
        val = 16;
    } else {
        val = 24;
    }
    syncM5WidgetsFromValue(val);
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onM5BitToggled() {
    if (updatingUi_) return;
    quint16 val = 0;
    for (int b = 0; b < 5; ++b) {
        if (btnM5Bits_[b]->isChecked()) {
            val |= (1u << (4 - b));
        }
    }
    syncM5WidgetsFromValue(val);
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onM6ModeOrRegChanged() {
    if (updatingUi_) return;
    int mode = comboM6Mode_->currentIndex();
    quint16 val = 0;
    if (mode == 0) {
        val = static_cast<quint16>(qBound(0, comboM6Reg_->currentIndex(), 15));
    } else if (mode == 1) {
        val = 16;
    } else {
        val = 24;
    }
    syncM6WidgetsFromValue(val);
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onM6BitToggled() {
    if (updatingUi_) return;
    quint16 val = 0;
    for (int b = 0; b < 5; ++b) {
        if (btnM6Bits_[b]->isChecked()) {
            val |= (1u << (4 - b));
        }
    }
    syncM6WidgetsFromValue(val);
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onM11Changed(int index) {
    if (updatingUi_) return;
    int keepM7 = comboM7_->currentIndex();
    bool altM7 = (index == 1 || index == 3 || index == 5 || index == 7);
    populateM7Combo(altM7, keepM7);
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onM2Changed(int index) {
    if (updatingUi_) return;
    int keepM3 = comboM3_->currentIndex();
    bool altM3 = (index == 1);
    populateM3Combo(altM3, keepM3);
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onM13Changed(int index) {
    if (updatingUi_) return;
    comboM13_->setToolTip(K1804Microcode::m13HelpText(index));
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onHeaderEdited() {
    if (updatingUi_) return;
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onAnyComboChanged() {
    if (updatingUi_) return;
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::rebuildCurrentState() {
    currentStepName_ = editStepName_->text().trimmed();
    if (currentStepName_.isEmpty()) {
        currentStepName_ = QStringLiteral("1");
    }

    currentAddress_ = K1804Microcode::fromOctal(editAddressOct_->text());
    currentMFields_[1] = K1804Microcode::fromOctal(editM1Oct_->text());
    currentMFields_[2] = static_cast<quint16>(qMax(0, comboM2_->currentIndex()));
    currentMFields_[3] = static_cast<quint16>(qMax(0, comboM3_->currentIndex()));
    currentMFields_[4] = static_cast<quint16>(qMax(0, comboM4_->currentIndex()));

    quint16 v5 = 0;
    for (int b = 0; b < 5; ++b) {
        if (btnM5Bits_[b]->isChecked()) v5 |= (1u << (4 - b));
    }
    currentMFields_[5] = v5;

    quint16 v6 = 0;
    for (int b = 0; b < 5; ++b) {
        if (btnM6Bits_[b]->isChecked()) v6 |= (1u << (4 - b));
    }
    currentMFields_[6] = v6;

    currentMFields_[7] = static_cast<quint16>(qMax(0, comboM7_->currentIndex()));
    currentMFields_[8] = static_cast<quint16>(qMax(0, comboM8_->currentIndex()));
    currentMFields_[9] = static_cast<quint16>(qMax(0, comboM9_->currentIndex()));
    currentMFields_[10] = static_cast<quint16>(qMax(0, comboM10_->currentIndex()));
    currentMFields_[11] = static_cast<quint16>(qMax(0, comboM11_->currentIndex()));
    currentMFields_[12] = static_cast<quint16>(qMax(0, comboM12_->currentIndex()));
    currentMFields_[13] = static_cast<quint16>(qMax(0, comboM13_->currentIndex()));
    currentMFields_[14] = static_cast<quint16>(qMax(0, comboM14_->currentIndex()));
    currentMFields_[15] = static_cast<quint16>(K1804Microcode::fromOctal(editM15Oct_->text()) & 0xFF);

    lblAddressDec_->setText(QStringLiteral("(= %1 dec)").arg(currentAddress_));
    lblM1Dec_->setText(QStringLiteral("(= %1 dec, 0..177777 oct)").arg(currentMFields_[1]));
    lblM15Dec_->setText(QStringLiteral("(= %1 dec, 0..377 oct)").arg(currentMFields_[15]));

    currentLines_ = K1804Microcode::generateBlockLines(currentMFields_);
}

bool BlockParamsDialog::validateCurrentState(QString *errorOut) const {
    if (editStepName_->text().trimmed().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Укажите метку блока");
        return false;
    }
    if (editAddressOct_->text().trimmed().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Укажите адрес МК (oct)");
        return false;
    }
    if (currentAddress_ < 1 || currentAddress_ > 64) {
        if (errorOut) {
            *errorOut = QStringLiteral("Адрес %1 oct вне диапазона 001..100 oct").arg(editAddressOct_->text().trimmed());
        }
        return false;
    }

    if (scene_) {
        for (ShapeElement *elem : scene_->allElements()) {
            auto *other = dynamic_cast<MasterBlockElement*>(elem);
            if (!other || other == block_) continue;
            if (other->stepName().trimmed() == editStepName_->text().trimmed()) {
                if (errorOut) {
                    *errorOut = QStringLiteral("Метка «%1» уже занята").arg(other->stepName());
                }
                return false;
            }
            if (other->address() == currentAddress_) {
                if (errorOut) {
                    *errorOut = QStringLiteral("Адрес %1 oct уже занят блоком «%2»")
                                    .arg(K1804Microcode::toOctal(currentAddress_), other->stepName());
                }
                return false;
            }
        }
    }
    return true;
}

void BlockParamsDialog::updateLivePreview() {
    QString err;
    bool ok = validateCurrentState(&err);
    if (ok) {
        lblValidationStatus_->setText(QStringLiteral("Адрес свободен"));
        lblValidationStatus_->setStyleSheet(QStringLiteral(
            "background-color: #f0fdf4; color: #166534; border: 1px solid #86efac; "
            "border-radius: 4px; padding: 2px 8px; font-weight: 600; font-size: 10px;"));
        btnOk_->setEnabled(true);
    } else {
        lblValidationStatus_->setText(err);
        lblValidationStatus_->setStyleSheet(QStringLiteral(
            "background-color: #fef2f2; color: #991b1b; border: 1px solid #fca5a5; "
            "border-radius: 4px; padding: 2px 8px; font-weight: 600; font-size: 10px;"));
        btnOk_->setEnabled(false);
    }
}

void BlockParamsDialog::onResetDefaults() {
    updatingUi_ = true;
    editM1Oct_->setText(QStringLiteral("1"));
    editM15Oct_->setText(QStringLiteral("3"));

    comboM12_->setCurrentIndex(3);
    comboM14_->setCurrentIndex(0);
    syncM5WidgetsFromValue(0);
    syncM6WidgetsFromValue(3);
    comboM9_->setCurrentIndex(3);
    comboM10_->setCurrentIndex(3);
    comboM11_->setCurrentIndex(1);
    populateM7Combo(true, 0);
    comboM8_->setCurrentIndex(0);
    comboM4_->setCurrentIndex(3);
    comboM2_->setCurrentIndex(0);
    populateM3Combo(false, 3);
    comboM13_->setCurrentIndex(3);
    comboM13_->setToolTip(K1804Microcode::m13HelpText(3));

    updatingUi_ = false;
    rebuildCurrentState();
    updateLivePreview();
}

void BlockParamsDialog::onAcceptClicked() {
    rebuildCurrentState();
    QString err;
    if (!validateCurrentState(&err)) {
        QMessageBox::warning(this, QStringLiteral("Проверка параметров"), err);
        return;
    }

    if (!block_) {
        accept();
        return;
    }

    ModifyMasterBlockParamsCommand::BlockState oldState;
    oldState.stepName = block_->stepName();
    oldState.address = block_->address();
    for (int i = 1; i <= 15; ++i) {
        oldState.mFields[i] = block_->mField(i);
    }
    oldState.lines = block_->allLines();

    ModifyMasterBlockParamsCommand::BlockState newState;
    newState.stepName = currentStepName_;
    newState.address = currentAddress_;
    newState.mFields = currentMFields_;
    newState.lines = currentLines_;

    bool changed = (oldState.stepName != newState.stepName) ||
                   (oldState.address != newState.address) ||
                   (oldState.mFields != newState.mFields) ||
                   (oldState.lines != newState.lines);

    if (changed) {
        if (scene_ && scene_->undoStack()) {
            scene_->undoStack()->push(
                new ModifyMasterBlockParamsCommand(scene_, block_, oldState, newState));
        } else {
            block_->setStepName(newState.stepName);
            block_->setAddress(newState.address);
            for (int i = 1; i <= 15; ++i) {
                block_->setMField(i, newState.mFields[i]);
            }
            block_->setAllLines(newState.lines);
        }
    }

    accept();
}

} // namespace editor
