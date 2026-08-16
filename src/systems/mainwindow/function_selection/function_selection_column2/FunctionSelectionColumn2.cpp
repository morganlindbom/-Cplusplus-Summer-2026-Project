// FunctionSelectionColumn2.cpp
#include "systems/mainwindow/function_selection/function_selection_column2/FunctionSelectionColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include "systems/components/PicoPinMap.hpp"
#include <QListWidget>
namespace pvd {
FunctionSelectionColumn2::FunctionSelectionColumn2(const QString& db,QWidget* parent):QWidget(parent)
{
    /**Builds the Components navigator used by Function Selection.*/
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Components"));list_=new QListWidget(this);const QStringList special={"rp2350a","wireless","usb_connector","bootsel","onboard_led","debug_probe"};for(const auto& id:special){auto* item=new QListWidgetItem(componentDisplayName(id),list_);item->setData(Qt::UserRole,id);}for(int p=1;p<=40;++p){auto* item=new QListWidgetItem(QString("Pin %1").arg(p),list_);item->setData(Qt::UserRole,QString("pin_%1").arg(p));}l->addWidget(list_,1);connect(list_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item){/**Emits the stable selected component identifier.*/if(item)emit componentSelected(item->data(Qt::UserRole).toString());});list_->setCurrentRow(0);
}
void FunctionSelectionColumn2::selectComponent(const QString& id)
{
    /**Synchronizes an external Viewer selection into the Components list.*/
    for(int i=0;i<list_->count();++i)if(list_->item(i)->data(Qt::UserRole).toString()==id){list_->setCurrentRow(i);return;}
}
QString FunctionSelectionColumn2::currentComponent()const
{
    /**Returns the currently selected component identifier.*/
    return list_->currentItem()?list_->currentItem()->data(Qt::UserRole).toString():QString{};
}
}
