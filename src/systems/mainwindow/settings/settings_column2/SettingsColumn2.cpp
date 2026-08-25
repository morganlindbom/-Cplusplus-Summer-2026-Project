// SettingsColumn2.cpp
#include "systems/mainwindow/settings/settings_column2/SettingsColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QListWidget>
namespace pvd {
SettingsColumn2::SettingsColumn2(const QString& db,QWidget* parent):QWidget(parent)
{
    /**Builds the Selected Functions navigator for Settings.*/
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Selected Functions"));list_=new QListWidget(this);list_->setObjectName("settings_selection");l->addWidget(list_,1);connect(list_,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item){/**Requests Settings Column 3 context from System.*/if(item)emit selectionRequested(item->data(Qt::UserRole).toString());});
}
void SettingsColumn2::refresh(const ApplicationState& state)
{
    /**Rebuilds the list from active non-disabled project selections.*/
    list_->clear();for(const auto& s:state.selections){if(s.functionId=="disabled"||s.functionId.isEmpty())continue;auto* item=new QListWidgetItem(s.displayName+" — "+s.functionName,list_);item->setData(Qt::UserRole,s.componentId);}if(list_->count()>0)list_->setCurrentRow(0);
}
}
