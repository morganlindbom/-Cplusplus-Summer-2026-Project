// FunctionSelectionColumn3.hpp
#pragma once
#include "systems/components/FunctionCatalog.hpp"
#include <QWidget>
class QComboBox; class QLabel;
namespace pvd { class FunctionSelectionColumn3 final:public QWidget{Q_OBJECT public:explicit FunctionSelectionColumn3(const QString& db,FunctionCatalog* catalog,QWidget* parent=nullptr);void setComponent(const QString& id);void setSelectedFunction(const QString& functionId);signals:void functionChanged(const QString& componentId,const FunctionOption& option,int gpio,int physicalPin);private:void populate();QString componentId_;FunctionCatalog* catalog_=nullptr;QComboBox* selector_=nullptr;QLabel* info_=nullptr;QVector<FunctionOption> options_;}; }
Q_DECLARE_METATYPE(pvd::FunctionOption)
