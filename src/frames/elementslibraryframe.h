#pragma once

#include "src/elements/elementtype.h"

#include <QDockWidget>
#include <QFrame>

class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui { class ElementsLibraryFrame; }
QT_END_NAMESPACE

namespace editor {

class CanvasScene;

class ShapeCard : public QFrame {
    Q_OBJECT

public:
    ShapeCard(ElementType type, const QString &title, const QString &iconPath, QWidget *parent = nullptr);

    ElementType elementType() const { return type_; }
    void setActive(bool active);

signals:
    void clicked(ElementType type);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    ElementType type_;
    QString title_;
    QPoint dragStartPos_;
    bool isActive_ = false;
    QLabel *iconLabel_ = nullptr;
    QLabel *textLabel_ = nullptr;
};

class ElementsLibraryFrame : public QDockWidget {
    Q_OBJECT

public:
    explicit ElementsLibraryFrame(CanvasScene *scene, QWidget *parent = nullptr);
    ~ElementsLibraryFrame() override;

    void setScene(CanvasScene *scene);

public slots:
    void onToolChanged(ElementType tool);

private:
    Ui::ElementsLibraryFrame *ui = nullptr;
    CanvasScene *scene_ = nullptr;
    ShapeCard *masterBlockCard_ = nullptr;
    ShapeCard *connectorCard_ = nullptr;
};

} // namespace editor
