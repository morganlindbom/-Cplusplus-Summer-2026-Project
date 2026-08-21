// TransferColumn3.cpp
#include "systems/mainwindow/transfer/transfer_column3/TransferColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QDir>
#include <QDirIterator>
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
        QString safetyReason;
        if(!artifactIsCurrent(&safetyReason)){log_->appendPlainText("Transfer blocked: "+safetyReason);return;}
        if(method_=="OpenOCD probe"){
            const QString ocd=findOpenOcd(), scripts=QFileInfo(ocd).dir().filePath("scripts");
            QString elf=artifact_; elf.replace(QRegularExpression("\\.uf2$",QRegularExpression::CaseInsensitiveOption),".elf"); elf.replace('\\','/');
            if(!QFileInfo::exists(elf)){log_->appendPlainText("Matching ELF not found: "+elf);return;}
            // OpenOCD parses the -c value as a Tcl command. Quote the ELF path
            // because project directories commonly contain spaces.
            const QString programCommand=QString("program \"%1\" verify reset exit").arg(elf);
            process_->start(ocd,{"-s",scripts,"-f","interface/cmsis-dap.cfg","-f","target/rp2350.cfg","-c","adapter speed 5000","-c",programCommand});
            log_->appendPlainText("Flashing through CMSIS-DAP probe: "+elf);
        }else if(method_.startsWith("picotool"))process_->start("picotool",{"load","-f",artifact_});
        else{const QString target=QDir(drive_->text()).filePath(QFileInfo(artifact_).fileName());if(QFile::copy(artifact_,target))log_->appendPlainText("Copied to "+target);else log_->appendPlainText("Copy failed. Check drive path and BOOTSEL mode.");}
    });
}
void TransferColumn3::configure(const QString& artifact,const QString& method,const QString& generatedDirectory,const QString& buildDirectory,const QString& expectedTarget,bool latestBuildSuccessful)
{
    artifact_=artifact;
    method_=method;
    generatedDirectory_=generatedDirectory;
    buildDirectory_=buildDirectory;
    expectedTarget_=expectedTarget;
    latestBuildSuccessful_=latestBuildSuccessful;
}
bool TransferColumn3::artifactIsCurrent(QString* reason)const
{
    const QFileInfo artifactInfo(artifact_);
    if(artifact_.isEmpty()||!artifactInfo.exists()){if(reason)*reason="the expected firmware artifact does not exist.";return false;}
    const QFileInfo buildMarker(QDir(buildDirectory_).filePath(".pvd_build_success"));
    // The marker is the durable source of truth for a successful build. The
    // in-memory flag is only a UI hint and may be reset when navigating between
    // workflow pages or reopening the project.
    if(!buildMarker.exists()){
        if(reason)*reason="the active build directory has no successful build marker.";
        return false;
    }
    if(!buildMarker.exists()||buildMarker.lastModified()<artifactInfo.lastModified()){
        if(reason)*reason="the active build directory has no successful build marker newer than the firmware.";
        return false;
    }
    if(artifactInfo.completeBaseName()!=expectedTarget_){if(reason)*reason=QString("artifact '%1' does not match active target '%2'.").arg(artifactInfo.completeBaseName(),expectedTarget_);return false;}
    const QString artifactDir=QDir(artifactInfo.absolutePath()).canonicalPath();
    const QString expectedBuildDir=QDir(buildDirectory_).canonicalPath();
    if(artifactDir.isEmpty()||expectedBuildDir.isEmpty()||artifactDir!=expectedBuildDir){if(reason)*reason="artifact is outside the active project's configured build directory.";return false;}
    QDirIterator generated(generatedDirectory_,QDir::Files,QDirIterator::Subdirectories);
    while(generated.hasNext()){
        const QFileInfo sourceInfo(generated.next());
        if(sourceInfo.lastModified()>artifactInfo.lastModified()){
            if(reason)*reason=QString("generated source '%1' is newer than the firmware; rebuild before transfer.").arg(sourceInfo.fileName());
            return false;
        }
    }
    if(method_=="OpenOCD probe"){
        QString elf=artifact_;elf.replace(QRegularExpression("\\.uf2$",QRegularExpression::CaseInsensitiveOption),".elf");
        const QFileInfo elfInfo(elf);
        if(!elfInfo.exists()){
            if(reason)*reason="matching ELF is missing.";
            return false;
        }
        QDirIterator generatedForElf(generatedDirectory_,QDir::Files,QDirIterator::Subdirectories);
        while(generatedForElf.hasNext()){
            const QFileInfo sourceInfo(generatedForElf.next());
            if(sourceInfo.lastModified()>elfInfo.lastModified()){
                if(reason)*reason=QString("generated source '%1' is newer than the ELF; rebuild before transfer.").arg(sourceInfo.fileName());
                return false;
            }
        }
    }
    return true;
}
}
