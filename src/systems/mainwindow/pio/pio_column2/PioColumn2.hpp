// PioColumn2.hpp
#pragma once
#include <QWidget>
class QListWidget;
namespace pvd { class PioColumn2 final:public QWidget{Q_OBJECT public:explicit PioColumn2(const QString& db,QWidget* parent=nullptr);void setPrograms(const QStringList& names);signals:void programSelected(const QString& name);private:QListWidget* list_=nullptr;}; }
