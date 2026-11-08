#include "mainwindow.h"
#include "projectstorage.h"
#include "src/canvas/canvasview.h"
#include "src/elements/connectorElement/connectorelement.h"
#include "src/elements/masterBlockElement/masterblockelement.h"
#include "src/frames/bParamsDialog/data/helpdialog.h"
#include "src/frames/exportDialog/exportbinarydialog.h"
#include "src/themes/thememanager.h"
#include "ui_mainwindow.h"

#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QEnterEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QToolButton>

namespace editor {

namespace {

class DockCloseButton : public QToolButton {
public:
  explicit DockCloseButton(QWidget *parent = nullptr) : QToolButton(parent) {
    setObjectName(QStringLiteral("dockCloseBtn"));
    setFixedSize(20, 20);
    setIcon(QIcon(QStringLiteral(":/dock-close.svg")));
    setIconSize(QSize(12, 12));
    setAutoRaise(true);
    setCursor(Qt::ArrowCursor);
    setToolTip(QStringLiteral("Закрыть панель"));
  }

protected:
  void enterEvent(QEnterEvent *event) override {
    setIcon(QIcon(QStringLiteral(":/dock-close-hover.svg")));
    QToolButton::enterEvent(event);
  }

  void leaveEvent(QEvent *event) override {
    setIcon(QIcon(QStringLiteral(":/dock-close.svg")));
    QToolButton::leaveEvent(event);
  }
};

class DockCollapseButton : public QToolButton {
public:
  explicit DockCollapseButton(QDockWidget *dock, QWidget *parent = nullptr)
      : QToolButton(parent), dock_(dock) {
    setObjectName(QStringLiteral("dockCollapseBtn"));
    setFixedSize(20, 20);
    setIcon(QIcon(QStringLiteral(":/dock-collapse.svg")));
    setIconSize(QSize(12, 12));
    setAutoRaise(true);
    setCursor(Qt::ArrowCursor);
    setToolTip(QStringLiteral("Свернуть панель"));
    connect(this, &QToolButton::clicked, this, &DockCollapseButton::onToggle);
  }

private:
  void onToggle() {
    if (!dock_ || !dock_->widget())
      return;
    bool isCurrentlyVisible = dock_->widget()->isVisible();
    dock_->widget()->setVisible(!isCurrentlyVisible);
    if (isCurrentlyVisible) {
      setIcon(QIcon(QStringLiteral(":/dock-expand.svg")));
      setToolTip(QStringLiteral("Развернуть панель"));
    } else {
      setIcon(QIcon(QStringLiteral(":/dock-collapse.svg")));
      setToolTip(QStringLiteral("Свернуть панель"));
    }
  }
  QDockWidget *dock_;
};

} // anonymous namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowIcon(QIcon(QStringLiteral(":/app_icon.png")));

  // Полностью убираем статус-бар, чтобы внизу не было лишней высокой полосы
  setStatusBar(nullptr);

  // Центральный холст
  scene_ = new CanvasScene(this);
  pages_.append({QStringLiteral("Страница 1"), scene_});
  view_ = new CanvasView(scene_, this);
  tabWidget_ = new CanvasTabWidget(view_, this);
  ui->centralLayout->addWidget(tabWidget_);

  // Док-панели с кастомными современными заголовками
  createDockWindows();

  // Сигналы и действия из ui
  setupUiConnections();
  connectScene(scene_);

  // Применение темы из внешнего QSS-файла (src/themes/light.qss)
  applyStrictMinimalistTheme();

  updateWindowTitle();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::connectScene(CanvasScene *scene) {
  if (!scene)
    return;
  connect(scene, &CanvasScene::activeToolChanged, this,
          &MainWindow::onToolChanged);
  connect(scene, &CanvasScene::gridSizeChanged, this, [this](qreal size) {
    if (comboGridStep_) {
      int idx = comboGridStep_->findData(size);
      if (idx >= 0)
        comboGridStep_->setCurrentIndex(idx);
    }
  });

  if (auto stack = scene->undoStack()) {
    connect(stack, &QUndoStack::canUndoChanged, ui->actionUndo,
            &QAction::setEnabled);
    connect(stack, &QUndoStack::canRedoChanged, ui->actionRedo,
            &QAction::setEnabled);
    ui->actionUndo->setEnabled(stack->canUndo());
    ui->actionRedo->setEnabled(stack->canRedo());
  } else {
    ui->actionUndo->setEnabled(false);
    ui->actionRedo->setEnabled(false);
  }
}

void MainWindow::onPageChanged(int index) {
  if (index < 0 || index >= pages_.size() || index == activePageIndex_)
    return;

  if (scene_) {
    disconnect(scene_, nullptr, this, nullptr);
    if (scene_->undoStack()) {
      disconnect(scene_->undoStack(), nullptr, ui->actionUndo, nullptr);
      disconnect(scene_->undoStack(), nullptr, ui->actionRedo, nullptr);
    }
  }

  activePageIndex_ = index;
  scene_ = pages_[index].scene;

  view_->setCanvasScene(scene_);
  connectScene(scene_);

  if (elementsDock_)
    elementsDock_->setScene(scene_);
  if (libraryDock_)
    libraryDock_->setScene(scene_);
  if (propertyDock_)
    propertyDock_->setScene(scene_);

  if (ui->actionGrid)
    ui->actionGrid->setChecked(scene_->isGridVisible());
  if (ui->actionShowGrid)
    ui->actionShowGrid->setChecked(scene_->isGridVisible());
  if (ui->actionSnap)
    ui->actionSnap->setChecked(scene_->isSnapToGrid());
  if (ui->actionSnapToGrid)
    ui->actionSnapToGrid->setChecked(scene_->isSnapToGrid());
  if (comboGridStep_) {
    int idx = comboGridStep_->findData(scene_->gridSize());
    if (idx >= 0)
      comboGridStep_->setCurrentIndex(idx);
  }
  view_->update();
}

void MainWindow::onAddPage() {
  auto newScene = new CanvasScene(this);
  if (scene_) {
    newScene->setGridSize(scene_->gridSize());
    newScene->setGridVisible(scene_->isGridVisible());
    newScene->setSnapToGrid(scene_->isSnapToGrid());
    newScene->setPageFormat(scene_->pageFormat());
  }

  int pageNum = pages_.size() + 1;
  pages_.append({QStringLiteral("Страница %1").arg(pageNum), newScene});

  onPageChanged(pages_.size() - 1);
}

void MainWindow::updateWindowTitle() {
  QString title = QStringLiteral("DrawFlow — %1").arg(projectName_);
  if (!currentFilePath_.isEmpty()) {
    title += QStringLiteral(" [%1]").arg(currentFilePath_);
  }
  setWindowTitle(title);
}

void MainWindow::applyStrictMinimalistTheme() {
  ThemeManager::apply(this, Theme::Light);
}

void MainWindow::setupUiConnections() {
  ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

  // Файл
  connect(ui->actionNew, &QAction::triggered, this, &MainWindow::onNewProject);
  connect(ui->actionOpen, &QAction::triggered, this,
          &MainWindow::onOpenProject);
  connect(ui->actionSave, &QAction::triggered, this,
          &MainWindow::onSaveProject);
  connect(ui->actionSaveAs, &QAction::triggered, this,
          &MainWindow::onSaveProjectAs);

  QAction *actionExport =
      new QAction(QStringLiteral("Экспорт микрокоманд К1804 (*.bin)..."), this);
  actionExport->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
  ui->menuFile->insertAction(ui->actionExit, actionExport);
  ui->menuFile->insertSeparator(ui->actionExit);
  connect(actionExport, &QAction::triggered, this, &MainWindow::onExportBinary);

  connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

  // Страницы
  connect(tabWidget_, &CanvasTabWidget::pageAdded, this,
          &MainWindow::onAddPage);
  connect(tabWidget_, &CanvasTabWidget::pageChanged, this,
          &MainWindow::onPageChanged);

  // Правка
  connect(ui->actionUndo, &QAction::triggered, this, [this]() {
    if (scene_ && scene_->undoStack())
      scene_->undoStack()->undo();
  });
  connect(ui->actionRedo, &QAction::triggered, this, [this]() {
    if (scene_ && scene_->undoStack())
      scene_->undoStack()->redo();
  });
  connect(ui->actionCut, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->cutSelection();
  });
  connect(ui->actionCopy, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->copySelection();
  });
  connect(ui->actionPaste, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->pasteClipboard();
  });
  connect(ui->actionDuplicate, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->duplicateSelection();
  });
  connect(ui->actionSelectAll, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->selectAllElements();
  });
  connect(ui->actionDelete, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->deleteSelectedElements();
  });

  // Выбор
  toolActionGroup_ = new QActionGroup(this);
  toolActionGroup_->setExclusive(true);
  toolActionGroup_->addAction(ui->actionSelect);
  ui->actionSelect->setChecked(true);
  connect(ui->actionSelect, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->setActiveTool(ElementType::None);
  });

  // Сетка и привязка
  ui->actionGrid->setChecked(scene_->isGridVisible());
  ui->actionShowGrid->setChecked(scene_->isGridVisible());
  connect(ui->actionGrid, &QAction::toggled, this, [this](bool v) {
    if (scene_)
      scene_->setGridVisible(v);
  });
  connect(ui->actionShowGrid, &QAction::toggled, ui->actionGrid,
          &QAction::setChecked);
  connect(ui->actionGrid, &QAction::toggled, ui->actionShowGrid,
          &QAction::setChecked);

  ui->actionSnap->setChecked(scene_->isSnapToGrid());
  ui->actionSnapToGrid->setChecked(scene_->isSnapToGrid());
  connect(ui->actionSnap, &QAction::toggled, this, [this](bool v) {
    if (scene_)
      scene_->setSnapToGrid(v);
  });
  connect(ui->actionSnapToGrid, &QAction::toggled, ui->actionSnap,
          &QAction::setChecked);
  connect(ui->actionSnap, &QAction::toggled, ui->actionSnapToGrid,
          &QAction::setChecked);

  // Выбор шага сетки в тулбаре прямо возле сетки
  comboGridStep_ = new QComboBox(ui->mainToolBar);
  comboGridStep_->setObjectName(QStringLiteral("comboGridStep"));
  comboGridStep_->setToolTip(QStringLiteral("Шаг сетки (px)"));
  comboGridStep_->addItem(QStringLiteral("10 px"), 10.0);
  comboGridStep_->addItem(QStringLiteral("20 px"), 20.0);
  comboGridStep_->addItem(QStringLiteral("50 px"), 50.0);
  comboGridStep_->setCurrentIndex(1); // 20 px по умолчанию
  connect(comboGridStep_, &QComboBox::currentIndexChanged, this,
          [this](int idx) {
            if (scene_ && comboGridStep_) {
              qreal step = comboGridStep_->itemData(idx).toDouble();
              scene_->setGridSize(step);
            }
          });
  ui->mainToolBar->insertWidget(ui->actionSnap, comboGridStep_);

  // Шаг сетки в меню
  connect(ui->actionGridStep10, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->setGridSize(10.0);
  });
  connect(ui->actionGridStep20, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->setGridSize(20.0);
  });
  connect(ui->actionGridStep50, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->setGridSize(50.0);
  });
  connect(ui->actionGridSettings, &QAction::triggered, this,
          &MainWindow::onGridSettings);

  // Формат листа
  connect(ui->actionPageA4Landscape, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->setPageFormat(PageFormat::A4Landscape);
  });
  connect(ui->actionPageA4Portrait, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->setPageFormat(PageFormat::A4Portrait);
  });
  connect(ui->actionPageInfinite, &QAction::triggered, this, [this]() {
    if (scene_)
      scene_->setPageFormat(PageFormat::Infinite);
  });

  // Масштаб
  connect(ui->actionZoomIn, &QAction::triggered, view_, &CanvasView::zoomIn);
  connect(ui->actionZoomOut, &QAction::triggered, view_, &CanvasView::zoomOut);
  connect(ui->actionZoomReset, &QAction::triggered, view_,
          &CanvasView::resetZoom);
  connect(ui->actionZoom100, &QAction::triggered, view_,
          &CanvasView::resetZoom);
  connect(view_, &CanvasView::zoomChanged, this, [this](double factor) {
    if (tabWidget_) {
      tabWidget_->setZoomText(QStringLiteral("%1%").arg(qRound(factor * 100)));
    }
  });

  // Диалоги
  auto *actionHelp = new QAction(QStringLiteral("Справочник микрокоманд К1804..."), this);
  actionHelp->setShortcut(QKeySequence(QStringLiteral("F1")));
  ui->menuHelp->insertAction(ui->actionAbout, actionHelp);
  ui->menuHelp->insertSeparator(ui->actionAbout);
  connect(actionHelp, &QAction::triggered, this, &MainWindow::onHelpContents);

  connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::onToolChanged(ElementType tool) {
  if (tool == ElementType::None && ui->actionSelect) {
    ui->actionSelect->setChecked(true);
  }
}

QWidget *MainWindow::createCustomDockTitleBar(const QString &title,
                                              QDockWidget *dock) {
  QWidget *titleBar = new QWidget(dock);
  titleBar->setObjectName(QStringLiteral("dockCustomTitleBar"));
  titleBar->setFixedHeight(26);

  QHBoxLayout *layout = new QHBoxLayout(titleBar);
  layout->setContentsMargins(8, 0, 4, 0);
  layout->setSpacing(2);

  QLabel *label = new QLabel(title, titleBar);
  label->setObjectName(QStringLiteral("dockTitleLabel"));
  layout->addWidget(label);
  layout->addStretch();

  // Кнопка сворачивания/разворачивания содержимого панели
  auto btnCollapse = new DockCollapseButton(dock, titleBar);
  layout->addWidget(btnCollapse);

  // Кнопка открепления (float)
  auto btnFloat = new QToolButton(titleBar);
  btnFloat->setObjectName(QStringLiteral("dockFloatBtn"));
  btnFloat->setFixedSize(20, 20);
  btnFloat->setIcon(QIcon(QStringLiteral(":/dock-float.svg")));
  btnFloat->setIconSize(QSize(12, 12));
  btnFloat->setAutoRaise(true);
  btnFloat->setToolTip(QStringLiteral("Открепить окно"));
  connect(btnFloat, &QToolButton::clicked, dock,
          [dock]() { dock->setFloating(!dock->isFloating()); });
  layout->addWidget(btnFloat);

  // Кнопка закрытия панели (аккуратный крестик, с яркой красной подсветкой при
  // наведении)
  auto btnClose = new DockCloseButton(titleBar);
  connect(btnClose, &QToolButton::clicked, dock, &QDockWidget::close);
  layout->addWidget(btnClose);

  return titleBar;
}

void MainWindow::createDockWindows() {
  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // 1. Библиотека элементов (слева)
  libraryDock_ = new ElementsLibraryFrame(scene_, this);
  libraryDock_->setMinimumWidth(120); // минимальны емаксимальные размеры фрейма
  libraryDock_->setMaximumWidth(140);
  libraryDock_->setTitleBarWidget(
      createCustomDockTitleBar(QStringLiteral("Фигуры"), libraryDock_));
  addDockWidget(Qt::LeftDockWidgetArea, libraryDock_);

  // 2. Дерево объектов (справа сверху)
  elementsDock_ = new WindowElementsFrame(scene_, this);
  elementsDock_->setMinimumWidth(230);
  elementsDock_->setMaximumWidth(300);
  elementsDock_->setTitleBarWidget(createCustomDockTitleBar(
      QStringLiteral("Дерево объектов"), elementsDock_));
  addDockWidget(Qt::RightDockWidgetArea, elementsDock_);

  // 3. Свойства (справа снизу)
  propertyDock_ = new ElementPropertyFrame(scene_, this);
  propertyDock_->setMinimumWidth(230);
  propertyDock_->setMaximumWidth(300);
  propertyDock_->setTitleBarWidget(
      createCustomDockTitleBar(QStringLiteral("Свойства"), propertyDock_));
  addDockWidget(Qt::RightDockWidgetArea, propertyDock_);

  // Разделяем правую часть по вертикали на две аккуратные зоны
  splitDockWidget(elementsDock_, propertyDock_, Qt::Vertical);

  // Задаем комфортную ширину боковых панелей и высоту правой зоны по умолчанию
  resizeDocks({libraryDock_, elementsDock_, propertyDock_}, {125, 225, 225},
              Qt::Horizontal);
  resizeDocks({elementsDock_, propertyDock_}, {360, 540}, Qt::Vertical);

  // Подключение меню "Вид"
  ui->menuView->addSeparator();
  ui->menuView->addAction(libraryDock_->toggleViewAction());
  ui->menuView->addAction(elementsDock_->toggleViewAction());
  ui->menuView->addAction(propertyDock_->toggleViewAction());
}

void MainWindow::onNewProject() {
  bool ok = false;
  QString name = QInputDialog::getText(
      this, QStringLiteral("Новый проект"), QStringLiteral("Имя схемы:"),
      QLineEdit::Normal, QStringLiteral("Схема 1"), &ok);
  if (ok && !name.trimmed().isEmpty()) {
    projectName_ = name.trimmed();
    currentFilePath_.clear();

    // Удаляем все дополнительные страницы кроме первой
    for (int i = 1; i < pages_.size(); ++i) {
      delete pages_[i].scene;
    }
    pages_.resize(1);
    pages_[0].name = QStringLiteral("Страница 1");
    activePageIndex_ = 0;
    scene_ = pages_[0].scene;
    scene_->clearAll();

    tabWidget_->setPages({pages_[0].name}, 0);
    view_->setCanvasScene(scene_);
    connectScene(scene_);
    if (elementsDock_) elementsDock_->setScene(scene_);
    if (libraryDock_) libraryDock_->setScene(scene_);
    if (propertyDock_) propertyDock_->setScene(scene_);

    updateWindowTitle();
  }
}

void MainWindow::onOpenProject() {
  QString fileName = QFileDialog::getOpenFileName(
      this, QStringLiteral("Открыть схему"), QString(),
      QStringLiteral("Схемы DrawFlow / XML (*.xml *.drawio);;Все файлы (*.*)"));
  if (fileName.isEmpty()) {
    return;
  }

  QList<PageInfo> loadedPages;
  QString loadedProjectName;
  QString errorMsg;
  if (ProjectStorage::loadFromFile(fileName, loadedProjectName, loadedPages, &errorMsg, this)) {
    // Освобождаем старые сцены
    for (const auto &p : pages_) {
      delete p.scene;
    }
    pages_.clear();

    QStringList names;
    for (const auto &pi : loadedPages) {
      PageData pd;
      pd.name = pi.name;
      pd.scene = pi.scene;
      pages_.append(pd);
      names.append(pi.name);
    }
    projectName_ = loadedProjectName;
    currentFilePath_ = fileName;
    activePageIndex_ = 0;
    scene_ = pages_.isEmpty() ? nullptr : pages_[0].scene;

    tabWidget_->setPages(names, 0);
    if (scene_) {
      view_->setCanvasScene(scene_);
      connectScene(scene_);
      if (elementsDock_) elementsDock_->setScene(scene_);
      if (libraryDock_) libraryDock_->setScene(scene_);
      if (propertyDock_) propertyDock_->setScene(scene_);
    }
    updateWindowTitle();
  } else {
    QMessageBox::critical(
        this, QStringLiteral("Ошибка открытия"),
        QStringLiteral("Не удалось открыть файл:\n%1").arg(errorMsg));
  }
}

void MainWindow::onSaveProject() {
  if (currentFilePath_.isEmpty()) {
    onSaveProjectAs();
    return;
  }

  QList<PageInfo> toSave;
  for (const auto &p : pages_) {
    toSave.append({p.name, p.scene});
  }

  QString errorMsg;
  if (ProjectStorage::saveToFile(currentFilePath_, projectName_, toSave,
                                 &errorMsg)) {
    updateWindowTitle();
  } else {
    QMessageBox::critical(
        this, QStringLiteral("Ошибка сохранения"),
        QStringLiteral("Не удалось сохранить файл:\n%1").arg(errorMsg));
  }
}

void MainWindow::onSaveProjectAs() {
  QString defaultName = currentFilePath_.isEmpty()
                            ? (projectName_ + QStringLiteral(".xml"))
                            : currentFilePath_;
  QString fileName = QFileDialog::getSaveFileName(
      this, QStringLiteral("Сохранить схему как"), defaultName,
      QStringLiteral("Схемы DrawFlow / XML (*.xml *.drawio);;Все файлы (*.*)"));
  if (fileName.isEmpty()) {
    return;
  }

  QList<PageInfo> toSave;
  for (const auto &p : pages_) {
    toSave.append({p.name, p.scene});
  }

  QString errorMsg;
  if (ProjectStorage::saveToFile(fileName, projectName_, toSave, &errorMsg)) {
    currentFilePath_ = fileName;
    updateWindowTitle();
  } else {
    QMessageBox::critical(
        this, QStringLiteral("Ошибка сохранения"),
        QStringLiteral("Не удалось сохранить файл:\n%1").arg(errorMsg));
  }
}

void MainWindow::onExportBinary() {
  QList<PageInfo> exportPages;
  for (const auto &p : pages_) {
    exportPages.append({p.name, p.scene});
  }

  QString defaultDir = currentFilePath_.isEmpty()
                           ? QString()
                           : QFileInfo(currentFilePath_).absolutePath();
  ExportBinaryDialog dlg(exportPages, activePageIndex_, defaultDir, this);
  if (dlg.exec() != QDialog::Accepted) {
    return;
  }

  int targetIndex = dlg.selectedPageIndex();
  if (targetIndex < 0 || targetIndex >= pages_.size()) {
    return;
  }

  CanvasScene *targetScene = pages_[targetIndex].scene;
  QString fileName = dlg.selectedFilePath();

  int count = 0;
  QString errorMsg;
  if (ProjectStorage::exportK1804Binary(fileName, targetScene, &count, &errorMsg)) {
    QMessageBox::information(
        this, QStringLiteral("Экспорт завершен"),
        QStringLiteral("Файл данных сформирован. Обработано %1 блоков на странице \"%2\".")
            .arg(count)
            .arg(pages_[targetIndex].name));
  } else {
    QMessageBox::critical(
        this, QStringLiteral("Ошибка экспорта"),
        QStringLiteral("Не удалось экспортировать данные:\n%1").arg(errorMsg));
  }
}

void MainWindow::onGridSettings() {
  bool ok = false;
  int currentStep = static_cast<int>(scene_->gridSize());
  int newStep = QInputDialog::getInt(this, QStringLiteral("Шаг сетки"),
                                     QStringLiteral("Размер ячейки (px):"),
                                     currentStep, 5, 200, 5, &ok);
  if (ok) {
    scene_->setGridSize(newStep);
  }
}

void MainWindow::onHelpContents() {
  HelpDialog dlg(this);
  dlg.exec();
}

void MainWindow::onAbout() {
  QMessageBox::about(
      this, QStringLiteral("О программе"),
      QStringLiteral(
          "<h3>DrawFlow</h3>"
          "<p>Минималистичная среда редактирования схем и диаграмм.</p>"
          "<p>Собрано с Qt <b>%1</b>.</p>")
          .arg(QT_VERSION_STR));
}

} // namespace editor
