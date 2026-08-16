// ProjectStore.hpp
#pragma once
#include "systems/state/ApplicationState.hpp"
#include <QString>
namespace pvd {
class ProjectStore final
{
public:
    static bool save(const ApplicationState& state, QString* error=nullptr);
    static bool load(const QString& directory, ApplicationState* state, QString* error=nullptr);
};
}
