#include "systems/generation/FunctionExecutionModel.hpp"

namespace pvd {

bool FunctionExecutionModel::supportsCoreSelection(const QString& functionId)
{
    return functionId == "gpio.output"
        || functionId == "onboard_led.output"
        || functionId == "robo.buzzer"
        || functionId == "robo.neopixel";
}

QString FunctionExecutionModel::effectiveCore(const FunctionSelection& selection, bool core1Enabled)
{
    if (!core1Enabled || !supportsCoreSelection(selection.functionId))
        return QStringLiteral("Core 0");
    return selection.settings.value("execution_core", "Core 0") == "Core 1"
        ? QStringLiteral("Core 1") : QStringLiteral("Core 0");
}

QString FunctionExecutionModel::runtimeModel(const QString& functionId)
{
    if (functionId == "gpio.output" || functionId == "onboard_led.output")
        return QStringLiteral("cooperative periodic state update");
    if (functionId == "robo.buzzer")
        return QStringLiteral("cooperative one-shot tone completion");
    if (functionId == "robo.neopixel")
        return QStringLiteral("cooperative one-shot PIO data submission");
    return QStringLiteral("hardware-driven or initialization-only");
}

QString FunctionExecutionModel::coreSelectionReason(const QString& functionId)
{
    if (supportsCoreSelection(functionId))
        return QStringLiteral("Runtime can execute cooperatively on either RP2350 core; hardware initialization remains on Core 0.");
    return QStringLiteral("This function is hardware-driven or initialization-only with the current settings, so it has no movable software runtime.");
}

}
