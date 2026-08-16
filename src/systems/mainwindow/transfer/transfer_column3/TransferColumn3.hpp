// TransferColumn3.hpp
#pragma once
#include <QWidget>
class QLineEdit; class QPlainTextEdit; class QProcess;
namespace pvd { class TransferColumn3 final:public QWidget{Q_OBJECT public:explicit TransferColumn3(const QString& db,QWidget* parent=nullptr);void configure(const QString& artifact,const QString& method);private:QString artifact_,method_;QLineEdit* drive_=nullptr;QPlainTextEdit* log_=nullptr;QProcess* process_=nullptr;}; }
