#include "systems/mainwindow/debug/debug_column2/DebugColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
namespace pvd {
DebugColumn2::DebugColumn2(const QString& db,QWidget* parent):QWidget(parent){
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Debug Configuration"));
    auto* f=new QFormLayout(); f->setRowWrapPolicy(QFormLayout::WrapAllRows);
    QString exe=QStandardPaths::findExecutable("openocd");
    if(exe.isEmpty()) for(const auto& candidate:{QDir::home().filePath(".pico-sdk/openocd/0.12.0+dev/openocd.exe"),QStringLiteral("C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/openocd/openocd.exe")}) if(QFileInfo::exists(candidate)){exe=candidate;break;}
    QString scripts=exe.isEmpty()?QString():QFileInfo(exe).dir().filePath("scripts");
    QString interfaceCfg=QDir(scripts).filePath("interface/cmsis-dap.cfg"),targetCfg=QDir(scripts).filePath("target/rp2350.cfg");
    openocd_=new QLineEdit(exe.isEmpty()?QStringLiteral("openocd"):exe,this); interface_=new QLineEdit(QFileInfo::exists(interfaceCfg)?interfaceCfg:QStringLiteral("interface/cmsis-dap.cfg"),this); target_=new QLineEdit(QFileInfo::exists(targetCfg)?targetCfg:QStringLiteral("target/rp2350.cfg"),this); speed_=new QComboBox(this);speed_->setEditable(true);speed_->addItems({"100","500","1000","2000","5000","10000","25000","50000","Custom..."});speed_->setCurrentText("5000");connect(speed_,qOverload<int>(&QComboBox::activated),this,[this](int index){if(index==speed_->count()-1)speed_->setEditText(QString{});});
    f->addRow("OpenOCD",openocd_); f->addRow("Interface",interface_); f->addRow("Target",target_); f->addRow("Adapter kHz",speed_); l->addLayout(f); l->addStretch();
}
QString DebugColumn2::openocd()const{return openocd_->text();}
QString DebugColumn2::interfaceCfg()const{return interface_->text();}
QString DebugColumn2::targetCfg()const{return target_->text();}
int DebugColumn2::speed()const{return speed_->currentText().toInt();}
QString DebugColumn2::resetMethod()const{return resetMethod_;}
void DebugColumn2::applySetting(const QString& key,const QString& value){if(key=="adapter_speed")speed_->setCurrentText(value);else if(key=="reset_method"&&(value=="run reset"||value=="halt reset"||value=="none"))resetMethod_=value;}
}
