#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <array>

namespace editor {

struct K1804OptionItem {
    int code = 0;          // Числовое значение (индекс)
    QString octalCode;     // Восьмеричное представление ("00", "01", ...)
    QString shortText;     // Краткая мнемоника
    QString description;   // Полное описание из SWModData
};

struct K1804M11Item {
    int code = 0;
    QString octalCode;
    QString shiftRon;
    QString writeRon;
    QString shiftRq;
    QString writeRq;
    QString outputY;
};

class K1804Microcode {
public:
    // Вспомогательное форматирование в восьмеричную систему
    static QString toOctal(quint16 val, int minWidth = 1);
    static quint16 fromOctal(const QString &octStr, bool *ok = nullptr);

    // Таблицы справочников из SWModData1..SWModData5
    static QVector<K1804OptionItem> m12Options(); // Шина D (0..7)
    static QVector<K1804OptionItem> m14Options(); // Выходная шина / управление (0..3)
    static QVector<K1804OptionItem> m13Options(); // Загрузка PSW (0..3)
    static QString m13HelpText(int m13Index);     // Справка по разрядам PSW (0..3)

    static QVector<K1804OptionItem> m4Options();  // Команды БМУ К1804ВУ1 (0..15)
    static QString m4ShortMnemonic(int m4Index);  // Краткое имя команды перехода (JZ, CJS...)
    static QVector<K1804OptionItem> m2Options();  // Инверсия / маска условия (0..3)
    static QVector<K1804OptionItem> m3Options(bool altMode); // Условие TST (0..15)

    static QVector<K1804OptionItem> m7Options(bool altMode); // Сдвиг АЛУ (0..3)
    static QVector<K1804OptionItem> m8Options();  // Входной перенос C0 (0..3)
    static QVector<K1804OptionItem> m9Options();  // Источники операндов R :: S (0..7)
    static QVector<K1804OptionItem> m10Options(); // Функция АЛУ (0..7)
    static QVector<K1804M11Item> m11Options();    // Приемник результата и сдвиг АЛУ (0..7)

    // Генерация 6 строк текста внутри фигуры (точный аналог SWShapeText из BlockParams.bas)
    static std::array<QString, 6> generateBlockLines(const std::array<quint16, 16> &mFields);

    // Упаковка 5x16-битных слов микрокоманды для бинарного экспорта (SWDataExport)
    static std::array<quint16, 5> packBinaryWords(quint16 address, const std::array<quint16, 16> &mFields);
};

} // namespace editor

