// BuildColumn2.hpp
#pragma once
#include <QWidget>
class QLineEdit;
namespace pvd { class BuildColumn2 final:public QWidget{Q_OBJECT public:explicit BuildColumn2(const QString& db,QWidget* parent=nullptr);void setProjectPath(const QString& path);QString buildDirectory()const;private:QLineEdit* source_=nullptr;QLineEdit* build_=nullptr;}; }
