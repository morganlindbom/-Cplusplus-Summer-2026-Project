// GenerateColumn3.hpp
#pragma once
#include <QWidget>
class QLabel;
namespace pvd { class GenerateColumn3 final:public QWidget{Q_OBJECT public:explicit GenerateColumn3(const QString& db,QWidget* parent=nullptr);void setStatus(const QString& text,bool ok=true);signals:void validateRequested();void generateRequested();private:QLabel* status_=nullptr;}; }
