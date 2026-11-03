#pragma once

#include "src/canvas/canvastabwidget.h"
#include "src/frames/elementpropertyframe.h"
#include "src/frames/elementslibraryframe.h"
#include "src/frames/windowelementsframe.h"

#include <QMainWindow>

class QActionGroup;
class QComboBox;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace editor {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onGridSettings();
    void onAbout();
    void onToolChanged(ElementType tool);

private:
    void setupUiConnections();
    void createDockWindows();
    QWidget* createCustomDockTitleBar(const QString &title, QDockWidget *dock);
    void initSampleElements();
    void applyStrictMinimalistTheme();
    void updateWindowTitle();

    Ui::MainWindow *ui = nullptr;

    QString projectName_{QStringLiteral("Схема 1")};
    QString currentFilePath_;

    CanvasScene *scene_ = nullptr;
    CanvasView *view_ = nullptr;
    CanvasTabWidget *tabWidget_ = nullptr;

    ElementsLibraryFrame *libraryDock_ = nullptr;
    WindowElementsFrame *elementsDock_ = nullptr;
    ElementPropertyFrame *propertyDock_ = nullptr;

    QActionGroup *toolActionGroup_ = nullptr;
    QComboBox *comboGridStep_ = nullptr;
};

} // namespace editor
