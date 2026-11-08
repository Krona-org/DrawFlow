#pragma once

#include "src/canvas/canvasscene.h"

#include <QList>
#include <QString>

namespace editor {

struct PageInfo {
    QString name;
    CanvasScene *scene = nullptr;
};

class ProjectStorage {
public:
    // Загруженные сцены принадлежат sceneParent; при nullptr — вызывающему коду.
    static bool saveToFile(const QString &filePath, const QString &projectName, const QList<PageInfo> &pages, QString *errorMessage = nullptr);
    static bool loadFromFile(const QString &filePath, QString &projectName, QList<PageInfo> &pages, QString *errorMessage = nullptr, QObject *sceneParent = nullptr);
    static bool saveToFile(const QString &filePath, const QString &projectName, CanvasScene *scene, QString *errorMessage = nullptr);
    static bool loadFromFile(const QString &filePath, QString &projectName, CanvasScene *scene, QString *errorMessage = nullptr);

    // Бинарный экспорт микрокоманд в формате K1804 (SWDataExport)
    static bool exportK1804Binary(const QString &filePath, CanvasScene *scene, int *exportedCount = nullptr, QString *errorMessage = nullptr);
};

} // namespace editor
