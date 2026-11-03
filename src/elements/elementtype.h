#pragma once

#include <QString>

namespace editor {

enum class ElementType {
    None = 0,
    Rectangle,
    Line
};

inline QString elementTypeName(ElementType type) {
    switch (type) {
    case ElementType::Rectangle:
        return QStringLiteral("Прямоугольник");
    case ElementType::Line:
        return QStringLiteral("Линия");
    default:
        return QStringLiteral("Неизвестно");
    }
}

} // namespace editor

