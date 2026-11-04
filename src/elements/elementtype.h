#pragma once

#include <QString>

namespace editor {

enum class ElementType {
    None = 0,
    MasterBlock,
    Connector
};

inline QString elementTypeName(ElementType type) {
    switch (type) {
    case ElementType::MasterBlock:
        return QStringLiteral("K1804 MasterBlock");
    case ElementType::Connector:
        return QStringLiteral("Connector");
    default:
        return QStringLiteral("Неизвестно");
    }
}

} // namespace editor
