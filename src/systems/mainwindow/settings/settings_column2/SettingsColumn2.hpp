// SettingsColumn2.hpp
#pragma once
#include "systems/state/ApplicationState.hpp"
#include <QWidget>
class QListWidget;
namespace pvd { class SettingsColumn2 final:public QWidget{Q_OBJECT public:explicit SettingsColumn2(const QString& db,QWidget* parent=nullptr);void refresh(const ApplicationState& state);signals:void selectionRequested(const QString& componentId);private:QListWidget* list_=nullptr;}; }
