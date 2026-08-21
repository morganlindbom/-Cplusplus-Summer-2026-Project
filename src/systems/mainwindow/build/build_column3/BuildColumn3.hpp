// BuildColumn3.hpp
#pragma once
#include <QWidget>
class QPlainTextEdit; class QProcess;
namespace pvd { class BuildColumn3 final:public QWidget{Q_OBJECT public:explicit BuildColumn3(const QString& db,QWidget* parent=nullptr);void setPaths(const QString& source,const QString& build);void build();signals:void buildStarted();void buildCompleted(bool ok,const QString& output);private:void run(const QString& program,const QStringList& args,bool isBuild);QString source_,build_;QPlainTextEdit* log_=nullptr;QProcess* process_=nullptr;bool currentProcessIsBuild_=false;}; }
