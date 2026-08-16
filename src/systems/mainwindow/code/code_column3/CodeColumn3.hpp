// CodeColumn3.hpp
#pragma once
#include <QWidget>
class QPlainTextEdit; class QLabel;
namespace pvd { class CodeColumn3 final:public QWidget{Q_OBJECT public:explicit CodeColumn3(const QString& db,QWidget* parent=nullptr);void loadFile(const QString& path);signals:void fileSaved(const QString& path);private:void saveCurrent();QPlainTextEdit* editor_=nullptr;QLabel* pathLabel_=nullptr;QString path_;}; }
