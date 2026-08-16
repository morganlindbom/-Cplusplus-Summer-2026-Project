// BuildColumn2.cpp
#include "systems/mainwindow/build/build_column2/BuildColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QDir>
#include <QFormLayout>
#include <QLineEdit>
namespace pvd { BuildColumn2::BuildColumn2(const QString& db,QWidget* parent):QWidget(parent){/**Builds build-path configuration.*/auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Build Configuration"));auto* f=new QFormLayout();f->setRowWrapPolicy(QFormLayout::WrapAllRows);source_=new QLineEdit(this);source_->setReadOnly(true);build_=new QLineEdit(this);f->addRow("Generated source",source_);f->addRow("Build directory",build_);l->addLayout(f);l->addStretch();}void BuildColumn2::setProjectPath(const QString& path){/**Derives generated source and build paths from the active project.*/source_->setText(QDir(path).filePath("generated"));build_->setText(QDir(path).filePath("build"));}QString BuildColumn2::buildDirectory()const{/**Returns the configured CMake build directory.*/return build_->text();} }
