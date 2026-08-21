// TransferColumn2.hpp
#pragma once
#include <QWidget>
class QLineEdit; class QComboBox;
namespace pvd { class TransferColumn2 final:public QWidget{Q_OBJECT public:explicit TransferColumn2(const QString& db,QWidget* parent=nullptr);void setBuildPath(const QString& path,const QString& targetName);QString artifact()const;QString method()const;private:QLineEdit* artifact_=nullptr;QComboBox* method_=nullptr;}; }
