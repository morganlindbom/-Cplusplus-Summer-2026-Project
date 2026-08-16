// ProjectGenerator.hpp
#pragma once
#include "systems/state/ApplicationState.hpp"
#include <QHash>
#include <QStringList>
namespace pvd { class ProjectGenerator final{public:static bool generate(ApplicationState* state,const QHash<QString,QString>& pioPrograms,QString* error=nullptr);static bool validate(const ApplicationState& state,QString* report=nullptr);private:static QString generateMain(const ApplicationState& state);static QString generateCMake(const ApplicationState& state,const QStringList& pioFiles);}; }
