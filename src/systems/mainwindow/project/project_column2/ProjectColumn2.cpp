#include "systems/mainwindow/project/project_column2/ProjectColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
namespace pvd {
ProjectColumn2::ProjectColumn2(const QString& db,QWidget* parent):QWidget(parent){
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Project / State Settings"));auto* form=new QFormLayout();form->setRowWrapPolicy(QFormLayout::WrapAllRows);auto* product=new QComboBox(this);product->addItems({"Raspberry Pi Pico 2 W","Raspberry Pi Pico 2"});language_=new QComboBox(this);language_->addItems({"C++","C"});state_=new QComboBox(this);state_->addItems({"Development","Testing","Release"});form->addRow("Product",product);form->addRow("Language",language_);form->addRow("State",state_);l->addLayout(form);l->addWidget(makePanelTitle("State-specific options",this));for(const auto& t:{"Debug session tools","Runtime diagnostics","Verbose build evidence"}){auto* c=new QCheckBox(t,this);c->setChecked(true);l->addWidget(c);}l->addStretch();connect(language_,&QComboBox::currentTextChanged,this,&ProjectColumn2::languageChanged);connect(state_,&QComboBox::currentTextChanged,this,&ProjectColumn2::stateChanged);
}
QString ProjectColumn2::language()const{return language_->currentText();}
QString ProjectColumn2::stateName()const{return state_->currentText();}
}
