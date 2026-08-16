// CodeColumn2.hpp
#pragma once
#include <QWidget>
class QListWidget;
namespace pvd { class CodeColumn2 final:public QWidget{Q_OBJECT public:explicit CodeColumn2(const QString& db,QWidget* parent=nullptr);void setFiles(const QStringList& files);signals:void fileSelected(const QString& path);private:QListWidget* list_=nullptr;}; }
