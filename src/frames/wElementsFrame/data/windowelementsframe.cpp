#include "windowelementsframe.h"
#include "src/canvas/canvasscene.h"
#include "ui_windowelementsframe.h"

#include <QHeaderView>
#include <QIcon>
#include <QTreeWidget>

namespace editor {

WindowElementsFrame::WindowElementsFrame(CanvasScene *scene, QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::WindowElementsFrame)
    , scene_(scene)
{
    ui->setupUi(this);

    ui->treeWidget->setRootIsDecorated(false);
    ui->treeWidget->setIndentation(0);
    ui->treeWidget->header()->setStretchLastSection(true);
    ui->treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &WindowElementsFrame::onItemSelectionChanged);
    connect(ui->treeWidget, &QTreeWidget::itemClicked, this, &WindowElementsFrame::onItemClicked);
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &WindowElementsFrame::onItemChanged);

    connect(scene_, &CanvasScene::elementAdded, this, &WindowElementsFrame::scheduleRefresh);
    connect(scene_, &CanvasScene::elementRemoved, this, &WindowElementsFrame::scheduleRefresh);
    connect(scene_, &CanvasScene::sceneCleared, this, &WindowElementsFrame::scheduleRefresh);
    connect(scene_, &CanvasScene::sceneSelectionChanged, this, &WindowElementsFrame::onSceneSelectionChanged);
    connect(scene_, &CanvasScene::selectionBatchFinished, this, [this] {
        if (refreshPending_) { refreshPending_ = false; refreshElementsList(); }
    });
    connect(scene_, &QObject::destroyed, this, [this] {
        scene_ = nullptr;
        refreshElementsList();
    });

    refreshElementsList();
}

WindowElementsFrame::~WindowElementsFrame() {
    delete ui;
}

void WindowElementsFrame::setScene(CanvasScene *scene) {
    if (scene_ != scene) {
        if (scene_) {
            disconnect(scene_, nullptr, this, nullptr);
        }
        scene_ = scene;
        if (scene_) {
            connect(scene_, &CanvasScene::elementAdded, this, &WindowElementsFrame::scheduleRefresh);
            connect(scene_, &CanvasScene::elementRemoved, this, &WindowElementsFrame::scheduleRefresh);
            connect(scene_, &CanvasScene::sceneCleared, this, &WindowElementsFrame::scheduleRefresh);
            connect(scene_, &CanvasScene::sceneSelectionChanged, this, &WindowElementsFrame::onSceneSelectionChanged);
    connect(scene_, &CanvasScene::selectionBatchFinished, this, [this] {
        if (refreshPending_) { refreshPending_ = false; refreshElementsList(); }
    });
            connect(scene_, &QObject::destroyed, this, [this] {
                scene_ = nullptr;
                refreshElementsList();
            });
        }
        refreshElementsList();
    }
}

void WindowElementsFrame::scheduleRefresh() {
    if (scene_ && scene_->isSelectionBatching()) {
        refreshPending_ = true;
    } else {
        refreshPending_ = false;
        refreshElementsList();
    }
}

void WindowElementsFrame::refreshElementsList() {
    isUpdatingSelection_ = true;
    ui->treeWidget->clear();

    const auto elements = scene_ ? scene_->allElements() : QList<ShapeElement *>{};
    for (ShapeElement *element : elements) {
        auto item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, element->name());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(element)));

        if (element->elementType() == ElementType::MasterBlock) {
            item->setIcon(0, QIcon(QStringLiteral(":/masterblock.svg")));
        } else if (element->elementType() == ElementType::Connector) {
            item->setIcon(0, QIcon(QStringLiteral(":/connector.svg")));
        } else if (element->elementType() == ElementType::Mask) {
            item->setIcon(0, QIcon(QStringLiteral(":/mask.svg")));
        }

        if (element->isSelected()) {
            ui->treeWidget->setCurrentItem(item);
        }
    }

    isUpdatingSelection_ = false;
}

void WindowElementsFrame::onSceneSelectionChanged() {
    if (isUpdatingSelection_) return;

    isUpdatingSelection_ = true;
    ShapeElement *selected = scene_->selectedElement();

    if (!selected) {
        ui->treeWidget->clearSelection();
        isUpdatingSelection_ = false;
        return;
    }

    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
        auto elem = static_cast<ShapeElement*>(item->data(0, Qt::UserRole).value<void*>());
        if (elem == selected) {
            ui->treeWidget->setCurrentItem(item);
            break;
        }
    }
    isUpdatingSelection_ = false;
}

void WindowElementsFrame::onItemSelectionChanged() {
    if (isUpdatingSelection_) return;

    isUpdatingSelection_ = true;
    QTreeWidgetItem *current = ui->treeWidget->currentItem();
    if (current) {
        auto elem = static_cast<ShapeElement*>(current->data(0, Qt::UserRole).value<void*>());
        if (elem) {
            scene_->clearSelection();
            elem->setSelected(true);
        }
    } else {
        scene_->clearSelection();
    }
    isUpdatingSelection_ = false;
}

void WindowElementsFrame::onItemClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    auto elem = static_cast<ShapeElement*>(item->data(0, Qt::UserRole).value<void*>());
    if (!elem) return;
    scene_->clearSelection();
    elem->setSelected(true);
}

void WindowElementsFrame::onItemChanged(QTreeWidgetItem *item, int column) {
    if (column == 0 && item) {
        auto elem = static_cast<ShapeElement*>(item->data(0, Qt::UserRole).value<void*>());
        if (elem && elem->name() != item->text(0)) {
            elem->setName(item->text(0));
        }
    }
}

} // namespace editor
