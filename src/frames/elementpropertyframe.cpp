#include "elementpropertyframe.h"
#include "ui_elementpropertyframe.h"
#include "src/canvas/canvasscene.h"

namespace editor {

ElementPropertyFrame::ElementPropertyFrame(CanvasScene *scene, QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::ElementPropertyFrame)
    , scene_(scene)
{
    ui->setupUi(this);
    if (scene_) {
        connect(scene_, &CanvasScene::sceneSelectionChanged, this, &ElementPropertyFrame::updateProperties);
    }
}

ElementPropertyFrame::~ElementPropertyFrame() {
    delete ui;
}

void ElementPropertyFrame::setScene(CanvasScene *scene) {
    if (scene_ != scene) {
        if (scene_) {
            disconnect(scene_, nullptr, this, nullptr);
        }
        scene_ = scene;
        if (scene_) {
            connect(scene_, &CanvasScene::sceneSelectionChanged, this, &ElementPropertyFrame::updateProperties);
        }
    }
}

void ElementPropertyFrame::updateProperties() {
    // Свойства элементов временно скрыты по запросу пользователя для последующего редизайна
}

} // namespace editor
