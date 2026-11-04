#include "mainwindow.h"
#include "projectstorage.h"
#include "src/canvas/canvasview.h"
#include "src/elements/connectorelement.h"
#include "src/elements/masterblockelement.h"
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
    setFixedSize(20, 20);
    setIcon(QIcon(QStringLiteral(":/dock-close.svg")));
    setIconSize(QSize(12, 12));
    setAutoRaise(true);
    setCursor(Qt::ArrowCursor);
    setToolTip(QStringLiteral("Закрыть панель"));
    setStyleSheet(QStringLiteral("QToolButton { background: transparent; "
                                 "border: none; border-radius: 4px; }"));
  }

protected:
  void enterEvent(QEnterEvent *event) override {
    setIcon(QIcon(QStringLiteral(":/dock-close-hover.svg")));
    setStyleSheet(QStringLiteral("QToolButton { background-color: #ef4444; "
                                 "border: none; border-radius: 4px; }"));
    QToolButton::enterEvent(event);
  }

  void leaveEvent(QEvent *event) override {
    setIcon(QIcon(QStringLiteral(":/dock-close.svg")));
    setStyleSheet(QStringLiteral("QToolButton { background: transparent; "
                                 "border: none; border-radius: 4px; }"));
    QToolButton::leaveEvent(event);
  }
};

class DockCollapseButton : public QToolButton {
public:
  explicit DockCollapseButton(QDockWidget *dock, QWidget *parent = nullptr)
      : QToolButton(parent), dock_(dock) {
    setFixedSize(20, 20);
    setIcon(QIcon(QStringLiteral(":/dock-collapse.svg")));
    setIconSize(QSize(12, 12));
    setAutoRaise(true);
    setCursor(Qt::ArrowCursor);
    setToolTip(QStringLiteral("Свернуть панель"));
    setStyleSheet(
        QStringLiteral("QToolButton { background: transparent; border: none; "
                       "border-radius: 3px; }"
                       "QToolButton:hover { background-color: #cbd5e1; }"));
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

  // Применение нейтральной приглушенной серой темы
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

  view_->setScene(scene_);
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
  // 1. Палитра приложения (Исходный светлый стиль)
  QPalette palette;
  palette.setColor(QPalette::Window, QColor(234, 237, 240));        // #eaedf0
  palette.setColor(QPalette::WindowText, QColor(33, 37, 41));       // #212529
  palette.setColor(QPalette::Base, QColor(255, 255, 255));          // #ffffff
  palette.setColor(QPalette::AlternateBase, QColor(241, 243, 245)); // #f1f3f5
  palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
  palette.setColor(QPalette::ToolTipText, QColor(33, 37, 41));
  palette.setColor(QPalette::Text, QColor(33, 37, 41));
  palette.setColor(QPalette::Button, QColor(241, 243, 245));
  palette.setColor(QPalette::ButtonText, QColor(33, 37, 41));
  palette.setColor(QPalette::Highlight, QColor(71, 85, 105)); // #475569
  palette.setColor(QPalette::HighlightedText, Qt::white);
  QApplication::setPalette(palette);

  // 2. Исходный светлый QSS
  setStyleSheet(QStringLiteral(
      "QMainWindow { background-color: #eaedf0; }"

      "QMenuBar { background-color: #eaedf0; border-bottom: 1px solid #cbd5e1; "
      "font-size: 12px; color: #334155; padding: 2px 4px; }"
      "QMenuBar::item { background: transparent; padding: 4px 8px; "
      "border-radius: 3px; color: #334155; }"
      "QMenuBar::item:selected { background-color: #cbd5e1; color: #0f172a; }"

      "QMenu { background-color: #ffffff; border: 1px solid #cbd5e1; "
      "border-radius: 4px; padding: 3px; color: #334155; font-size: 12px; }"
      "QMenu::item { padding: 5px 22px 5px 12px; border-radius: 3px; color: "
      "#334155; }"
      "QMenu::item:selected { background-color: #f1f5f9; color: #0f172a; }"
      "QMenu::separator { height: 1px; background: #e2e8f0; margin: 3px 6px; }"

      "QToolBar { background-color: #eaedf0; border-bottom: 1px solid #cbd5e1; "
      "spacing: 4px; padding: 2px 6px; min-height: 30px; }"
      "QToolBar::separator { width: 1px; background-color: #cbd5e1; margin: "
      "4px 6px; }"
      "QToolButton { background: transparent; border: 1px solid transparent; "
      "border-radius: 3px; padding: 3px 5px; color: #475569; }"
      "QToolButton:hover { background-color: #cbd5e1; border-color: #94a3b8; "
      "color: #0f172a; }"
      "QToolButton:pressed { background-color: #94a3b8; }"
      "QToolButton:checked { background-color: #cbd5e1; border: 1px solid "
      "#64748b; color: #0f172a; }"

      "QDockWidget { font-size: 11px; color: #334155; border: 1px solid "
      "#cbd5e1; }"
      "#dockCustomTitleBar { background-color: #eaedf0; border-bottom: 1px "
      "solid #cbd5e1; }"

      "QMainWindow::separator { background-color: #cbd5e1; width: 3px; height: "
      "3px; }"
      "QMainWindow::separator:hover { background-color: #94a3b8; }"

      "QTreeWidget { background-color: #ffffff; border: 1px solid #cbd5e1; "
      "border-radius: 3px; color: #334155; font-size: 11px; outline: none; }"
      "QTreeWidget::branch { background: transparent; }"
      "QTreeWidget::branch:selected { background: transparent; }"
      "QTreeWidget::item { padding: 4px; }"
      "QTreeWidget::item:hover { background-color: #f1f5f9; }"
      "QTreeWidget::item:selected { background-color: #e2e8f0; color: #0f172a; "
      "font-weight: 600; }"
      "QHeaderView::section { background-color: #eaedf0; border: none; "
      "border-bottom: 1px solid #cbd5e1; padding: 3px 6px; font-weight: 600; "
      "color: #475569; font-size: 11px; }"

      "QGroupBox { font-size: 11px; font-weight: 600; color: #475569; border: "
      "1px solid #cbd5e1; border-radius: 3px; margin-top: 14px; padding-top: "
      "8px; background-color: #ffffff; }"
      "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top "
      "left; left: 8px; padding: 0 4px; background-color: #ffffff; }"

      "QLabel { color: #475569; font-size: 11px; }"
      "QLineEdit, QDoubleSpinBox, QSpinBox, QComboBox { background-color: "
      "#ffffff; border: 1px solid #cbd5e1; border-radius: 3px; padding: 2px "
      "6px; font-size: 11px; color: #1e293b; }"
      "QLineEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus, QComboBox:focus "
      "{ border-color: #64748b; }"
      "QComboBox::drop-down { border: none; width: 16px; }"
      "QComboBox QAbstractItemView { background-color: #ffffff; color: "
      "#1e293b; selection-background-color: #e2e8f0; selection-color: #0f172a; "
      "border: 1px solid #cbd5e1; }"

      "QPushButton { background-color: #ffffff; border: 1px solid #cbd5e1; "
      "border-radius: 3px; padding: 4px 8px; font-size: 11px; font-weight: "
      "500; color: #334155; }"
      "QPushButton:hover { background-color: #f8fafc; border-color: #94a3b8; }"
      "QPushButton:pressed { background-color: #e2e8f0; }"

      "QScrollBar:vertical { background: #eaedf0; width: 8px; margin: 0; }"
      "QScrollBar::handle:vertical { background: #cbd5e1; min-height: 16px; "
      "border-radius: 4px; }"
      "QScrollBar::handle:vertical:hover { background: #94a3b8; }"
      "QScrollBar::horizontal { background: #eaedf0; height: 8px; margin: 0; }"
      "QScrollBar::handle:horizontal { background: #cbd5e1; min-width: 16px; "
      "border-radius: 4px; }"
      "QScrollBar::handle:horizontal:hover { background: #94a3b8; }"
      "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"

      "#scrollArea, #contentWidget { background-color: #eaedf0; border: none; }"
      "#noSelectionLabel { color: #64748b; font-size: 11px; padding: 24px 0; "
      "}"));
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
  comboGridStep_->setToolTip(QStringLiteral("Шаг сетки (px)"));
  comboGridStep_->addItem(QStringLiteral("10 px"), 10.0);
  comboGridStep_->addItem(QStringLiteral("20 px"), 20.0);
  comboGridStep_->addItem(QStringLiteral("50 px"), 50.0);
  comboGridStep_->setCurrentIndex(1); // 20 px по умолчанию
  comboGridStep_->setStyleSheet(QStringLiteral(
      "QComboBox { background-color: #ffffff; border: 1px solid #cbd5e1; "
      "border-radius: 3px; padding: 2px 6px; color: #1e293b; font-size: 11px; "
      "min-width: 60px; height: 20px; }"
      "QComboBox:hover { border-color: #94a3b8; }"
      "QComboBox::drop-down { border: none; width: 14px; }"
      "QComboBox QAbstractItemView { background-color: #ffffff; color: "
      "#1e293b; selection-background-color: #e2e8f0; selection-color: #0f172a; "
      "border: 1px solid #cbd5e1; }"));
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
  label->setStyleSheet(
      QStringLiteral("font-size: 11px; font-weight: 700; color: #334155; "
                     "letter-spacing: 0.2px;"));
  layout->addWidget(label);
  layout->addStretch();

  // Кнопка сворачивания/разворачивания содержимого панели
  auto btnCollapse = new DockCollapseButton(dock, titleBar);
  layout->addWidget(btnCollapse);

  // Кнопка открепления (float)
  auto btnFloat = new QToolButton(titleBar);
  btnFloat->setFixedSize(20, 20);
  btnFloat->setIcon(QIcon(QStringLiteral(":/dock-float.svg")));
  btnFloat->setIconSize(QSize(12, 12));
  btnFloat->setAutoRaise(true);
  btnFloat->setToolTip(QStringLiteral("Открепить окно"));
  btnFloat->setStyleSheet(
      QStringLiteral("QToolButton { background: transparent; border: none; "
                     "border-radius: 3px; }"
                     "QToolButton:hover { background-color: #cbd5e1; }"));
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

  // 1. Узкая компактная библиотека элементов (слева)
  libraryDock_ = new ElementsLibraryFrame(scene_, this);
  libraryDock_->setMinimumWidth(110);
  libraryDock_->setMaximumWidth(130);
  libraryDock_->setTitleBarWidget(
      createCustomDockTitleBar(QStringLiteral("Фигуры"), libraryDock_));
  addDockWidget(Qt::LeftDockWidgetArea, libraryDock_);

  // 2. Дерево объектов (справа сверху)
  elementsDock_ = new WindowElementsFrame(scene_, this);
  elementsDock_->setMinimumWidth(230);
  elementsDock_->setTitleBarWidget(createCustomDockTitleBar(
      QStringLiteral("Дерево объектов"), elementsDock_));
  addDockWidget(Qt::RightDockWidgetArea, elementsDock_);

  // 3. Свойства (справа снизу)
  propertyDock_ = new ElementPropertyFrame(scene_, this);
  propertyDock_->setMinimumWidth(230);
  propertyDock_->setTitleBarWidget(
      createCustomDockTitleBar(QStringLiteral("Свойства"), propertyDock_));
  addDockWidget(Qt::RightDockWidgetArea, propertyDock_);

  // Разделяем правую часть по вертикали на две аккуратные зоны
  splitDockWidget(elementsDock_, propertyDock_, Qt::Vertical);

  // Поднимаем панель свойств повыше: дерево объектов ~280px (чуть меньше
  // половины), а свойствам даем основной приоритет по высоте (~620px)
  resizeDocks({elementsDock_, propertyDock_}, {380, 520}, Qt::Vertical);

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
    scene_->clearAll();
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

  QString errorMsg;
  if (ProjectStorage::loadFromFile(fileName, projectName_, scene_, &errorMsg)) {
    currentFilePath_ = fileName;
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

  QString errorMsg;
  if (ProjectStorage::saveToFile(currentFilePath_, projectName_, scene_,
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

  QString errorMsg;
  if (ProjectStorage::saveToFile(fileName, projectName_, scene_, &errorMsg)) {
    currentFilePath_ = fileName;
    updateWindowTitle();
  } else {
    QMessageBox::critical(
        this, QStringLiteral("Ошибка сохранения"),
        QStringLiteral("Не удалось сохранить файл:\n%1").arg(errorMsg));
  }
}

void MainWindow::onExportBinary() {
  QString defaultName = projectName_ + QStringLiteral(".bin");
  QString fileName = QFileDialog::getSaveFileName(
      this, QStringLiteral("Экспорт микрокоманд К1804"), defaultName,
      QStringLiteral(
          "Бинарные файлы микрокоманд (*.bin *.rom);;Все файлы (*.*)"));
  if (fileName.isEmpty())
    return;

  int count = 0;
  QString errorMsg;
  if (ProjectStorage::exportK1804Binary(fileName, scene_, &count, &errorMsg)) {
    QMessageBox::information(
        this, QStringLiteral("Экспорт завершен"),
        QStringLiteral("Файл данных сформирован. Обработано %1 блоков на "
                       "текущей странице.")
            .arg(count));
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
