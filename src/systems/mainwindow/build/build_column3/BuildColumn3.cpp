// BuildColumn3.cpp
#include "systems/mainwindow/build/build_column3/BuildColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>

namespace {
QString findExisting(const QStringList& candidates)
{
    for (const QString& path : candidates)
        if (QFileInfo::exists(path))
            return path;
    return {};
}
}

namespace pvd {
BuildColumn3::BuildColumn3(const QString& db,QWidget* parent):QWidget(parent)
{
    auto* layout=makePanelLayout(this,SqliteUtil::metadata(db,"title","Build"));
    auto* row=new QHBoxLayout();
    auto* configure=new QPushButton("Configure",this);
    auto* buildButton=new QPushButton("Build",this);
    row->addWidget(configure); row->addWidget(buildButton); layout->addLayout(row);
    log_=new QPlainTextEdit(this); log_->setReadOnly(true); layout->addWidget(log_,1);
    process_=new QProcess(this);
    connect(process_,&QProcess::readyReadStandardOutput,this,[this](){log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardOutput()));});
    connect(process_,&QProcess::readyReadStandardError,this,[this](){log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardError()));});
    connect(process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){emit buildCompleted(status==QProcess::NormalExit&&code==0);});
    connect(configure,&QPushButton::clicked,this,[this](){
        /**Starts a clean Pico configure so a desktop CMake cache cannot be reused.*/
        if(!build_.isEmpty()){
            QFile::remove(QDir(build_).filePath("CMakeCache.txt"));
            QDir(QDir(build_).filePath("CMakeFiles")).removeRecursively();
        }
        const QString toolchain=QDir::home().filePath(".pico-sdk/toolchain/15_2_Rel1/bin");
        run("cmake",{"-S",source_,"-B",build_,"-G","Ninja","-DPICO_BOARD=pico2_w",
            "-DCMAKE_C_COMPILER="+QDir(toolchain).filePath("arm-none-eabi-gcc.exe"),
            "-DCMAKE_CXX_COMPILER="+QDir(toolchain).filePath("arm-none-eabi-g++.exe"),
            "-DCMAKE_ASM_COMPILER="+QDir(toolchain).filePath("arm-none-eabi-gcc.exe")});
    });
    connect(buildButton,&QPushButton::clicked,this,[this](){run("cmake",{"--build",build_,"--parallel","4"});});
}
void BuildColumn3::setPaths(const QString& source,const QString& build){source_=source;build_=build;}
void BuildColumn3::run(const QString& program,const QStringList& args)
{
    if(process_->state()!=QProcess::NotRunning)return;
    QProcessEnvironment env=QProcessEnvironment::systemEnvironment();
    const QString sdk=findExisting({env.value("PICO_SDK_PATH"),QDir::home().filePath(".pico-sdk/sdk/2.3.0")});
    const QString toolchain=findExisting({env.value("PICO_TOOLCHAIN_PATH"),QDir::home().filePath(".pico-sdk/toolchain/15_2_Rel1/bin")});
    if(!sdk.isEmpty())env.insert("PICO_SDK_PATH",sdk);
    if(!toolchain.isEmpty())env.insert("PICO_TOOLCHAIN_PATH",toolchain);
    process_->setProcessEnvironment(env);
    log_->appendPlainText("$ "+program+" "+args.join(' '));
    log_->appendPlainText("PICO_SDK_PATH="+env.value("PICO_SDK_PATH"));
    log_->appendPlainText("PICO_TOOLCHAIN_PATH="+env.value("PICO_TOOLCHAIN_PATH"));
    process_->start(program,args);
}
}
