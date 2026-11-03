#pragma once

#include "src/elements/shapeelement.h"

#include <QDockWidget>

class QPushButton;

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

public slots:
    void updateProperties();

private slots:
    void onNameChanged(const QString &text);
    void onGeometryChanged();
    void onBorderColorClicked();
    void onBorderWidthChanged(double val);
    void onBorderStyleChanged(int index);
    void onFillColorClicked();
    void onFillStyleChanged(int index);
    void onRoundingChanged(double val);
    void onMarkerChanged();

private:
    void setupUiConnections();
    void updateColorButton(QPushButton *btn, const QColor &color);

    Ui::ElementPropertyFrame *ui = nullptr;
    CanvasScene *scene_ = nullptr;
    ShapeElement *currentElement_ = nullptr;
    bool isUpdating_ = false;
};

} // namespace editor
