// GenerateColumn2.hpp
#pragma once
#include <QWidget>
class QListWidget;
namespace pvd { class GenerateColumn2 final:public QWidget{Q_OBJECT public:explicit GenerateColumn2(const QString& db,QWidget* parent=nullptr);void setFiles(const QStringList& files);private:QListWidget* list_=nullptr;}; }
