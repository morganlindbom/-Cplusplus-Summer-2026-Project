// TransferColumn3.hpp
#pragma once
#include <QWidget>
class QLineEdit; class QPlainTextEdit; class QProcess;
namespace pvd { class TransferColumn3 final:public QWidget{Q_OBJECT public:explicit TransferColumn3(const QString& db,QWidget* parent=nullptr);void configure(const QString& artifact,const QString& method,const QString& generatedDirectory,const QString& buildDirectory,const QString& expectedTarget,bool latestBuildSuccessful);private:bool artifactIsCurrent(QString* reason)const;QString artifact_,method_,generatedDirectory_,buildDirectory_,expectedTarget_;bool latestBuildSuccessful_=false;QLineEdit* drive_=nullptr;QPlainTextEdit* log_=nullptr;QProcess* process_=nullptr;}; }
