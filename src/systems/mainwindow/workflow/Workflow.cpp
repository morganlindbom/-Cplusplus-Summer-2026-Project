// Workflow.cpp
#include "systems/mainwindow/workflow/Workflow.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QListWidget>

namespace pvd {
Workflow::Workflow(const QString& databasePath, QWidget* parent) : QWidget(parent)
{
    /**Builds the permanent workflow navigation object.*/
    auto* layout=makePanelLayout(this, SqliteUtil::metadata(databasePath,"title","Workflow"));
    list_=new QListWidget(this);
    const QStringList names={"Project","Function Selection","Settings","Generate","C++ / C Code","ASM PIO Code","Build","Transfer","Debug"};
    const QStringList ids={"project","function_selection","settings","generate","code","pio","build","transfer","debug"};
    for(int i=0;i<names.size();++i){ auto* item=new QListWidgetItem(names[i],list_); item->setData(Qt::UserRole,ids[i]); }
    layout->addWidget(list_,1);
    connect(list_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item){
        /**Emits a stable workflow identifier when navigation changes.*/
        if(item) emit workflowSelected(item->data(Qt::UserRole).toString());
    });
    list_->setCurrentRow(0);
}

QString Workflow::currentWorkflow() const
{
    /**Returns the stable identifier of the selected workflow.*/
    return list_->currentItem() ? list_->currentItem()->data(Qt::UserRole).toString() : QString{};
}

void Workflow::selectWorkflow(const QString& workflowId)
{
    /**Selects a workflow by its stable identifier.*/
    for(int i=0;i<list_->count();++i) if(list_->item(i)->data(Qt::UserRole).toString()==workflowId){ list_->setCurrentRow(i); return; }
}
}
