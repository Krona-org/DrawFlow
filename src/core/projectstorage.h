#pragma once

#include "src/canvas/canvasscene.h"

#include <QString>

namespace editor {

class ProjectStorage {
public:
    static bool saveToFile(const QString &filePath, const QString &projectName, CanvasScene *scene, QString *errorMessage = nullptr);
    static bool loadFromFile(const QString &filePath, QString &projectName, CanvasScene *scene, QString *errorMessage = nullptr);

    // Бинарный экспорт микрокоманд в формате K1804 (SWDataExport)
    static bool exportK1804Binary(const QString &filePath, CanvasScene *scene, int *exportedCount = nullptr, QString *errorMessage = nullptr);
};

} // namespace editor
