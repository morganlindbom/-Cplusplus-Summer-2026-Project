// TransferColumn2.cpp
#include "systems/mainwindow/transfer/transfer_column2/TransferColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>

namespace pvd {
TransferColumn2::TransferColumn2(const QString& db,QWidget* parent):QWidget(parent)
{
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Transfer Target"));
    auto* f=new QFormLayout(); f->setRowWrapPolicy(QFormLayout::WrapAllRows); artifact_=new QLineEdit(this); method_=new QComboBox(this);
    method_->addItems({"OpenOCD probe","picotool load","Copy UF2 to drive"});
    f->addRow("Firmware artifact",artifact_); f->addRow("Method",method_); l->addLayout(f); l->addStretch();
}
void TransferColumn2::setBuildPath(const QString& path,const QString& targetName){const QString candidate=QDir(path).filePath(targetName+".uf2");artifact_->setText(QFileInfo::exists(candidate)?candidate:QString{});}
QString TransferColumn2::artifact()const{return artifact_->text();}
QString TransferColumn2::method()const{return method_->currentText();}
}
