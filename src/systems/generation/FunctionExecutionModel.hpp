#pragma once

#include "systems/state/ApplicationState.hpp"
#include <QString>

namespace pvd {

class FunctionExecutionModel final
{
public:
    static bool supportsCoreSelection(const QString& functionId);
    static QString effectiveCore(const FunctionSelection& selection, bool core1Enabled);
    static QString runtimeModel(const QString& functionId);
    static QString coreSelectionReason(const QString& functionId);
};

}
