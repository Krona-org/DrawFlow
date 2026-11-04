#pragma once

#include "src/elements/shapeElement/shapeelement.h"

#include <QDockWidget>

class QTreeWidgetItem;

QT_BEGIN_NAMESPACE
namespace Ui { class WindowElementsFrame; }
QT_END_NAMESPACE

namespace editor {

class CanvasScene;

class WindowElementsFrame : public QDockWidget {
    Q_OBJECT

public:
    explicit WindowElementsFrame(CanvasScene *scene, QWidget *parent = nullptr);
    ~WindowElementsFrame() override;

    void setScene(CanvasScene *scene);

public slots:
    void refreshElementsList();
    void onSceneSelectionChanged();

private slots:
    void onItemSelectionChanged();
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemChanged(QTreeWidgetItem *item, int column);

private:
    Ui::WindowElementsFrame *ui = nullptr;
    CanvasScene *scene_ = nullptr;
    bool isUpdatingSelection_ = false;
};

} // namespace editor
