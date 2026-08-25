// SettingsColumn3.hpp
#pragma once
#include "systems/state/ApplicationState.hpp"
#include <QHash>
#include <QPointer>
#include <QWidget>
class QFormLayout; class QLabel; class QWidget;
namespace pvd { class FunctionCatalog; class SettingsColumn3 final:public QWidget{Q_OBJECT public:explicit SettingsColumn3(const QString& db,FunctionCatalog* catalog,QWidget* parent=nullptr);void showSelection(const FunctionSelection& selection);void setCore1Enabled(bool enabled);QString currentComponentId() const;signals:void settingChanged(const QString& componentId,const QString& key,const QString& value);private:void clearForm();void addSetting(const QString& key,const QString& label,const QString& type,const QString& defaultValue,const QString& help,const QStringList& values={});FunctionCatalog* catalog_=nullptr;QFormLayout* form_=nullptr;QLabel* info_=nullptr;QString componentId_,functionId_,functionName_;int gpio_=-1;QHash<QString,QString> current_;QPointer<QWidget> directionEditor_;quint64 editorGeneration_=0;bool core1Enabled_=false;}; }
