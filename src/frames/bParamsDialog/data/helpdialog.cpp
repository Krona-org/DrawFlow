#include "helpdialog.h"
#include "src/elements/masterBlockElement/k1804microcode.h"
#include "src/widgets/smoothscrollarea.h"

#include <QApplication>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <array>

namespace editor {

namespace {

/**
 * @brief Нативный графический виджет предпросмотра блока микрокоманды (MasterBlock),
 *        который рисует блок точно так же, как он выглядит на рабочей схеме,
 *        используя реальный генератор мнемоник K1804Microcode::generateBlockLines().
 */
class BlockPreviewWidget : public QWidget {
public:
  BlockPreviewWidget(const QString &stepLabel, const QString &octAddress,
                     const std::array<quint16, 16> &mFields,
                     QWidget *parent = nullptr)
      : QWidget(parent), stepLabel_(stepLabel), octAddress_(octAddress),
        lines_(K1804Microcode::generateBlockLines(mFields)) {
    setFixedSize(270, 178);
  }

protected:
  void paintEvent(QPaintEvent *event) override {
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Фон мини-холста с тонкой сеткой
    const QRectF canvasRect(1, 1, width() - 2, height() - 2);
    const bool darkTheme = qApp && qApp->property("drawflowDarkTheme").toBool();
    p.setPen(QPen(darkTheme ? QColor(104, 104, 104) : QColor(203, 213, 225), 1));
    p.setBrush(darkTheme ? QColor(76, 76, 76) : QColor(241, 245, 249));
    p.drawRoundedRect(canvasRect, 8, 8);

    // Цвет сетки согласован с цветом сетки холста (QColor(195, 205, 218))
    p.setPen(QPen(darkTheme ? QColor(104, 104, 104) : QColor(195, 205, 218), 1));
    for (int x = 16; x < width() - 4; x += 16) {
      p.drawLine(x, 2, x, height() - 3);
    }
    for (int y = 16; y < height() - 4; y += 16) {
      p.drawLine(2, y, width() - 3, y);
    }

    // Основной прямоугольник MasterBlock
    const QRectF blockRect(26, 26, width() - 52, height() - 40);
    p.setPen(QPen(QColor(51, 65, 85), 1.8));
    p.setBrush(darkTheme ? QColor(190, 190, 190) : QColor(248, 250, 252));
    p.drawRect(blockRect);

    // Верхние плашки: слева — шаг/метка, справа — восьмеричный адрес МК
    const QRectF leftBadge(blockRect.left() + 10, blockRect.top() - 10, 52, 20);
    const QRectF rightBadge(blockRect.right() - 62, blockRect.top() - 10, 52,
                            20);

    p.setPen(QPen(QColor(51, 65, 85), 1.6));
    p.setBrush(darkTheme ? QColor(190, 190, 190) : QColor(248, 250, 252));
    p.drawRect(leftBadge);
    p.drawRect(rightBadge);

    QFont badgeFont(QStringLiteral("Segoe UI"), 9, QFont::Bold);
    p.setFont(badgeFont);
    p.setPen(QColor(15, 23, 42));
    p.drawText(leftBadge, Qt::AlignCenter, stepLabel_);
    p.drawText(rightBadge, Qt::AlignCenter, octAddress_);

    // Синие порты подключения ('x') слева и справа
    p.setPen(QPen(QColor(37, 99, 235), 1.6));
    const std::array<qreal, 3> portRatios = {0.30, 0.55, 0.80};
    for (qreal r : portRatios) {
      const qreal py = blockRect.top() + blockRect.height() * r;
      const qreal lx = blockRect.left();
      const qreal rx = blockRect.right();
      const qreal d = 3.5;
      p.drawLine(QPointF(lx - d, py - d), QPointF(lx + d, py + d));
      p.drawLine(QPointF(lx - d, py + d), QPointF(lx + d, py - d));
      p.drawLine(QPointF(rx - d, py - d), QPointF(rx + d, py + d));
      p.drawLine(QPointF(rx - d, py + d), QPointF(rx + d, py - d));
    }

    // 6 строк текста микрокоманды внутри блока
    QFont lineFont(QStringLiteral("Consolas"), 8, QFont::Medium);
    p.setFont(lineFont);
    p.setPen(QColor(15, 23, 42));

    const qreal textTop = blockRect.top() + 14;
    const qreal textHeight = blockRect.height() - 18;
    const qreal rowH = textHeight / 6.0;

    for (int i = 0; i < 6; ++i) {
      const QRectF lineRect(blockRect.left() + 6, textTop + i * rowH,
                            blockRect.width() - 12, rowH);
      p.drawText(lineRect, Qt::AlignCenter, lines_[static_cast<size_t>(i)]);
    }
  }

private:
  QString stepLabel_;
  QString octAddress_;
  std::array<QString, 6> lines_;
};

// Вспомогательные фабрики нативных виджетов со стилями из light.qss

QWidget *wrapInScrollPage(QWidget *contentWidget, QWidget *parent) {
  auto *scroll = new SmoothScrollArea(parent);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  contentWidget->setObjectName(QStringLiteral("helpPageContainer"));
  scroll->setWidget(contentWidget);
  return scroll;
}

QFrame *createCard(QWidget *parent, QVBoxLayout **outLayout) {
  auto *card = new QFrame(parent);
  card->setObjectName(QStringLiteral("helpCard"));
  card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  auto *lay = new QVBoxLayout(card);
  lay->setContentsMargins(16, 14, 16, 14);
  lay->setSpacing(10);
  if (outLayout) {
    *outLayout = lay;
  }
  return card;
}

QLabel *createBadge(const QString &text, const QString &badgeObjectName,
                    QWidget *parent) {
  auto *lbl = new QLabel(text, parent);
  lbl->setObjectName(badgeObjectName);
  lbl->setAlignment(Qt::AlignCenter);
  lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  return lbl;
}

QLabel *createCodeChip(const QString &text, QWidget *parent) {
  auto *lbl = new QLabel(text, parent);
  lbl->setObjectName(QStringLiteral("helpCodeChip"));
  lbl->setAlignment(Qt::AlignCenter);
  lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  return lbl;
}

QLabel *createCardTitle(const QString &text, QWidget *parent) {
  auto *lbl = new QLabel(text, parent);
  lbl->setObjectName(QStringLiteral("helpCardTitle"));
  lbl->setWordWrap(true);
  return lbl;
}

QLabel *createSectionHeading(const QString &text, QWidget *parent) {
  auto *lbl = new QLabel(text, parent);
  lbl->setObjectName(QStringLiteral("helpSectionHeading"));
  lbl->setWordWrap(true);
  return lbl;
}

QLabel *createBodyLabel(const QString &text, QWidget *parent) {
  auto *lbl = new QLabel(text, parent);
  lbl->setObjectName(QStringLiteral("helpBodyText"));
  lbl->setWordWrap(true);
  lbl->setTextFormat(Qt::RichText);
  return lbl;
}

QFrame *createSeparator(QWidget *parent) {
  auto *line = new QFrame(parent);
  line->setObjectName(QStringLiteral("helpRowSeparator"));
  line->setFrameShape(QFrame::HLine);
  return line;
}

/**
 * @brief Создаёт нативную таблицу опций поля M (занимающую 100% ширины карточки)
 *        на основе списка K1804OptionItem.
 */
QFrame *createOptionsTable(const QVector<K1804OptionItem> &items,
                           const QString &badgeStyle, QWidget *parent) {
  auto *tableFrame = new QFrame(parent);
  tableFrame->setObjectName(QStringLiteral("helpInnerTable"));
  tableFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  auto *vbox = new QVBoxLayout(tableFrame);
  vbox->setContentsMargins(12, 10, 12, 10);
  vbox->setSpacing(6);

  // Шапка таблицы
  {
    auto *hdrRow = new QHBoxLayout();
    hdrRow->setSpacing(12);
    auto *lblCode = new QLabel(QStringLiteral("Код (oct / dec)"), tableFrame);
    lblCode->setObjectName(QStringLiteral("helpMutedText"));
    lblCode->setFixedWidth(96);

    auto *lblShort = new QLabel(QStringLiteral("Мнемоника"), tableFrame);
    lblShort->setObjectName(QStringLiteral("helpMutedText"));
    lblShort->setFixedWidth(210);

    auto *lblDesc =
        new QLabel(QStringLiteral("Подробное описание режима"), tableFrame);
    lblDesc->setObjectName(QStringLiteral("helpMutedText"));

    hdrRow->addWidget(lblCode);
    hdrRow->addWidget(lblShort);
    hdrRow->addWidget(lblDesc, 1);
    vbox->addLayout(hdrRow);
    vbox->addWidget(createSeparator(tableFrame));
  }

  for (int i = 0; i < items.size(); ++i) {
    const auto &it = items[i];
    auto *row = new QHBoxLayout();
    row->setSpacing(12);

    auto *codeWrap = new QWidget(tableFrame);
    codeWrap->setFixedWidth(96);
    auto *codeLay = new QHBoxLayout(codeWrap);
    codeLay->setContentsMargins(0, 0, 0, 0);
    codeLay->setSpacing(4);
    auto *badge = createBadge(
        QStringLiteral("%1 oct (%2)").arg(it.octalCode).arg(it.code),
        badgeStyle, codeWrap);
    codeLay->addWidget(badge);
    codeLay->addStretch(1);

    auto *shortLbl = new QLabel(it.shortText, tableFrame);
    shortLbl->setObjectName(QStringLiteral("helpCodeChip"));
    shortLbl->setFixedWidth(210);
    shortLbl->setWordWrap(true);

    auto *descLbl = createBodyLabel(it.description, tableFrame);

    row->addWidget(codeWrap, 0, Qt::AlignTop);
    row->addWidget(shortLbl, 0, Qt::AlignTop);
    row->addWidget(descLbl, 1, Qt::AlignTop);
    vbox->addLayout(row);

    if (i + 1 < items.size()) {
      vbox->addWidget(createSeparator(tableFrame));
    }
  }

  return tableFrame;
}

/**
 * @brief Создаёт карточку отдельного поля M с цветным бейджем, заголовком,
 *        пояснением и встроенным виджетом таблицы.
 */
QFrame *createMFieldCard(const QString &mCode, const QString &badgeStyle,
                         const QString &title, const QString &explanationHtml,
                         QWidget *tableOrExtraWidget, QWidget *parent) {
  QVBoxLayout *cardLayout = nullptr;
  QFrame *card = createCard(parent, &cardLayout);

  auto *headerRow = new QHBoxLayout();
  headerRow->setSpacing(10);
  headerRow->addWidget(createBadge(mCode, badgeStyle, card));
  headerRow->addWidget(createCardTitle(title, card), 1);
  cardLayout->addLayout(headerRow);

  cardLayout->addWidget(createBodyLabel(explanationHtml, card));

  if (tableOrExtraWidget) {
    tableOrExtraWidget->setParent(card);
    cardLayout->addWidget(tableOrExtraWidget);
  }

  return card;
}

} // namespace

HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent) {
  setObjectName(QStringLiteral("helpDialog"));
  setWindowTitle(
      QStringLiteral("Справочник и практическое руководство К1804"));
  setMinimumSize(860, 620);
  resize(980, 720);
  setupUi();
}

void HelpDialog::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(14, 14, 14, 14);
  mainLayout->setSpacing(10);

  // Верхний баннер справочника
  auto *banner = new QFrame(this);
  banner->setObjectName(QStringLiteral("helpBannerCard"));
  auto *bannerLayout = new QHBoxLayout(banner);
  bannerLayout->setContentsMargins(16, 12, 16, 12);
  bannerLayout->setSpacing(14);

  auto *bannerBadge = createBadge(QStringLiteral("К1804 / Am2900"),
                                  QStringLiteral("helpBadgeBlue"), banner);
  bannerLayout->addWidget(bannerBadge, 0, Qt::AlignVCenter);

  auto *bannerTextCol = new QVBoxLayout();
  bannerTextCol->setSpacing(2);
  auto *bannerTitle = new QLabel(
      QStringLiteral("Интерактивный справочник микрокоманд К1804"), banner);
  bannerTitle->setObjectName(QStringLiteral("helpBannerTitle"));
  auto *bannerSub = new QLabel(
      QStringLiteral("Архитектура секционного процессора, назначение полей M1–M15 "
                     "и готовые рецепты построения микропрограмм"),
      banner);
  bannerSub->setObjectName(QStringLiteral("helpBannerSubtitle"));
  bannerSub->setWordWrap(true);
  bannerTextCol->addWidget(bannerTitle);
  bannerTextCol->addWidget(bannerSub);
  bannerLayout->addLayout(bannerTextCol, 1);

  mainLayout->addWidget(banner);

  // Вкладки
  auto *tabs = new QTabWidget(this);
  tabs->setObjectName(QStringLiteral("helpTabs"));
  tabs->addTab(createOverviewTab(), QStringLiteral("Общий обзор"));
  tabs->addTab(createExamplesTab(), QStringLiteral("Примеры задач"));
  tabs->addTab(createTransitionsTab(), QStringLiteral("1. Переходы (ВУ1)"));
  tabs->addTab(createAluTab(), QStringLiteral("2. АЛУ (ВС1)"));
  tabs->addTab(createPswTab(), QStringLiteral("3. Флаги PSW"));
  tabs->addTab(createBusesTab(), QStringLiteral("4. Шины и сигналы"));

  mainLayout->addWidget(tabs, 1);

  // Нижняя панель с кнопкой закрытия
  auto *btnRow = new QHBoxLayout();
  btnRow->addStretch(1);
  auto *btnClose = new QPushButton(QStringLiteral("Закрыть"), this);
  btnClose->setObjectName(QStringLiteral("primaryButton"));
  btnClose->setMinimumWidth(120);
  connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
  btnRow->addWidget(btnClose);
  mainLayout->addLayout(btnRow);
}

QWidget *HelpDialog::createOverviewTab() {
  auto *page = new QWidget(this);
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(12);

  // Карточка 1: Как устроен блок микрокоманды на схеме
  {
    QVBoxLayout *cardLay = nullptr;
    QFrame *card = createCard(page, &cardLay);
    cardLay->addWidget(createCardTitle(
        QStringLiteral("Что представляет собой блок микрокоманды (MasterBlock)"),
        card));

    auto *splitRow = new QHBoxLayout();
    splitRow->setSpacing(18);

    // Слева — подробное объяснение
    auto *leftCol = new QVBoxLayout();
    leftCol->setSpacing(8);
    leftCol->addWidget(createBodyLabel(
        QStringLiteral(
            "Каждый блок на схеме соответствует <b>одной микрокоманде "
            "(одному машинному такту)</b> секционного микропроцессорного "
            "комплекта <b>К1804</b> (отечественный аналог <b>AMD Am2900</b>). "
            "За один такт процессор одновременно читает регистры, выполняет "
            "операцию в АЛУ, обновляет флаги и вычисляет адрес следующей "
            "микрокоманды."),
        card));

    auto *bulletsFrame = new QFrame(card);
    bulletsFrame->setObjectName(QStringLiteral("helpInnerTable"));
    auto *bulletsLay = new QVBoxLayout(bulletsFrame);
    bulletsLay->setContentsMargins(12, 10, 12, 10);
    bulletsLay->setSpacing(6);

    const std::array<std::pair<QString, QString>, 4> items = {{
        {QStringLiteral("Плашка слева сверху"),
         QStringLiteral("<b>Логическая метка шага</b> (например, <b>1</b>) — "
                        "порядковый номер блока на схеме для удобства чтения.")},
        {QStringLiteral("Плашка справа сверху"),
         QStringLiteral("<b>Физический адрес МК в ПЗУ (oct)</b> — восьмеричный "
                        "адрес ячейки (от <b>001</b> до <b>100 oct</b>, т.е. "
                        "1..64 dec). Два блока не могут занимать один адрес.")},
        {QStringLiteral("6 строк внутри блока"),
         QStringLiteral("<b>Автоматическая мнемосхема</b> — генерируется из "
                        "полей M1–M15: операция на шине D, операция АЛУ, запись "
                        "PSW, выходная шина, условие и команда перехода.")},
        {QStringLiteral("Формат в памяти ПЗУ"),
         QStringLiteral("<b>80 бит (5 слов по 16 бит = 10 байт)</b> — при "
                        "экспорте все 15 полей упаковываются в бинарный код.")},
    }};

    for (size_t i = 0; i < items.size(); ++i) {
      auto *r = new QHBoxLayout();
      r->setSpacing(10);
      auto *chip = createCodeChip(items[i].first, bulletsFrame);
      chip->setFixedWidth(155);
      r->addWidget(chip, 0, Qt::AlignTop);
      r->addWidget(createBodyLabel(items[i].second, bulletsFrame), 1);
      bulletsLay->addLayout(r);
      if (i + 1 < items.size()) {
        bulletsLay->addWidget(createSeparator(bulletsFrame));
      }
    }
    leftCol->addWidget(bulletsFrame);
    leftCol->addStretch(1);

    // Справа — живой графический превью-виджет блока
    auto *rightCol = new QVBoxLayout();
    rightCol->setSpacing(6);
    auto *previewCaption =
        new QLabel(QStringLiteral("Вид блока на рабочей схеме:"), card);
    previewCaption->setObjectName(QStringLiteral("helpMutedText"));
    previewCaption->setAlignment(Qt::AlignCenter);

    std::array<quint16, 16> demoFields{};
    demoFields[1] = 0;
    demoFields[4] = 14; // CONT (16 oct)
    demoFields[5] = 1;  // R1
    demoFields[6] = 2;  // R2
    demoFields[9] = 1;  // A :: B
    demoFields[10] = 0; // R + S + C0
    demoFields[11] = 3; // F -> B
    demoFields[12] = 0; // RDI -> D
    demoFields[13] = 2; // Загрузить с выходов МПС
    auto *preview = new BlockPreviewWidget(QStringLiteral("1"),
                                           QStringLiteral("1"), demoFields, card);

    rightCol->addWidget(previewCaption);
    rightCol->addWidget(preview, 0, Qt::AlignCenter);
    rightCol->addStretch(1);

    splitRow->addLayout(leftCol, 1);
    splitRow->addLayout(rightCol, 0);
    cardLay->addLayout(splitRow);
    layout->addWidget(card);
  }

  // Карточка 2: 4 архитектурных узла (M1..M15)
  {
    QVBoxLayout *cardLay = nullptr;
    QFrame *card = createCard(page, &cardLay);
    cardLay->addWidget(createCardTitle(
        QStringLiteral("Четыре функциональные группы полей микрокоманды (M1 – M15)"),
        card));
    cardLay->addWidget(createBodyLabel(
        QStringLiteral("В окне редактора микрокоманды все 15 управляющих полей "
                       "разделены на 4 логических блока в соответствии с "
                       "аппаратными микросхемами комплекта К1804:"),
        card));

    auto *grid = new QGridLayout();
    grid->setSpacing(10);

    struct GroupSummary {
      QString badgeText;
      QString badgeStyle;
      QString title;
      QString fields;
      QString desc;
    };

    const std::array<GroupSummary, 4> groups = {{
        {QStringLiteral("Группа 1"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("Управление переходами и адресацией (К1804ВУ1)"),
         QStringLiteral("Поля: M1, M4, M3, M2"),
         QStringLiteral("Определяет, какая микрокоманда выполнится в следующем "
                        "такте: линейный шаг CONT (Address + 1), условный "
                        "прыжок CJP по флагу TST, вызов подпрограммы CJS/CRTN "
                        "или аппаратный цикл RFCT.")},
        {QStringLiteral("Группа 2"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("Арифметико-логическое устройство (К1804ВС1)"),
         QStringLiteral("Поля: M5, M6, M9, M10, M8, M7, M11"),
         QStringLiteral("Выбирает регистры общего назначения R0..R15 (порты A "
                        "и B), коммутирует входы АЛУ (R и S), выполняет "
                        "сложение, вычитание или логику, сдвигает результат и "
                        "записывает его обратно в РОН или регистр Q.")},
        {QStringLiteral("Группа 3"), QStringLiteral("helpBadgePurple"),
         QStringLiteral("Регистр признаков и состояния (PSW)"),
         QStringLiteral("Поле: M13"),
         QStringLiteral("Управляет 6-разрядным регистром флагов PSW[0..5]: "
                        "сохраняет старые флаги или защёлкивает новые признаки "
                        "переноса (C), переполнения (OVR), знака (N) и нуля (Z) "
                        "после операции АЛУ.")},
        {QStringLiteral("Группа 4"), QStringLiteral("helpBadgeAmber"),
         QStringLiteral("Шины данных и внешние сигналы"),
         QStringLiteral("Поля: M12, M14, M15"),
         QStringLiteral("Управляет мультиплексором входной шины D (подача "
                        "константы M1, регистра команд RK или входных данных), "
                        "выдачей результата на внешние шины A/B и маской M15.")},
    }};

    for (int i = 0; i < 4; ++i) {
      const auto &g = groups[static_cast<size_t>(i)];
      auto *box = new QFrame(card);
      box->setObjectName(QStringLiteral("helpInnerTable"));
      auto *bLay = new QVBoxLayout(box);
      bLay->setContentsMargins(12, 10, 12, 10);
      bLay->setSpacing(6);

      auto *top = new QHBoxLayout();
      top->setSpacing(8);
      top->addWidget(createBadge(g.badgeText, g.badgeStyle, box));
      top->addWidget(createCodeChip(g.fields, box));
      top->addStretch(1);
      bLay->addLayout(top);

      bLay->addWidget(createSectionHeading(g.title, box));
      bLay->addWidget(createBodyLabel(g.desc, box));
      bLay->addStretch(1);

      grid->addWidget(box, i / 2, i % 2);
    }

    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    cardLay->addLayout(grid);
    layout->addWidget(card);
  }

  // Карточка 3: Расшифровка 6 строк внутри блока
  {
    QVBoxLayout *cardLay = nullptr;
    QFrame *card = createCard(page, &cardLay);
    cardLay->addWidget(createCardTitle(
        QStringLiteral("Построчная расшифровка 6 строк текста внутри блока"),
        card));

    auto *table = new QFrame(card);
    table->setObjectName(QStringLiteral("helpInnerTable"));
    auto *tLay = new QVBoxLayout(table);
    tLay->setContentsMargins(12, 10, 12, 10);
    tLay->setSpacing(6);

    const std::array<std::tuple<QString, QString, QString>, 6> rows = {{
        {QStringLiteral("Строка 1"), QStringLiteral("M12 (Шина D)"),
         QStringLiteral("Источник данных на входной шине D АЛУ: "
                        "<b>RDI --> D</b> (вход), <b>M1 --> D</b> (константа из "
                        "M1) или <b>RK --> D</b> (регистр команд).")},
        {QStringLiteral("Строка 2"),
         QStringLiteral("M5, M6, M7, M8, M9, M10, M11 (АЛУ)"),
         QStringLiteral("Выполняемая операция АЛУ, приёмник результата и "
                        "сдвиг. Например: <b>R1 + R2 --> R2</b> или "
                        "<b>NOP</b> (если запись отключена и операция пустая).")},
        {QStringLiteral("Строка 3"), QStringLiteral("M13 (Регистр PSW)"),
         QStringLiteral("Режим обновления флагов: <b>PSW <-- PSW</b> (хранение), "
                        "<b>PSW <-- МПС</b> (обновление флагов Z, N, OVR, C) "
                        "или <b>PSW <-- СДВ</b>.")},
        {QStringLiteral("Строка 4"), QStringLiteral("M14 (Выходная шина)"),
         QStringLiteral("Внешний выходной строб: <b>NOP</b> (нет выдачи), "
                        "<b>Y --> B</b> (результат на шину данных) или "
                        "<b>Y --> A</b> (результат на шину адреса).")},
        {QStringLiteral("Строка 5"), QStringLiteral("M4, M3, M2 (Переход)"),
         QStringLiteral("Мнемоника команды БМУ и проверяемое условие: например, "
                        "<b>CONT; TST = PSW[0]</b> или "
                        "<b>CJP; TST = NOT(PSW[3])</b>.")},
        {QStringLiteral("Строка 6"), QStringLiteral("M1, M15 (Константы)"),
         QStringLiteral("Восьмеричные значения поля адреса/константы <b>M1</b> "
                        "и поля маски <b>M15</b>: например, "
                        "<b>M1: 15; M15: 0</b>.")},
    }};

    for (size_t i = 0; i < rows.size(); ++i) {
      auto *r = new QHBoxLayout();
      r->setSpacing(12);
      auto *b1 = createBadge(std::get<0>(rows[i]),
                             QStringLiteral("helpBadgeBlue"), table);
      b1->setFixedWidth(84);
      auto *b2 = createCodeChip(std::get<1>(rows[i]), table);
      b2->setFixedWidth(210);
      auto *desc = createBodyLabel(std::get<2>(rows[i]), table);

      r->addWidget(b1, 0, Qt::AlignTop);
      r->addWidget(b2, 0, Qt::AlignTop);
      r->addWidget(desc, 1, Qt::AlignTop);
      tLay->addLayout(r);
      if (i + 1 < rows.size()) {
        tLay->addWidget(createSeparator(table));
      }
    }

    cardLay->addWidget(table);
    layout->addWidget(card);
  }

  layout->addStretch(1);
  return wrapInScrollPage(page, this);
}

QWidget *HelpDialog::createExamplesTab() {
  auto *page = new QWidget(this);
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(12);

  struct StepSetting {
    QString mCode;
    QString badgeStyle;
    QString valueText;
    QString reasonText;
  };

  struct PracticalExample {
    QString badgeText;
    QString badgeStyle;
    QString title;
    QString goalHtml;
    QString stepLabel;
    QString octAddr;
    std::array<quint16, 16> mFields;
    QVector<StepSetting> settings;
  };

  QVector<PracticalExample> examples;

  // Пример 1: Сложение двух регистров R1 + R2 -> R2
  {
    PracticalExample ex;
    ex.badgeText = QStringLiteral("Пример 1 • Арифметика");
    ex.badgeStyle = QStringLiteral("helpBadgeGreen");
    ex.title = QStringLiteral(
        "Сложение двух регистров (R1 + R2 --> R2) и переход к следующему шагу");
    ex.goalHtml = QStringLiteral(
        "<b>Задача:</b> сложить содержимое регистров общего назначения "
        "<b>R1</b> и <b>R2</b>, записать сумму обратно в <b>R2</b>, зафиксировать "
        "признаки результата в регистре состояния <b>PSW</b> и перейти к "
        "следующей по порядку микрокоманде.");
    ex.stepLabel = QStringLiteral("1");
    ex.octAddr = QStringLiteral("1");
    ex.mFields.fill(0);
    ex.mFields[4] = 14; // CONT (16 oct)
    ex.mFields[5] = 1;  // Порт A = R1
    ex.mFields[6] = 2;  // Порт B = R2
    ex.mFields[8] = 0;  // C0 = 0
    ex.mFields[9] = 1;  // R :: S = A :: B
    ex.mFields[10] = 0; // F = R + S + C0
    ex.mFields[11] = 3; // F -> B (запись в РОН по адресу порта B)
    ex.mFields[13] = 2; // Загрузить PSW с выходов МПС
    ex.settings = {
        {QStringLiteral("M5"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("РОН R0..R15 -> R1 (01 oct)"),
         QStringLiteral("Подаём регистр R1 на порт A")},
        {QStringLiteral("M6"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("РОН R0..R15 -> R2 (02 oct)"),
         QStringLiteral("Подаём регистр R2 на порт B (он же станет приёмником)")},
        {QStringLiteral("M9"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[01 oct] R :: S = A :: B"),
         QStringLiteral("Подключаем оба порта A и B ко входам АЛУ R и S")},
        {QStringLiteral("M10"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[00 oct] F = R + S + C0"),
         QStringLiteral("Операция суммирования в АЛУ")},
        {QStringLiteral("M8"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[00 oct] C0 = '0'"),
         QStringLiteral("Входной перенос равен 0 (чистое сложение R1 + R2)")},
        {QStringLiteral("M11"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[03 oct] РОН: F -> B (без сдвига)"),
         QStringLiteral("Результат АЛУ F записывается в регистр порта B (R2)")},
        {QStringLiteral("M13"), QStringLiteral("helpBadgePurple"),
         QStringLiteral("[02 oct] Загрузить с выходов МПС"),
         QStringLiteral("Защёлкиваем флаги Z, N, OVR, C от суммы")},
        {QStringLiteral("M4"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("[16 oct] CONT — Последовательная выборка"),
         QStringLiteral("Переход к следующей ячейке (Address + 1)")},
    };
    examples.push_back(ex);
  }

  // Пример 2: Вычитание R5 - R3 -> R5
  {
    PracticalExample ex;
    ex.badgeText = QStringLiteral("Пример 2 • Вычитание");
    ex.badgeStyle = QStringLiteral("helpBadgeGreen");
    ex.title = QStringLiteral(
        "Вычитание регистров (R5 - R3 --> R5) в дополнительном коде");
    ex.goalHtml = QStringLiteral(
        "<b>Задача:</b> вычислить разность <b>R5 - R3</b> и сохранить результат "
        "в <b>R5</b>. В К1804ВС1 вычитание выполняется по формуле "
        "<b>F = S - R - 1 + C0</b> (код <b>01 oct</b>) или "
        "<b>F = R - S - 1 + C0</b> (код <b>02 oct</b>). Чтобы получить точное "
        "вычитание без заёма (<b>- 1 + 1 = 0</b>), необходимо подать входной "
        "перенос <b>C0 = '1'</b> в поле <b>M8</b>.");
    ex.stepLabel = QStringLiteral("2");
    ex.octAddr = QStringLiteral("2");
    ex.mFields.fill(0);
    ex.mFields[4] = 14; // CONT (16 oct)
    ex.mFields[5] = 3;  // Порт A = R3 (вычитаемое R)
    ex.mFields[6] = 5;  // Порт B = R5 (уменьшаемое S и приёмник)
    ex.mFields[8] = 1;  // C0 = '1'
    ex.mFields[9] = 1;  // R :: S = A :: B
    ex.mFields[10] = 1; // F = S - R - 1 + C0
    ex.mFields[11] = 3; // F -> B (R5)
    ex.mFields[13] = 2; // Обновить PSW
    ex.settings = {
        {QStringLiteral("M5"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("РОН R0..R15 -> R3 (03 oct)"),
         QStringLiteral("Порт A = R3 (вычитаемое R)")},
        {QStringLiteral("M6"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("РОН R0..R15 -> R5 (05 oct)"),
         QStringLiteral("Порт B = R5 (уменьшаемое S и регистр-приёмник)")},
        {QStringLiteral("M9"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[01 oct] R :: S = A :: B"),
         QStringLiteral("Операнды R = R3, S = R5")},
        {QStringLiteral("M10"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[01 oct] F = S - R - 1 + C0"),
         QStringLiteral("Вычитание S минус R (R5 - R3)")},
        {QStringLiteral("M8"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[01 oct] C0 = '1'"),
         QStringLiteral("Компенсирует -1 в формуле АЛУ (даёт чистое R5 - R3)")},
        {QStringLiteral("M11"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[03 oct] РОН: F -> B"),
         QStringLiteral("Запись разности в R5")},
    };
    examples.push_back(ex);
  }

  // Пример 3: Загрузка восьмеричной константы в регистр (M1 = 077 -> R3)
  {
    PracticalExample ex;
    ex.badgeText = QStringLiteral("Пример 3 • Константа");
    ex.badgeStyle = QStringLiteral("helpBadgeAmber");
    ex.title = QStringLiteral(
        "Загрузка константы из поля M1 в регистр (077 oct --> R3)");
    ex.goalHtml = QStringLiteral(
        "<b>Задача:</b> записать числовую константу <b>077 oct</b> (63 dec) "
        "напрямую из микрокоманды в регистр <b>R3</b>. Для этого константа "
        "задаётся в поле <b>M1</b>, коммутируется на входную шину <b>D</b> "
        "через поле <b>M12</b>, подаётся на АЛУ через пару операндов "
        "<b>D :: 0</b> в поле <b>M9</b> и записывается в порт B (<b>R3</b>).");
    ex.stepLabel = QStringLiteral("3");
    ex.octAddr = QStringLiteral("3");
    ex.mFields.fill(0);
    ex.mFields[1] = 077; // Константа 077 oct
    ex.mFields[4] = 14;  // CONT
    ex.mFields[6] = 3;   // Порт B = R3
    ex.mFields[8] = 0;   // C0 = 0
    ex.mFields[9] = 7;   // R :: S = D :: 0
    ex.mFields[10] = 0;  // F = R + S + C0 = D + 0
    ex.mFields[11] = 3;  // F -> B (R3)
    ex.mFields[12] = 1;  // Константа M1 -> D
    ex.settings = {
        {QStringLiteral("M1"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("77 (oct) = 63 dec"),
         QStringLiteral("Значение загружаемой 16-битной константы")},
        {QStringLiteral("M12"), QStringLiteral("helpBadgeAmber"),
         QStringLiteral("[01 oct] Константа из МК (M1) --> D"),
         QStringLiteral("Подаём поле M1 на внутреннюю шину данных D")},
        {QStringLiteral("M6"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("РОН R0..R15 -> R3 (03 oct)"),
         QStringLiteral("Выбираем целевой регистр R3 в порту B")},
        {QStringLiteral("M9"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[07 oct] R :: S = D :: 0"),
         QStringLiteral("На вход R подаётся шина D, на вход S — константа 0")},
        {QStringLiteral("M10"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[00 oct] F = R + S + C0"),
         QStringLiteral("F = D + 0 = D (транзит константы через АЛУ)")},
        {QStringLiteral("M11"), QStringLiteral("helpBadgeGreen"),
         QStringLiteral("[03 oct] РОН: F -> B"),
         QStringLiteral("Записываем результат в регистр R3")},
    };
    examples.push_back(ex);
  }

  // Пример 4: Условный переход по флагу нуля Z (CJP)
  {
    PracticalExample ex;
    ex.badgeText = QStringLiteral("Пример 4 • Ветвление");
    ex.badgeStyle = QStringLiteral("helpBadgeBlue");
    ex.title = QStringLiteral(
        "Условный переход: если результат НЕ равен нулю (Z == 0), перейти на 015 oct");
    ex.goalHtml = QStringLiteral(
        "<b>Задача:</b> проверить флаг нуля <b>Z (PSW[3])</b>, сформированный в "
        "предыдущем такте. Если результат <b>НЕ равен нулю</b> (инверсия "
        "условия), выполнить переход на адрес микрокоманды <b>015 oct</b>; "
        "иначе продолжить выполнение со следующего адреса.");
    ex.stepLabel = QStringLiteral("4");
    ex.octAddr = QStringLiteral("4");
    ex.mFields.fill(0);
    ex.mFields[1] = 015; // Целевой адрес перехода 015 oct
    ex.mFields[2] = 1;   // Инверсия условия (NOT)
    ex.mFields[3] = 3;   // PSW[3] (флаг нуля Z)
    ex.mFields[4] = 3;   // CJP — Условный переход по адресу из МК
    ex.settings = {
        {QStringLiteral("M1"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("15 (oct) = 13 dec"),
         QStringLiteral("Адрес микрокоманды, куда совершается прыжок")},
        {QStringLiteral("M4"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("[03 oct] CJP — Условный переход по адресу из РМК"),
         QStringLiteral("Если условие истинно -> адрес M1, иначе -> Address + 1")},
        {QStringLiteral("M3"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("[03 oct] PSW[3] (Флаг нуля Z)"),
         QStringLiteral("Опрашиваем 3-й бит регистра состояния (признак Z)")},
        {QStringLiteral("M2"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("[01 oct] Инверсия условия (NOT)"),
         QStringLiteral("Переход сработает при Z == 0 (результат не нулевой)")},
    };
    examples.push_back(ex);
  }

  // Пример 5: Вызов микроподпрограммы и аппаратный цикл
  {
    PracticalExample ex;
    ex.badgeText = QStringLiteral("Пример 5 • Подпрограмма");
    ex.badgeStyle = QStringLiteral("helpBadgeBlue");
    ex.title = QStringLiteral(
        "Безусловный вызов микроподпрограммы по адресу 040 oct (CJS)");
    ex.goalHtml = QStringLiteral(
        "<b>Задача:</b> вызвать микроподпрограмму по адресу <b>040 oct</b> с "
        "сохранением адреса возврата (текущий адрес + 1) во внутреннем стеке "
        "К1804ВУ1. Чтобы вызов сработал <b>безусловно</b>, выбирается условие "
        "<b>Константа 0</b> (код <b>07 oct</b>) вместе с <b>Инверсией условия "
        "NOT</b> (<b>NOT(0) = 1</b> — всегда истинно). Для возврата в конце "
        "подпрограммы используется команда <b>CRTN (12 oct)</b>.");
    ex.stepLabel = QStringLiteral("5");
    ex.octAddr = QStringLiteral("5");
    ex.mFields.fill(0);
    ex.mFields[1] = 040; // Адрес ПП 040 oct
    ex.mFields[2] = 1;   // Инверсия NOT(0) = 1
    ex.mFields[3] = 7;   // Константа 0
    ex.mFields[4] = 1;   // CJS (01 oct)
    ex.settings = {
        {QStringLiteral("M1"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("40 (oct) = 32 dec"),
         QStringLiteral("Начальный адрес вызываемой микроподпрограммы")},
        {QStringLiteral("M4"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("[01 oct] CJS — Условный переход к ПП по адресу из РМК"),
         QStringLiteral("Помещает Address + 1 в стек и переходит на M1")},
        {QStringLiteral("M3"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("[07 oct] Константа 0"),
         QStringLiteral("Фиксированный логический уровень 0")},
        {QStringLiteral("M2"), QStringLiteral("helpBadgeBlue"),
         QStringLiteral("[01 oct] Инверсия условия (NOT)"),
         QStringLiteral("Превращает Константу 0 в логическую 1 (безусловный вызов)")},
    };
    examples.push_back(ex);
  }

  for (const auto &ex : examples) {
    QVBoxLayout *cardLay = nullptr;
    QFrame *card = createCard(page, &cardLay);

    auto *hdrRow = new QHBoxLayout();
    hdrRow->setSpacing(10);
    hdrRow->addWidget(createBadge(ex.badgeText, ex.badgeStyle, card));
    hdrRow->addWidget(createCardTitle(ex.title, card), 1);
    cardLay->addLayout(hdrRow);

    cardLay->addWidget(createBodyLabel(ex.goalHtml, card));

    auto *contentRow = new QHBoxLayout();
    contentRow->setSpacing(16);

    // Таблица настроек слева
    auto *settingsTable = new QFrame(card);
    settingsTable->setObjectName(QStringLiteral("helpInnerTable"));
    settingsTable->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Preferred);
    auto *sLay = new QVBoxLayout(settingsTable);
    sLay->setContentsMargins(12, 10, 12, 10);
    sLay->setSpacing(6);

    for (int i = 0; i < ex.settings.size(); ++i) {
      const auto &st = ex.settings[i];
      auto *r = new QHBoxLayout();
      r->setSpacing(10);
      auto *b = createBadge(st.mCode, st.badgeStyle, settingsTable);
      b->setFixedWidth(44);
      auto *val = createCodeChip(st.valueText, settingsTable);
      val->setFixedWidth(250);
      val->setWordWrap(true);
      auto *why = createBodyLabel(st.reasonText, settingsTable);

      r->addWidget(b, 0, Qt::AlignTop);
      r->addWidget(val, 0, Qt::AlignTop);
      r->addWidget(why, 1, Qt::AlignTop);
      sLay->addLayout(r);
      if (i + 1 < ex.settings.size()) {
        sLay->addWidget(createSeparator(settingsTable));
      }
    }

    // Графический блок справа
    auto *previewCol = new QVBoxLayout();
    previewCol->setSpacing(4);
    auto *lblPrev =
        new QLabel(QStringLiteral("Результат на схеме:"), card);
    lblPrev->setObjectName(QStringLiteral("helpMutedText"));
    lblPrev->setAlignment(Qt::AlignCenter);
    auto *previewWidget =
        new BlockPreviewWidget(ex.stepLabel, ex.octAddr, ex.mFields, card);
    previewCol->addWidget(lblPrev);
    previewCol->addWidget(previewWidget, 0, Qt::AlignCenter);
    previewCol->addStretch(1);

    contentRow->addWidget(settingsTable, 1);
    contentRow->addLayout(previewCol, 0);
    cardLay->addLayout(contentRow);

    layout->addWidget(card);
  }

  layout->addStretch(1);
  return wrapInScrollPage(page, this);
}

QWidget *HelpDialog::createTransitionsTab() {
  auto *page = new QWidget(this);
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(12);

  const QString badgeBlue = QStringLiteral("helpBadgeBlue");

  // Карточка M1
  layout->addWidget(createMFieldCard(
      QStringLiteral("M1"), badgeBlue,
      QStringLiteral("Поле M1 — Константа / Адрес перехода (0..177777 oct)"),
      QStringLiteral(
          "16-разрядное поле прямого адреса или данных (вводится в "
          "восьмеричной системе от <b>0</b> до <b>177777 oct</b>, т.е. "
          "0..65535 dec). Выполняет две роли в зависимости от настроек других "
          "полей:<br>"
          "• <b>Адрес перехода БМУ (при командах CJP, CJS, JSRP, JRP, JP, "
          "CJPP, RPCT):</b> задаёт номер ячейки ПЗУ микрокоманд, куда "
          "передаётся управление.<br>"
          "• <b>Числовая константа данных (при M12 = 01 oct «Константа из МК "
          "--> D»):</b> подаёт 16-битное число напрямую на входную шину D "
          "процессора."),
      nullptr, page));

  // Карточка M4 (полный список 16 команд К1804ВУ1)
  layout->addWidget(createMFieldCard(
      QStringLiteral("M4"), badgeBlue,
      QStringLiteral("Поле M4 — Команда перехода БМУ К1804ВУ1 (16 инструкций)"),
      QStringLiteral(
          "Управляет блоком микропрограммного управления <b>К1804ВУ1 "
          "(Am2909/Am2911)</b>. Определяет источник адреса следующей "
          "микрокоманды: счётчик микрокоманд (+1), поле адреса <b>M1 (РМК)</b>, "
          "внутренний стек возврата из подпрограмм или регистр команд <b>RK</b>."),
      createOptionsTable(K1804Microcode::m4Options(), badgeBlue, page), page));

  // Карточка M2
  layout->addWidget(createMFieldCard(
      QStringLiteral("M2"), badgeBlue,
      QStringLiteral("Поле M2 — Модификация и инверсия проверяемого условия"),
      QStringLiteral(
          "Позволяет инвертировать выбранный в поле <b>M3</b> логический флаг "
          "(например, превратить проверку «Равно нулю Z == 1» в проверку "
          "«Не равно нулю Z == 0»), а также переключает мультиплексор условий "
          "<b>M3</b> в альтернативный режим проверки с маской <b>M15</b> "
          "(при кодах <b>02 oct</b> и <b>03 oct</b>)."),
      createOptionsTable(K1804Microcode::m2Options(), badgeBlue, page), page));

  // Карточка M3 (основной и альтернативный наборы условий)
  {
    auto *m3Container = new QWidget(page);
    auto *m3Lay = new QVBoxLayout(m3Container);
    m3Lay->setContentsMargins(0, 0, 0, 0);
    m3Lay->setSpacing(8);

    m3Lay->addWidget(createSectionHeading(
        QStringLiteral("Основной режим (при M2 = 00 или 01 oct — без маски):"),
        m3Container));
    m3Lay->addWidget(createOptionsTable(K1804Microcode::m3Options(false),
                                        badgeBlue, m3Container));

    m3Lay->addWidget(createSectionHeading(
        QStringLiteral("Режим проверки с маской M15 (при M2 = 02 или 03 oct):"),
        m3Container));
    m3Lay->addWidget(createOptionsTable(K1804Microcode::m3Options(true),
                                        badgeBlue, m3Container));

    layout->addWidget(createMFieldCard(
        QStringLiteral("M3"), badgeBlue,
        QStringLiteral("Поле M3 — Проверяемое условие ветвления TST"),
        QStringLiteral(
            "Выбирает сигнал, который подаётся на вход проверки условия "
            "<b>TST</b> микросхемы К1804ВУ1 для всех условных команд перехода "
            "(<b>CJP, CJS, JSRP, CJV, JRP, CRTN, CJPP, LOOP</b>):"),
        m3Container, page));
  }

  layout->addStretch(1);
  return wrapInScrollPage(page, this);
}

QWidget *HelpDialog::createAluTab() {
  auto *page = new QWidget(this);
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(12);

  const QString badgeGreen = QStringLiteral("helpBadgeGreen");

  // Карточка M5 и M6
  {
    auto *portsTable = new QFrame(page);
    portsTable->setObjectName(QStringLiteral("helpInnerTable"));
    auto *pLay = new QVBoxLayout(portsTable);
    pLay->setContentsMargins(12, 10, 12, 10);
    pLay->setSpacing(6);

    const std::array<std::tuple<QString, QString, QString>, 3> modes = {{
        {QStringLiteral("00 .. 17 oct"),
         QStringLiteral("Прямой выбор РОН R0..R15 (бит 4 = 0)"),
         QStringLiteral("Младшие 4 бита задают номер регистра общего "
                        "назначения от <b>R0 (00 oct)</b> до <b>R15 (17 oct)</b>.")},
        {QStringLiteral("20 oct"),
         QStringLiteral("Младший полубайт регистра команд RK[3..0]"),
         QStringLiteral("Бит 4 = 1, бит 3 = 0: номер регистра РОН выбирается "
                        "динамически из битов 3..0 регистра команд RK.")},
        {QStringLiteral("30 oct"),
         QStringLiteral("Старший полубайт регистра команд RK[7..4]"),
         QStringLiteral("Бит 4 = 1, бит 3 = 1: номер регистра РОН выбирается "
                        "динамически из битов 7..4 регистра команд RK.")},
    }};

    for (size_t i = 0; i < modes.size(); ++i) {
      auto *r = new QHBoxLayout();
      r->setSpacing(12);
      auto *b = createBadge(std::get<0>(modes[i]), badgeGreen, portsTable);
      b->setFixedWidth(100);
      auto *c = createCodeChip(std::get<1>(modes[i]), portsTable);
      c->setFixedWidth(260);
      auto *d = createBodyLabel(std::get<2>(modes[i]), portsTable);
      r->addWidget(b, 0, Qt::AlignTop);
      r->addWidget(c, 0, Qt::AlignTop);
      r->addWidget(d, 1, Qt::AlignTop);
      pLay->addLayout(r);
      if (i + 1 < modes.size()) {
        pLay->addWidget(createSeparator(portsTable));
      }
    }

    layout->addWidget(createMFieldCard(
        QStringLiteral("M5 / M6"), badgeGreen,
        QStringLiteral("Поля M5 и M6 — Адреса регистров порта A и порта B (РОН R0..R15)"),
        QStringLiteral(
            "Двухпортовое сверхоперативное ОЗУ внутри <b>К1804ВС1 (Am2901)</b> "
            "содержит 16 регистров общего назначения <b>R0..R15</b>. Поле "
            "<b>M5</b> выбирает регистр для чтения через <b>Порт A</b>, а поле "
            "<b>M6</b> — регистр для чтения через <b>Порт B</b>. При записи "
            "результата в РОН (поле M11) запись всегда производится в регистр, "
            "адресуемый <b>Портом B (M6)</b>."),
        portsTable, page));
  }

  // Карточка M9 (Источники операндов R и S)
  layout->addWidget(createMFieldCard(
      QStringLiteral("M9"), badgeGreen,
      QStringLiteral("Поле M9 — Коммутатор входов R и S АЛУ (8 комбинаций)"),
      QStringLiteral(
          "Определяет, какие именно данные подаются на левый вход <b>R</b> и "
          "правый вход <b>S</b> арифметико-логического устройства: регистры "
          "портов <b>A</b> и <b>B</b>, расширитель <b>Q</b>, внешняя шина "
          "данных <b>D</b> или аппаратный ноль <b>0</b>."),
      createOptionsTable(K1804Microcode::m9Options(), badgeGreen, page), page));

  // Карточка M10 (Операция АЛУ)
  layout->addWidget(createMFieldCard(
      QStringLiteral("M10"), badgeGreen,
      QStringLiteral("Поле M10 — Арифметическая или логическая функция АЛУ (8 операций)"),
      QStringLiteral(
          "Задаёт операцию, выполняемую над 16-битными входами <b>R</b> и "
          "<b>S</b> с учётом входного переноса <b>C0 (M8)</b>. Результат "
          "обозначается как <b>F</b>."),
      createOptionsTable(K1804Microcode::m10Options(), badgeGreen, page),
      page));

  // Карточка M8 (Входной перенос C0)
  layout->addWidget(createMFieldCard(
      QStringLiteral("M8"), badgeGreen,
      QStringLiteral("Поле M8 — Входной перенос C0 в младший разряд АЛУ"),
      QStringLiteral(
          "Формирует сигнал входного переноса <b>C0</b> для арифметических "
          "операций (коды M10 = 00, 01, 02 oct). При сложении обычно ставится "
          "<b>C0 = '0'</b>, для инкремента (<b>R + 1</b>) ставится "
          "<b>C0 = '1'</b>, при вычитании без заёма (<b>S - R - 1 + C0</b>) "
          "ставится <b>C0 = '1'</b>."),
      createOptionsTable(K1804Microcode::m8Options(), badgeGreen, page), page));

  // Карточка M11 (Приёмник результата и направление сдвига)
  {
    auto *m11Table = new QFrame(page);
    m11Table->setObjectName(QStringLiteral("helpInnerTable"));
    auto *tLay = new QVBoxLayout(m11Table);
    tLay->setContentsMargins(12, 10, 12, 10);
    tLay->setSpacing(6);

    const auto items = K1804Microcode::m11Options();
    for (int i = 0; i < items.size(); ++i) {
      const auto &it = items[i];
      auto *r = new QHBoxLayout();
      r->setSpacing(10);
      auto *b = createBadge(
          QStringLiteral("%1 oct (%2)").arg(it.octalCode).arg(it.code),
          badgeGreen, m11Table);
      b->setFixedWidth(96);

      auto *ronChip = createCodeChip(
          QStringLiteral("РОН: %1 (%2)").arg(it.writeRon, it.shiftRon),
          m11Table);
      ronChip->setFixedWidth(185);

      auto *rqChip = createCodeChip(
          QStringLiteral("RQ: %1 (%2)").arg(it.writeRq, it.shiftRq), m11Table);
      rqChip->setFixedWidth(175);

      auto *outLbl = createBodyLabel(
          QStringLiteral("Выход Y АЛУ: <b>%1</b>").arg(it.outputY), m11Table);

      r->addWidget(b, 0, Qt::AlignTop);
      r->addWidget(ronChip, 0, Qt::AlignTop);
      r->addWidget(rqChip, 0, Qt::AlignTop);
      r->addWidget(outLbl, 1, Qt::AlignTop);
      tLay->addLayout(r);
      if (i + 1 < items.size()) {
        tLay->addWidget(createSeparator(m11Table));
      }
    }

    layout->addWidget(createMFieldCard(
        QStringLiteral("M11"), badgeGreen,
        QStringLiteral("Поле M11 — Куда записать результат АЛУ F и как сдвигать"),
        QStringLiteral(
            "Определяет, куда записывается выход АЛУ <b>F</b>: в регистр "
            "<b>B</b> (РОН), в регистр-расширитель <b>Q (RQ)</b> или никуда "
            "(только выдача на выход <b>Y</b>). Коды <b>04..07 oct</b> "
            "одновременно включают сдвиг результата вправо (<b>/2</b>) или "
            "влево (<b>*2</b>)."),
        m11Table, page));
  }

  // Карточка M7 (Цепь сдвига регистров)
  {
    auto *m7Container = new QWidget(page);
    auto *m7Lay = new QVBoxLayout(m7Container);
    m7Lay->setContentsMargins(0, 0, 0, 0);
    m7Lay->setSpacing(8);

    m7Lay->addWidget(createSectionHeading(
        QStringLiteral("При сдвиге вправо (M11 = 00..05 oct):"), m7Container));
    m7Lay->addWidget(createOptionsTable(K1804Microcode::m7Options(false),
                                        badgeGreen, m7Container));

    m7Lay->addWidget(createSectionHeading(
        QStringLiteral("При сдвиге влево (M11 = 06..07 oct):"), m7Container));
    m7Lay->addWidget(createOptionsTable(K1804Microcode::m7Options(true),
                                        badgeGreen, m7Container));

    layout->addWidget(createMFieldCard(
        QStringLiteral("M7"), badgeGreen,
        QStringLiteral("Поле M7 — Конфигурация входных/выходных цепей сдвига"),
        QStringLiteral(
            "Задаёт схему заполнения освобождающихся крайних разрядов при "
            "сдвиге регистра РОН (<b>[R]</b>) и расширителя (<b>[RQ]</b>): "
            "логический сдвиг с заполнением нулём, арифметический сдвиг или "
            "циклический сдвиг через перенос <b>PSW[0]</b>."),
        m7Container, page));
  }

  layout->addStretch(1);
  return wrapInScrollPage(page, this);
}

QWidget *HelpDialog::createPswTab() {
  auto *page = new QWidget(this);
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(12);

  const QString badgePurple = QStringLiteral("helpBadgePurple");

  // Карточка M13 (4 режима записи PSW)
  layout->addWidget(createMFieldCard(
      QStringLiteral("M13"), badgePurple,
      QStringLiteral("Поле M13 — Управление регистром состояния процессора (PSW)"),
      QStringLiteral(
          "Регистр состояния <b>PSW (Program Status Word)</b> хранит 6 битовых "
          "признаков результата выполнения операций АЛУ и сдвигателя. Поле "
          "<b>M13</b> определяет, должны ли флаги сохранять прежнее значение "
          "или обновляться в текущем такте:"),
      createOptionsTable(K1804Microcode::m13Options(), badgePurple, page),
      page));

  // Подробная побитовая карта разрядов PSW[0..5] для каждого режима M13
  {
    QVBoxLayout *cardLay = nullptr;
    QFrame *card = createCard(page, &cardLay);
    cardLay->addWidget(createCardTitle(
        QStringLiteral("Побитовая расшифровка разрядов PSW[0] .. PSW[5]"),
        card));
    cardLay->addWidget(createBodyLabel(
        QStringLiteral("Именно эти разряды опрашиваются в поле <b>M3 (Условие "
                       "TST)</b> при организации условных переходов:"),
        card));

    auto *table = new QFrame(card);
    table->setObjectName(QStringLiteral("helpInnerTable"));
    auto *tLay = new QVBoxLayout(table);
    tLay->setContentsMargins(12, 10, 12, 10);
    tLay->setSpacing(6);

    const std::array<std::tuple<QString, QString, QString>, 6> bits = {{
        {QStringLiteral("PSW[0]"), QStringLiteral("Флаг переноса / сдвига (C)"),
         QStringLiteral("При M13 = 02 oct принимает выход переноса АЛУ "
                        "<b>C_n+4 (Carry)</b>; при M13 = 01 oct принимает бит, "
                        "выдвигаемый из регистра РОН при сдвиге.")},
        {QStringLiteral("PSW[1]"), QStringLiteral("Флаг переполнения (OVR)"),
         QStringLiteral("При M13 = 02 oct фиксирует знаковое арифметическое "
                        "переполнение <b>OVR</b>; при M13 = 01 oct — бит, "
                        "выдвигаемый из расширителя RQ.")},
        {QStringLiteral("PSW[2]"), QStringLiteral("Флаг знака результата (N)"),
         QStringLiteral("Принимает старший (знаковый) разряд результата АЛУ "
                        "<b>F[15]</b> (1 — число отрицательное, 0 — "
                        "положительное).")},
        {QStringLiteral("PSW[3]"), QStringLiteral("Флаг нулевого результата (Z)"),
         QStringLiteral("Равен <b>1</b>, если все 16 бит результата на выходе "
                        "АЛУ равны нулю (<b>F == 0</b>). Чаще всего "
                        "используется при проверках на равенство.")},
        {QStringLiteral("PSW[4]"),
         QStringLiteral("Генерация переноса (Ginv / F[15])"),
         QStringLiteral("При M13 = 02 oct сохраняет инверсный сигнал генерации "
                        "переноса <b>/G</b>; при M13 = 01 oct — знаковый бит "
                        "<b>F[15]</b>.")},
        {QStringLiteral("PSW[5]"),
         QStringLiteral("Распространение переноса (Pinv / Z)"),
         QStringLiteral("При M13 = 02 oct сохраняет инверсный сигнал "
                        "распространения переноса <b>/P</b>; при M13 = 01 oct "
                        "— флаг нуля <b>Z</b>.")},
    }};

    for (size_t i = 0; i < bits.size(); ++i) {
      auto *r = new QHBoxLayout();
      r->setSpacing(12);
      auto *b = createBadge(std::get<0>(bits[i]), badgePurple, table);
      b->setFixedWidth(84);
      auto *c = createCodeChip(std::get<1>(bits[i]), table);
      c->setFixedWidth(235);
      auto *d = createBodyLabel(std::get<2>(bits[i]), table);

      r->addWidget(b, 0, Qt::AlignTop);
      r->addWidget(c, 0, Qt::AlignTop);
      r->addWidget(d, 1, Qt::AlignTop);
      tLay->addLayout(r);
      if (i + 1 < bits.size()) {
        tLay->addWidget(createSeparator(table));
      }
    }

    cardLay->addWidget(table);
    layout->addWidget(card);
  }

  layout->addStretch(1);
  return wrapInScrollPage(page, this);
}

QWidget *HelpDialog::createBusesTab() {
  auto *page = new QWidget(this);
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(12);

  const QString badgeAmber = QStringLiteral("helpBadgeAmber");

  // Карточка M12 (Шина данных D)
  layout->addWidget(createMFieldCard(
      QStringLiteral("M12"), badgeAmber,
      QStringLiteral("Поле M12 — Источник данных для входной шины D (8 режимов)"),
      QStringLiteral(
          "Управляет входным мультиплексором шины <b>D</b> микропроцессорной "
          "секции К1804ВС1. Чтобы данные с шины D попали в АЛУ, в поле "
          "<b>M9</b> необходимо выбрать пару операндов с участием входа "
          "<b>D</b> (<b>D :: A</b>, <b>D :: Q</b> или <b>D :: 0</b>)."),
      createOptionsTable(K1804Microcode::m12Options(), badgeAmber, page),
      page));

  // Карточка M14 (Выходная шина)
  layout->addWidget(createMFieldCard(
      QStringLiteral("M14"), badgeAmber,
      QStringLiteral("Поле M14 — Управление выходными буферами шины данных B и адреса A"),
      QStringLiteral(
          "Определяет, выдаётся ли 16-битный результат с выхода <b>Y</b> АЛУ "
          "наружу из процессора в текущем такте:"),
      createOptionsTable(K1804Microcode::m14Options(), badgeAmber, page),
      page));

  // Карточка M15 (Маска константы)
  layout->addWidget(createMFieldCard(
      QStringLiteral("M15"), badgeAmber,
      QStringLiteral("Поле M15 — Восьмеричная маска константы (0..377 oct)"),
      QStringLiteral(
          "8-битное поле маски (от <b>000</b> до <b>377 oct</b>, т.е. 0..255 "
          "dec). Используется блоком проверки условий, когда в поле <b>M2</b> "
          "включён режим маскирования (коды <b>02 oct</b> или <b>03 oct</b>), "
          "позволяя выделять и опрашивать произвольные комбинации разрядов "
          "регистра состояния или внешних линий."),
      nullptr, page));

  layout->addStretch(1);
  return wrapInScrollPage(page, this);
}

} // namespace editor
