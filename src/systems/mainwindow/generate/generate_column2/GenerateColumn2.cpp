// GenerateColumn2.cpp
#include "systems/mainwindow/generate/generate_column2/GenerateColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QFileInfo>
#include <QListWidget>
namespace pvd { GenerateColumn2::GenerateColumn2(const QString& db,QWidget* parent):QWidget(parent){/**Builds the generated-file manifest column.*/auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Project Files"));list_=new QListWidget(this);l->addWidget(list_,1);}void GenerateColumn2::setFiles(const QStringList& files){/**Refreshes the generated-file manifest.*/list_->clear();for(const auto& f:files)list_->addItem(QFileInfo(f).fileName());} }
