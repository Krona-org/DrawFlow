#pragma once

#include <QString>

namespace editor {

enum class PortPosition {
    None = 0,
    InTop,
    InMid,
    InBot,
    OutTop,
    OutMid,
    OutBot
};

inline bool isInputPort(PortPosition port) {
    return port == PortPosition::InTop ||
           port == PortPosition::InMid ||
           port == PortPosition::InBot;
}

inline bool isOutputPort(PortPosition port) {
    return port == PortPosition::OutTop ||
           port == PortPosition::OutMid ||
           port == PortPosition::OutBot;
}

inline QString portPositionName(PortPosition port) {
    switch (port) {
    case PortPosition::InTop:
        return QStringLiteral("Вход (верх)");
    case PortPosition::InMid:
        return QStringLiteral("Вход (центр)");
    case PortPosition::InBot:
        return QStringLiteral("Вход (низ)");
    case PortPosition::OutTop:
        return QStringLiteral("Выход (верх)");
    case PortPosition::OutMid:
        return QStringLiteral("Выход (центр)");
    case PortPosition::OutBot:
        return QStringLiteral("Выход (низ)");
    default:
        return QStringLiteral("Нет");
    }
}

} // namespace editor

