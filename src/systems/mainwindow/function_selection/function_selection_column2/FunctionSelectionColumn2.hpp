// FunctionSelectionColumn2.hpp
#pragma once
#include <QWidget>
class QListWidget;
namespace pvd { class FunctionSelectionColumn2 final:public QWidget{Q_OBJECT public:explicit FunctionSelectionColumn2(const QString& db,QWidget* parent=nullptr);void selectComponent(const QString& id);QString currentComponent()const;signals:void componentSelected(const QString& id);private:QListWidget* list_=nullptr;}; }
