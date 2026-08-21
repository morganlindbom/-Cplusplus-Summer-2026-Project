// CodeColumn3.hpp
#pragma once
#include <QWidget>
class QPlainTextEdit; class QLabel;
namespace pvd { class CodeColumn3 final:public QWidget{Q_OBJECT public:explicit CodeColumn3(const QString& db,QWidget* parent=nullptr);void loadFile(const QString& path);void setBuildResult(bool ok,const QString& message);signals:void fileSaved(const QString& path);void buildRequested();private:void saveCurrent();void checkSyntax();QPlainTextEdit* editor_=nullptr;QLabel* pathLabel_=nullptr;QLabel* status_=nullptr;QString path_;}; }
