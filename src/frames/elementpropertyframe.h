#pragma once

#include <QDockWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class ElementPropertyFrame; }
QT_END_NAMESPACE

namespace editor {

class CanvasScene;

class ElementPropertyFrame : public QDockWidget {
    Q_OBJECT

public:
    explicit ElementPropertyFrame(CanvasScene *scene, QWidget *parent = nullptr);
    ~ElementPropertyFrame() override;

    void setScene(CanvasScene *scene);

public slots:
    void updateProperties();

private:
    Ui::ElementPropertyFrame *ui = nullptr;
    CanvasScene *scene_ = nullptr;
};

} // namespace editor
