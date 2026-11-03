#pragma once

#include <QList>
#include <QWidget>

class QLabel;
class QPushButton;
class QHBoxLayout;

namespace editor {

class CanvasView;

class CanvasTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit CanvasTabWidget(CanvasView *view, QWidget *parent = nullptr);
    ~CanvasTabWidget() override = default;

    void setZoomText(const QString &text);
    int pageCount() const { return pageButtons_.size(); }

signals:
    void pageChanged(int index);
    void pageAdded();

private slots:
    void onAddPageClicked();
    void onPageButtonClicked();

private:
    void updatePageButtonsStyle();

    CanvasView *view_ = nullptr;
    QWidget *bottomBar_ = nullptr;
    QHBoxLayout *pagesLayout_ = nullptr;
    QList<QPushButton*> pageButtons_;
    QPushButton *btnAddPage_ = nullptr;
    QLabel *zoomLabel_ = nullptr;
    int currentPageIndex_ = 0;
};

} // namespace editor
