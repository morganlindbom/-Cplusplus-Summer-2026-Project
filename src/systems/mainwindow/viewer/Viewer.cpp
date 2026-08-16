#include "systems/mainwindow/viewer/Viewer.hpp"
#include "systems/mainwindow/viewer/GlbView.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <functional>

namespace {
class BoardView final : public QGraphicsView {
public:
    std::function<void(QString)> selected;
    using QGraphicsView::QGraphicsView;
protected:
    void mousePressEvent(QMouseEvent* e) override {
        if(auto* hit=itemAt(e->pos())) {
            QString id=hit->data(0).toString();
            if(id.isEmpty()&&hit->parentItem()) id=hit->parentItem()->data(0).toString();
            if(!id.isEmpty()&&selected){selected(id);e->accept();return;}
        }
        QGraphicsView::mousePressEvent(e);
    }
};
}

namespace pvd {
Viewer::Viewer(const QString& databasePath,QWidget* parent):QWidget(parent) {
    auto* layout=makePanelLayout(this,SqliteUtil::metadata(databasePath,"title","Viewer"));
    auto* tools=new QHBoxLayout();
    auto* home=new QPushButton("Home",this);auto* top=new QPushButton("Top",this);auto* clear=new QPushButton("Clear",this);
    tools->addWidget(home);tools->addWidget(top);tools->addWidget(clear);layout->addLayout(tools);
    view_=new GlbView(this);view_->componentClicked=[this](const QString& id){emit componentSelected(id);};layout->addWidget(view_,1);status_=new QLabel("Simulation — ready",this);status_->setWordWrap(true);layout->addWidget(status_);
    buildBoard();
    connect(home,&QPushButton::clicked,this,[this](){view_->resetView();});
    connect(top,&QPushButton::clicked,this,[this](){view_->topView();});
    connect(clear,&QPushButton::clicked,this,[this](){selectComponent({});});
}
void Viewer::buildBoard(){
    items_.clear();QString path=QDir(QStringLiteral(PVD_RUNTIME_ROOT)).filePath("assets/PICO2W.glb");
    if(!QFileInfo::exists(path))path=QDir(QCoreApplication::applicationDirPath()).filePath("assets/PICO2W.glb");
    QString error;if(!view_->load(path,&error))status_->setText("GLB error: "+error);
}
void Viewer::selectComponent(const QString& id){selected_=id;view_->setSelectedComponent(id);if(id.isEmpty())status_->setText("Simulation — ready");else if(items_.contains(id))status_->setText("Selected: "+id);}
void Viewer::setSimulationText(const QString& text){status_->setText(text);}
}
