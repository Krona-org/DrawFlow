#pragma once

#include "canvasscene.h"

#include <QGraphicsView>

namespace editor {

class CanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CanvasView(CanvasScene *scene, QWidget *parent = nullptr);
    ~CanvasView() override = default;

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setCanvasScene(CanvasScene *scene);

signals:
    void zoomChanged(double factor);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void applyZoom(double factor);

    CanvasScene *scene_ = nullptr;
    bool isPanning_ = false;
    QPoint panStartPos_;
};

} // namespace editor
