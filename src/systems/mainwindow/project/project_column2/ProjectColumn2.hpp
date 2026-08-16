// ProjectColumn2.hpp
#pragma once
#include <QWidget>
class QComboBox; class QCheckBox;
namespace pvd { class ProjectColumn2 final:public QWidget{Q_OBJECT public:explicit ProjectColumn2(const QString& db,QWidget* parent=nullptr);QString language()const;QString stateName()const;signals:void languageChanged(const QString&);void stateChanged(const QString&);private:QComboBox* language_=nullptr;QComboBox* state_=nullptr;}; }
