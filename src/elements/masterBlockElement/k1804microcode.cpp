#include "k1804microcode.h"

namespace editor {

QString K1804Microcode::toOctal(quint16 val, int minWidth) {
  QString s = QString::number(val, 8);
  if (s.length() < minWidth) {
    s = QString(minWidth - s.length(), QLatin1Char('0')) + s;
  }
  return s;
}

quint16 K1804Microcode::fromOctal(const QString &octStr, bool *ok) {
  QString trimmed = octStr.trimmed();
  if (trimmed.isEmpty()) {
    if (ok)
      *ok = true;
    return 0;
  }
  bool localOk = false;
  uint val = trimmed.toUInt(&localOk, 8);
  if (localOk && val <= 0xFFFF) {
    if (ok)
      *ok = true;
    return static_cast<quint16>(val);
  }
  if (ok)
    *ok = false;
  return 0;
}

// SWModData1: SWArrayM12 (Шина D)
QVector<K1804OptionItem> K1804Microcode::m12Options() {
  static const char *items[] = {"RDI --> D",
                                "ZI --> D",
                                "M1 --> D",
                                "PSW --> D",
                                "RDI --> D ; BI --> RDI",
                                "ZI --> D ; BI --> RDI",
                                "M1 --> D ; BI --> RDI",
                                "RDI --> D ; BI --> RDI ; BI --> RK"};
  QVector<K1804OptionItem> res;
  res.reserve(8);
  for (int i = 0; i < 8; ++i) {
    QString text = QString::fromUtf8(items[i]);
    res.push_back({i, toOctal(i), text, text});
  }
  return res;
}

// SWModData4: SWArrayM14 (Выходная шина / внешние сигналы)
QVector<K1804OptionItem> K1804Microcode::m14Options() {
  static const char *items[] = {"null", "Y --> RD0", "Y --> MAR", "null"};
  QVector<K1804OptionItem> res;
  res.reserve(4);
  for (int i = 0; i < 4; ++i) {
    QString text = QString::fromUtf8(items[i]);
    QString desc = (text == QLatin1String("null"))
                       ? QStringLiteral("null (NOP — нет пересылки)")
                       : text;
    res.push_back({i, toOctal(i), text, desc});
  }
  return res;
}

// SWModData3: SWArrayM13 (Регистр слова состояния PSW)
QVector<K1804OptionItem> K1804Microcode::m13Options() {
  return {{0, QStringLiteral("0"), QStringLiteral("PSW <-- PSW"),
           QStringLiteral("Сохранение PSW")},
          {1, QStringLiteral("1"), QStringLiteral("PSW <-- C16, OVR, Z, F15"),
           QStringLiteral("Загрузка по признакам МПС К1804ВС1")},
          {2, QStringLiteral("2"),
           QStringLiteral("PSW <-- OVR, Z, F15 [+] OVR"),
           QStringLiteral("Загрузка с учетом сдвигов")},
          {3, QStringLiteral("3"), QStringLiteral("PSW <-- Y"),
           QStringLiteral("Загрузка с шины Y")}};
}

QString K1804Microcode::m13HelpText(int m13Index) {
  switch (m13Index) {
  case 0:
    return QStringLiteral("Разряды PSW — заносимая информация:\n"
                          "  0–7 : Сохранение текущего состояния PSW");
  case 1:
    return QStringLiteral("Разряды PSW — заносимая информация:\n"
                          "  0 : Перенос в старший разряд C16\n"
                          "  1 : Сигнал переполнения OVR\n"
                          "  2 : Сигнал нулевого результата Z\n"
                          "  3 : Знак результата F15");
  case 2:
    return QStringLiteral("Разряды PSW — заносимая информация:\n"
                          "  0 : Выталкиваемый при сдвиге разряд\n"
                          "  1 : Сигнал переполнения OVR\n"
                          "  2 : Сигнал нулевого результата Z\n"
                          "  3 : F15 [+] OVR");
  case 3:
    return QStringLiteral("Разряды PSW — заносимая информация:\n"
                          "  0 : Y[0]    4 : Y[4]\n"
                          "  1 : Y[1]    5 : Y[5]\n"
                          "  2 : Y[2]    6 : Y[6]\n"
                          "  3 : Y[3]    7 : Y[7]");
  default:
    return QString();
  }
}

// SWModData5: SWArrayM4 (Команды БМУ К1804ВУ1)
QVector<K1804OptionItem> K1804Microcode::m4Options() {
  struct Entry {
    const char *mnem;
    const char *desc;
  };
  static const Entry entries[16] = {
      {"JZ", "Перейти на нулевой адрес"},
      {"CJS", "Условный переход к ПП по адресу из RMK"},
      {"JMAP", "Переход по адресу из старшего байта RK"},
      {"CJP", "Условный переход по адресу из RMK"},
      {"PUSH", "Загрузка стека и условная загрузка СТ"},
      {"JSRP", "Условный переход к ПП из RA или RMK"},
      {"CJV", "Условный переход из младшего байта RK"},
      {"JRP", "Условный переход на адрес из RA или RMK"},
      {"RFCT", "Повторение цикла, если СТ не равен 0"},
      {"RPCT", "Повторение по RMK, если СТ не равен 0"},
      {"CRTN", "Условный возврат из подпрограммы"},
      {"CJPP", "Условный переход по RMK и прием из стека"},
      {"LDCT", "Загрузка счетчика и выборка"},
      {"LOOP", "Проверка условия окончания цикла"},
      {"CONT", "Последовательная выборка"},
      {"JP", "Безусловный переход по адресу из RMK"}};
  QVector<K1804OptionItem> res;
  res.reserve(16);
  for (int i = 0; i < 16; ++i) {
    res.push_back({i, toOctal(i), QString::fromUtf8(entries[i].mnem),
                   QString::fromUtf8(entries[i].desc)});
  }
  return res;
}

QString K1804Microcode::m4ShortMnemonic(int m4Index) {
  const auto opts = m4Options();
  if (m4Index >= 0 && m4Index < opts.size()) {
    return opts[m4Index].shortText;
  }
  return QStringLiteral("JZ");
}

// SWModData5: SWArrayM2 (Инверсия условия)
QVector<K1804OptionItem> K1804Microcode::m2Options() {
  return {{0, QStringLiteral("0"), QStringLiteral("Прямое"),
           QStringLiteral("Нет инверсии условия")},
          {1, QStringLiteral("1"), QStringLiteral("NOT"),
           QStringLiteral("Инверсия условия")},
          {2, QStringLiteral("2"), QStringLiteral("Прямое (маска)"),
           QStringLiteral("Нет инверсии условия (маска)")},
          {3, QStringLiteral("3"), QStringLiteral("NOT (маска)"),
           QStringLiteral("Инверсия условия (маска)")}};
}

// SWModData5: SWArrayM3 (Условие TST)
QVector<K1804OptionItem> K1804Microcode::m3Options(bool altMode) {
  static const char *stdItems[16] = {
      "PSW[0]",     "PSW[1]",      "PSW[2]", "PSW[3]", "PSW[4]", "PSW[5]",
      "Перенос СТ", "Константа 0", "FLG[0]", "FLG[1]", "FLG[2]", "FLG[3]",
      "FLG[4]",     "FLG[5]",      "FLG[6]", "FLG[7]"};
  static const char *altItems[16] = {
      "PSW[0]",     "PSW[1]",      "PSW[2]",     "PSW[3]",   "PSW[4]", "PSW[5]",
      "Перенос СТ", "Константа 0", "УР1",        "УР2.1",    "УР2.2",  "УР3",
      "УР4",        "УР5",         "RK[11/9]=0", "RK[5/3]=0"};
  const char **src = altMode ? altItems : stdItems;
  QVector<K1804OptionItem> res;
  res.reserve(16);
  for (int i = 0; i < 16; ++i) {
    QString txt = QString::fromUtf8(src[i]);
    res.push_back({i, toOctal(i), txt, txt});
  }
  return res;
}

// SWModData2: SWArrayM7 (Цепь сдвига АЛУ)
QVector<K1804OptionItem> K1804Microcode::m7Options(bool altMode) {
  static const char *stdItems[4] = {
      "'0' --> [R] --> [PSW(0)]  ;  '0' --> [RQ] -->",
      "==> [R] --> [PSW(0)] ==> (цикл)  ;  ==> [RQ] ==> (цикл)",
      "[PSW(0)] --> [R] --> [RQ] --> [PSW(0)]",
      "[(F15[+]OVR)] --> [R]--> [RQ] --> [PSW(0)]"};
  static const char *altItems[4] = {
      "[PSW(0)] <-- [R] <-- '0'  ;  <-- [RQ] <-- '0'",
      "<== [PSW(0)] <-- [R] <== (цикл)  ;  <-- [RQ] <-- (цикл)",
      "[PSW(0)] <-- [R] <-- [RQ] <-- '1'", "[PSW(0)] <-- [R] <-- [RQ] <-- '0'"};
  const char **src = altMode ? altItems : stdItems;
  QVector<K1804OptionItem> res;
  res.reserve(4);
  for (int i = 0; i < 4; ++i) {
    QString txt = QString::fromUtf8(src[i]);
    res.push_back({i, toOctal(i), txt, txt});
  }
  return res;
}

// SWModData2: SWArrayM8 (Входной перенос C0)
QVector<K1804OptionItem> K1804Microcode::m8Options() {
  static const char *items[4] = {"'0'", "'1'", "PSW[0]", "NOT PSW[0]"};
  QVector<K1804OptionItem> res;
  res.reserve(4);
  for (int i = 0; i < 4; ++i) {
    QString txt = QString::fromUtf8(items[i]);
    res.push_back({i, toOctal(i), txt, txt});
  }
  return res;
}

// SWModData2: SWArrayM9 (Операнды R :: S)
QVector<K1804OptionItem> K1804Microcode::m9Options() {
  static const char *items[8] = {"A :: Q", "A :: B", "0 :: Q", "0 :: B",
                                 "0 :: A", "D :: A", "D :: Q", "D :: 0"};
  QVector<K1804OptionItem> res;
  res.reserve(8);
  for (int i = 0; i < 8; ++i) {
    QString txt = QString::fromUtf8(items[i]);
    res.push_back({i, toOctal(i), txt, txt});
  }
  return res;
}

// SWModData2: SWArrayM10 (Функция АЛУ)
QVector<K1804OptionItem> K1804Microcode::m10Options() {
  static const char *items[8] = {
      "R + S + C0", "S - R - 1 + C0", "R - S - 1 + C0", "R v S",
      "R & S",      "NOT(R) & S",     "R [+] S",        "NOT (R [+] S)"};
  QVector<K1804OptionItem> res;
  res.reserve(8);
  for (int i = 0; i < 8; ++i) {
    QString txt = QString::fromUtf8(items[i]);
    res.push_back({i, toOctal(i), txt, txt});
  }
  return res;
}

// SWModData2: SWArrayM11 (Приемник результата и сдвиг)
QVector<K1804M11Item> K1804Microcode::m11Options() {
  return {
      {0, QStringLiteral("0"), QStringLiteral("-"), QStringLiteral("-"),
       QStringLiteral("-"), QStringLiteral("F -> Q"), QStringLiteral("F")},
      {1, QStringLiteral("1"), QStringLiteral("-"), QStringLiteral("-"),
       QStringLiteral("-"), QStringLiteral("-"), QStringLiteral("F")},
      {2, QStringLiteral("2"), QStringLiteral("-"), QStringLiteral("F -> B"),
       QStringLiteral("-"), QStringLiteral("-"), QStringLiteral("A")},
      {3, QStringLiteral("3"), QStringLiteral("-"), QStringLiteral("F -> B"),
       QStringLiteral("-"), QStringLiteral("-"), QStringLiteral("F")},
      {4, QStringLiteral("4"), QStringLiteral("-->"),
       QStringLiteral("F/2 -> B"), QStringLiteral("-->"),
       QStringLiteral("Q/2 -> Q"), QStringLiteral("F")},
      {5, QStringLiteral("5"), QStringLiteral("-->"),
       QStringLiteral("F/2 -> B"), QStringLiteral("-"), QStringLiteral("-"),
       QStringLiteral("F")},
      {6, QStringLiteral("6"), QStringLiteral("<--"),
       QStringLiteral("2*F -> B"), QStringLiteral("<--"),
       QStringLiteral("2*Q -> Q"), QStringLiteral("F")},
      {7, QStringLiteral("7"), QStringLiteral("<--"),
       QStringLiteral("2*F -> B"), QStringLiteral("-"), QStringLiteral("-"),
       QStringLiteral("F")}};
}

// Точный порт функции SWShapeText из BlockParams.bas
std::array<QString, 6>
K1804Microcode::generateBlockLines(const std::array<quint16, 16> &mFields) {
  std::array<QString, 6> lines;

  const quint16 m1 = mFields[1];
  const int m2 = mFields[2] & 0x03;
  const int m3 = mFields[3] & 0x0F;
  const int m4 = mFields[4] & 0x0F;
  const int m5 = mFields[5] & 0x1F;
  const int m6 = mFields[6] & 0x1F;
  const int m9 = mFields[9] & 0x07;
  const int m10 = mFields[10] & 0x07;
  const int m11 = mFields[11] & 0x07;
  const int m12 = mFields[12] & 0x07;
  const int m13 = mFields[13] & 0x03;
  const int m14 = mFields[14] & 0x03;
  const quint16 m15 = mFields[15] & 0xFF;

  // Строка 1: Мультиплексор шины D (M12)
  const auto m12List = m12Options();
  lines[0] = (m12 >= 0 && m12 < m12List.size()) ? m12List[m12].shortText
                                                : QStringLiteral("RDI --> D");

  // Строка 2: Операция АЛУ К1804ВС1 (M5, M6, M9, M10, M11)
  auto formatPortReg = [](int mVal) -> QString {
    // В оригинале BlockParams.bas:
    // 00..17 oct (0..15 dec) -> OptionButton1 ("R " + номер)
    // 18..27 oct (16..23 dec) -> OptionButton2 ("RK[2/0]")
    // 28..37 oct (24..31 dec) -> OptionButton3 ("RK[8/6]")
    if (mVal >= 24) {
      return QStringLiteral("RK[8/6]");
    } else if (mVal >= 16) {
      return QStringLiteral("RK[2/0]");
    } else {
      return QStringLiteral("R %1").arg(mVal & 0x0F);
    }
  };

  const QString str2A = formatPortReg(m5);
  const QString str2B = formatPortReg(m6);

  QString str2R, str2S;
  switch (m9) {
  case 0:
    str2R = str2A;
    str2S = QStringLiteral("Q");
    break;
  case 1:
    str2R = str2A;
    str2S = str2B;
    break;
  case 2:
    str2R = QStringLiteral("0");
    str2S = QStringLiteral("Q");
    break;
  case 3:
    str2R = QStringLiteral("0");
    str2S = str2B;
    break;
  case 4:
    str2R = QStringLiteral("0");
    str2S = str2A;
    break;
  case 5:
    str2R = QStringLiteral("D");
    str2S = str2A;
    break;
  case 6:
    str2R = QStringLiteral("D");
    str2S = QStringLiteral("Q");
    break;
  case 7:
    str2R = QStringLiteral("D");
    str2S = QStringLiteral("0");
    break;
  default:
    str2R = str2A;
    str2S = QStringLiteral("Q");
    break;
  }

  auto buildAluCore = [&](int op) -> QString {
    switch (op) {
    case 0:
      return str2R + QStringLiteral(" + ") + str2S;
    case 1:
      return str2S + QStringLiteral(" - ") + str2R + QStringLiteral(" - 1");
    case 2:
      return str2R + QStringLiteral(" - ") + str2S + QStringLiteral(" - 1");
    case 3:
      return str2R + QStringLiteral(" V ") + str2S;
    case 4:
      return str2R + QStringLiteral(" & ") + str2S;
    case 5:
      return QStringLiteral("NOT(") + str2R + QStringLiteral(") & ") + str2S;
    case 6:
      return str2R + QStringLiteral(" [+] ") + str2S;
    case 7:
      return QStringLiteral("NOT(") + str2R + QStringLiteral(" [+] ") + str2S +
             QStringLiteral(")");
    default:
      return str2R + QStringLiteral(" + ") + str2S;
    }
  };

  QString str2;
  if (m11 == 0) {
    str2 = QStringLiteral("NOP");
  } else if (m11 == 1) {
    str2 = buildAluCore(m10);
  } else if (m11 == 4 || m11 == 5) {
    if (m10 == 1) {
      str2 = str2B + QStringLiteral(" <--( ") + str2S + QStringLiteral(" - ") +
             str2R + QStringLiteral(" - 1/2)");
    } else {
      str2 = str2B + QStringLiteral(" <--( ") + buildAluCore(m10) +
             QStringLiteral(")/2");
    }
  } else if (m11 == 6 || m11 == 7) {
    str2 = str2B + QStringLiteral(" <--2*( ") + buildAluCore(m10) +
           QStringLiteral(")");
  } else {
    str2 = str2B + QStringLiteral(" <-- ") + buildAluCore(m10);
  }
  lines[1] = str2;

  // Строка 3: Загрузка PSW (M13)
  switch (m13) {
  case 0:
    lines[2] = QStringLiteral("PSW <-- PSW");
    break;
  case 1:
    lines[2] = QStringLiteral("PSW <-- C16, OVR, Z, F15");
    break;
  case 2:
    lines[2] = QStringLiteral("PSW <-- OVR, Z, F15 [+] OVR");
    break;
  case 3:
    lines[2] = QStringLiteral("PSW <-- Y");
    break;
  default:
    lines[2] = QStringLiteral("PSW <-- PSW");
    break;
  }

  // Строка 4: Выходная шина / управление (M14)
  const auto m14List = m14Options();
  QString m14Text = (m14 >= 0 && m14 < m14List.size()) ? m14List[m14].shortText
                                                       : QStringLiteral("null");
  if (m14Text.compare(QLatin1String("null"), Qt::CaseInsensitive) == 0) {
    lines[3] = QStringLiteral("NOP");
  } else {
    lines[3] = m14Text;
  }

  // Строка 5: Команда перехода БМУ (M4) и условие TST (M2, M3)
  const bool altM3 = (m2 == 2 || m2 == 3);
  const auto m3List = m3Options(altM3);
  const QString m3Text = (m3 >= 0 && m3 < m3List.size())
                             ? m3List[m3].shortText
                             : QStringLiteral("PSW[0]");
  const QString notPrefix =
      (m2 == 1 || m2 == 3) ? QStringLiteral("NOT ") : QString();
  lines[4] =
      m4ShortMnemonic(m4) + QStringLiteral("; TST = ") + notPrefix + m3Text;

  // Строка 6: Константы M1 и M15 (в восьмеричном формате, как в Visio)
  lines[5] =
      QStringLiteral("M1: %1; M15: %2").arg(toOctal(m1)).arg(toOctal(m15));

  return lines;
}

std::array<quint16, 5>
K1804Microcode::packBinaryWords(quint16 address,
                                const std::array<quint16, 16> &mFields) {
  std::array<quint16, 5> words{};

  // Слово 0: Адрес микрокоманды
  words[0] = address;

  // Слово 1: M1 (16 бит)
  words[1] = mFields[1];

  // Слово 2: (M6 << 11) | (M5 << 6) | (M3 << 2) | M2
  words[2] = static_cast<quint16>(
      ((mFields[6] & 0x1F) << 11) | ((mFields[5] & 0x1F) << 6) |
      ((mFields[3] & 0x0F) << 2) | (mFields[2] & 0x03));

  // Слово 3: (M13 << 14) | (M10 << 11) | (M9 << 8) | (M8 << 6) | (M7 << 4) | M4
  words[3] = static_cast<quint16>(
      ((mFields[13] & 0x03) << 14) | ((mFields[10] & 0x07) << 11) |
      ((mFields[9] & 0x07) << 8) | ((mFields[8] & 0x03) << 6) |
      ((mFields[7] & 0x03) << 4) | (mFields[4] & 0x0F));

  // Слово 4: (M15 << 8) | (M14 << 6) | (M12 << 3) | M11
  words[4] = static_cast<quint16>(
      ((mFields[15] & 0xFF) << 8) | ((mFields[14] & 0x03) << 6) |
      ((mFields[12] & 0x07) << 3) | (mFields[11] & 0x07));

  return words;
}

} // namespace editor
