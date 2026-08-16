// DebugColumn3.hpp
#pragma once
#include <QWidget>
class QPlainTextEdit; class QProcess;
namespace pvd { class DebugColumn3 final:public QWidget{Q_OBJECT public:explicit DebugColumn3(const QString& db,QWidget* parent=nullptr);~DebugColumn3() override;void configure(const QString& exe,const QString& interfaceCfg,const QString& targetCfg,int speed,const QString& resetMethod="run reset");void startServer();void stopServer();private:QString exe_,interface_,target_,resetMethod_="run reset";int speed_=5000;QPlainTextEdit* log_=nullptr;QProcess* process_=nullptr;}; }
