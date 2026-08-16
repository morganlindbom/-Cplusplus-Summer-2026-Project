// CodeColumn2.cpp
#include "systems/mainwindow/code/code_column2/CodeColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QFileInfo>
#include <QListWidget>
namespace pvd {
CodeColumn2::CodeColumn2(const QString& db,QWidget* parent):QWidget(parent)
{
    /**Builds the generated project-file navigator.*/
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","C++ / C Project Files"));list_=new QListWidget(this);l->addWidget(list_,1);connect(list_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item){/**Requests the selected file in the code editor.*/if(item)emit fileSelected(item->data(Qt::UserRole).toString());});
}
void CodeColumn2::setFiles(const QStringList& files)
{
    /**Repopulates the file list while retaining full file paths in item data.*/
    list_->clear();for(const auto& f:files){auto* item=new QListWidgetItem(QFileInfo(f).fileName(),list_);item->setData(Qt::UserRole,f);}if(list_->count())list_->setCurrentRow(0);
}
}
