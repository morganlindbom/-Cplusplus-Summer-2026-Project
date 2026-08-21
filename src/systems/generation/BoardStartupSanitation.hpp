#pragma once

#include "systems/state/ApplicationState.hpp"
#include <QString>

namespace pvd {

class BoardStartupSanitation final
{
public:
    static bool needsRoboPicoNeoPixelClear(const ApplicationState& state);
    static QString generateRoboPicoNeoPixelClear(const ApplicationState& state);
};

}
