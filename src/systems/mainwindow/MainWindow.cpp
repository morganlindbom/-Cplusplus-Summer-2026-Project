// MainWindow.cpp
#include "systems/mainwindow/MainWindow.hpp"
#include "systems/mainwindow/workflow/Workflow.hpp"
#include "systems/mainwindow/viewer/Viewer.hpp"
#include <QHBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QApplication>
#include <QScreen>
#include <algorithm>

namespace
{

    constexpr int kDefaultWindowWidth = 1200;
    constexpr int kDefaultWindowHeight = 600;

    constexpr int kTargetScreenIndex = 2;

    constexpr float kWorkflowColumnPercent = .15f;
    constexpr float kProjectSettingsColumnPercent = .20f;
    constexpr float kProjectManagementColumnPercent = .50f;
    constexpr float kViewerColumnPercent = .15f;

    QList<int> columnSizesForWidth(int width) { return {int(width * kWorkflowColumnPercent), int(width * kProjectSettingsColumnPercent), int(width * kProjectManagementColumnPercent), int(width * kViewerColumnPercent)}; }
}

namespace pvd
{
    MainWindow::MainWindow(Workflow *workflow, Viewer *viewer, QWidget *parent) : QMainWindow(parent), workflow_(workflow), viewer_(viewer)
    {
        /**Creates the permanent four-column shell used by every workflow page.*/
        resize(kDefaultWindowWidth, kDefaultWindowHeight);
        setMinimumSize(600, 400);
        const auto screens = QApplication::screens();
        if (!screens.isEmpty())
        {
            const int screenIndex = std::clamp(kTargetScreenIndex, 0, int(screens.size()) - 1);
            const QRect area = screens[screenIndex]->availableGeometry();
            move(area.center() - QPoint(width() / 2, height() / 2));
        }
        auto *root = new QWidget(this);
        auto *layout = new QHBoxLayout(root);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);
        auto *splitter = new QSplitter(Qt::Horizontal, root);
        column2Stack_ = new QStackedWidget(splitter);
        column3Stack_ = new QStackedWidget(splitter);
        splitter->addWidget(workflow_);
        splitter->addWidget(column2Stack_);
        splitter->addWidget(column3Stack_);
        splitter->addWidget(viewer_);
        splitter->setStretchFactor(0, int(kWorkflowColumnPercent * 100));
        splitter->setStretchFactor(1, int(kProjectSettingsColumnPercent * 100));
        splitter->setStretchFactor(2, int(kProjectManagementColumnPercent * 100));
        splitter->setStretchFactor(3, int(kViewerColumnPercent * 100));
        const QList<QWidget *> panels{workflow_, column2Stack_, column3Stack_, viewer_};
        for (auto *panel : panels)
        {
            panel->setMinimumSize(0, 0);
            panel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        }
        column3Stack_->setMinimumWidth(0);
        column3Stack_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        viewer_->setMinimumWidth(300);
        splitter->setChildrenCollapsible(false);
        splitter->setSizes(columnSizesForWidth(kDefaultWindowWidth));
        layout->addWidget(splitter);
        setCentralWidget(root);
        QTimer::singleShot(0, this, [splitter]()
                           { splitter->setSizes(columnSizesForWidth(splitter->width())); });
        setStyleSheet("QMainWindow{background:#f4f6f8;} QWidget{font-family:'Segoe UI';font-size:12px;} QListWidget,QPlainTextEdit,QTextEdit,QTableWidget,QTreeWidget{background:white;border:1px solid #d7dce2;} QPushButton{padding:5px 8px;} QGroupBox{font-weight:600;} QSplitter::handle{background:#e4e7eb;width:3px;}");
    }

    void MainWindow::registerWorkflow(const QString &id, QWidget *column2, QWidget *column3)
    {
        /**Registers one workflow's unique Column 2 and Column 3 objects.*/
        column2->setMinimumSize(0, 0);
        column3->setMinimumSize(0, 0);
        column2->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        column3->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        const int index = column2Stack_->addWidget(column2);
        column3Stack_->addWidget(column3);
        indices_[id] = index;
    }

    void MainWindow::activateWorkflow(const QString &id)
    {
        /**Swaps only Column 2 and Column 3 while Workflow and Viewer remain unchanged.*/
        if (!indices_.contains(id))
            return;
        column2Stack_->setCurrentIndex(indices_[id]);
        column3Stack_->setCurrentIndex(indices_[id]);
    }
}
