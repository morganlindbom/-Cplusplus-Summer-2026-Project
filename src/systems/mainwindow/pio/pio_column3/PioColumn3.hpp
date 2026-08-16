// PioColumn3.hpp
#pragma once
#include <QHash>
#include <QWidget>
class QPlainTextEdit; class QLabel;
namespace pvd { class PioColumn3 final:public QWidget{Q_OBJECT public:explicit PioColumn3(const QString& db,QWidget* parent=nullptr);void selectProgram(const QString& name);void reloadGeneratedFiles(const QStringList& files);QHash<QString,QString> programs()const;signals:void programsChanged();private:bool validate(QString* error)const;QString generatedTemplate(const QString& name)const;QPlainTextEdit* editor_=nullptr;QLabel* status_=nullptr;QString current_;QHash<QString,QString> programs_;}; }
