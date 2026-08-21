// PicoPinMap.hpp
#pragma once
#include <QString>
namespace pvd {
int gpioForPhysicalPin(int physicalPin);
QString componentDisplayName(const QString& componentId);
int roboPicoGpio(const QString& resourceId);
}
