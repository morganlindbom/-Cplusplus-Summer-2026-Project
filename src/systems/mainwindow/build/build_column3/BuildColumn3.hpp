// BuildColumn3.hpp
#pragma once
#include <QWidget>
class QPlainTextEdit; class QProcess;
namespace pvd { class BuildColumn3 final:public QWidget{Q_OBJECT public:explicit BuildColumn3(const QString& db,QWidget* parent=nullptr);void setPaths(const QString& source,const QString& build);signals:void buildCompleted(bool ok);private:void run(const QString& program,const QStringList& args);QString source_,build_;QPlainTextEdit* log_=nullptr;QProcess* process_=nullptr;}; }
