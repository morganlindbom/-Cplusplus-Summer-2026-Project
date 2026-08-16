// PioColumn2.cpp
#include "systems/mainwindow/pio/pio_column2/PioColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QListWidget>
#include <QSignalBlocker>
namespace pvd {
PioColumn2::PioColumn2(const QString& db,QWidget* parent):QWidget(parent)
{
    /**Builds the PIO program navigator.*/
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","PIO Programs"));list_=new QListWidget(this);l->addWidget(list_,1);connect(list_,&QListWidget::currentTextChanged,this,[this](const QString& n){/**Requests one PIO source buffer.*/if(!n.isEmpty())emit programSelected(n);});setPrograms({"Mogge"});
}
void PioColumn2::setPrograms(const QStringList& names)
{
    /**Replaces the visible PIO program list.*/
    const QSignalBlocker blocker(list_);list_->clear();list_->addItems(names);if(list_->count())list_->setCurrentRow(0);
}
}
