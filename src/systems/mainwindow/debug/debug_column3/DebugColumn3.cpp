// DebugColumn3.cpp
#include "systems/mainwindow/debug/debug_column3/DebugColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTcpSocket>

namespace pvd {
DebugColumn3::DebugColumn3(const QString& db,QWidget* parent):QWidget(parent)
{
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Debug"));
    auto* row=new QHBoxLayout(); auto* start=new QPushButton("Start OpenOCD",this); auto* stop=new QPushButton("Stop",this);
    row->addWidget(start); row->addWidget(stop); l->addLayout(row);
    log_=new QPlainTextEdit(this); log_->setReadOnly(true); l->addWidget(log_,1);
    process_=new QProcess(this);
    connect(process_,&QProcess::readyReadStandardOutput,this,[this](){log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardOutput()));});
    connect(process_,&QProcess::readyReadStandardError,this,[this](){log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardError()));});
    connect(process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){log_->appendPlainText("OpenOCD process error: "+QString::number(static_cast<int>(error)));});
    connect(start,&QPushButton::clicked,this,&DebugColumn3::startServer);
    connect(stop,&QPushButton::clicked,this,&DebugColumn3::stopServer);
}

DebugColumn3::~DebugColumn3(){stopServer();}

void DebugColumn3::configure(const QString& exe,const QString& interfaceCfg,const QString& targetCfg,int speed,const QString& resetMethod)
{
    exe_=exe; interface_=interfaceCfg; target_=targetCfg; speed_=speed; resetMethod_=resetMethod; startServer();
}

void DebugColumn3::startServer()
{
    if(process_->state()!=QProcess::NotRunning)return;
    log_->appendPlainText(QString("Connecting probe through %1").arg(exe_));
    QStringList args={"-f",interface_,"-f",target_,"-c",QString("adapter speed %1").arg(speed_)};
    // OpenOCD must finish target initialization before reset commands are issued.
    // Passing reset/run during configuration causes some OpenOCD 0.12 builds to
    // reject the command before the GDB server is available.
    process_->start(exe_,args);
}

void DebugColumn3::stopServer()
{
    if(!process_ || process_->state()==QProcess::NotRunning)return;
    log_->appendPlainText("Stopping OpenOCD...");
    QTcpSocket control;
    control.connectToHost(QHostAddress::LocalHost,4444);
    if(control.waitForConnected(500)){
        // A debugger may leave one or both RP2350 cores halted. Resume the
        // target before shutting down OpenOCD so Stop returns hardware to the
        // same running state it had before the debug session.
        control.write("reset run\nshutdown\n");
        control.waitForBytesWritten(500);
        control.waitForDisconnected(1500);
    }
    if(process_->state()!=QProcess::NotRunning && !process_->waitForFinished(2500)){
        log_->appendPlainText("OpenOCD did not terminate; forcing shutdown.");
        process_->kill();
        process_->waitForFinished(2000);
    }
    log_->appendPlainText("OpenOCD stopped.");
}
}
