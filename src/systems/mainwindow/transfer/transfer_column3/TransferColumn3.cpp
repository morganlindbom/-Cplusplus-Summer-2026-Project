// TransferColumn3.cpp
#include "systems/mainwindow/transfer/transfer_column3/TransferColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>

namespace {
QString findOpenOcd()
{
    const auto env=QProcessEnvironment::systemEnvironment();
    const QStringList paths={env.value("PICO_OPENOCD_PATH"),QDir::home().filePath(".pico-sdk/openocd/0.12.0+dev/openocd.exe"),QStringLiteral("C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/openocd/openocd.exe")};
    for(const auto& p:paths)if(QFileInfo::exists(p))return p;
    return QStringLiteral("openocd");
}
}

namespace pvd {
TransferColumn3::TransferColumn3(const QString& db,QWidget* parent):QWidget(parent)
{
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Transfer"));
    auto* f=new QFormLayout(); drive_=new QLineEdit("E:/",this); f->addRow("UF2 drive (copy method)",drive_); l->addLayout(f);
    auto* go=new QPushButton("Transfer Firmware",this); l->addWidget(go); log_=new QPlainTextEdit(this); log_->setReadOnly(true); l->addWidget(log_,1);
    process_=new QProcess(this);
    connect(process_,&QProcess::readyReadStandardOutput,this,[this](){log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardOutput()));});
    connect(process_,&QProcess::readyReadStandardError,this,[this](){log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardError()));});
    connect(go,&QPushButton::clicked,this,[this](){
        if(artifact_.isEmpty()){log_->appendPlainText("No firmware artifact selected.");return;}
        if(method_=="OpenOCD probe"){
            const QString ocd=findOpenOcd(), scripts=QFileInfo(ocd).dir().filePath("scripts");
            QString elf=artifact_; elf.replace(QRegularExpression("\\.uf2$",QRegularExpression::CaseInsensitiveOption),".elf"); elf.replace('\\','/');
            if(!QFileInfo::exists(elf)){log_->appendPlainText("Matching ELF not found: "+elf);return;}
            process_->start(ocd,{"-s",scripts,"-f","interface/cmsis-dap.cfg","-f","target/rp2350.cfg","-c","adapter speed 5000","-c",QString("program %1 verify reset exit").arg(elf)});
            log_->appendPlainText("Flashing through CMSIS-DAP probe: "+elf);
        }else if(method_.startsWith("picotool"))process_->start("picotool",{"load","-f",artifact_});
        else{const QString target=QDir(drive_->text()).filePath(QFileInfo(artifact_).fileName());if(QFile::copy(artifact_,target))log_->appendPlainText("Copied to "+target);else log_->appendPlainText("Copy failed. Check drive path and BOOTSEL mode.");}
    });
}
void TransferColumn3::configure(const QString& artifact,const QString& method){artifact_=artifact;method_=method;}
}
