// PicoPinMap.cpp
#include "systems/components/PicoPinMap.hpp"
#include <QHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
namespace pvd {
int gpioForPhysicalPin(int physicalPin)
{
    /**Maps Raspberry Pi Pico 2 W physical header pins to GPIO numbers.*/
    static const QHash<int,int> map={{1,0},{2,1},{4,2},{5,3},{6,4},{7,5},{9,6},{10,7},{11,8},{12,9},{14,10},{15,11},{16,12},{17,13},{19,14},{20,15},{21,16},{22,17},{24,18},{25,19},{26,20},{27,21},{29,22},{31,26},{32,27},{34,28}}; return map.value(physicalPin,-1);
}
QString componentDisplayName(const QString& componentId)
{
    /**Converts stable board-component identifiers into visible labels.*/
    if(componentId.startsWith("pin_")) return "Pin "+componentId.mid(4); if(componentId=="rp2350a")return "RP2350A"; if(componentId=="wireless")return "Wireless"; if(componentId=="usb_connector")return "USB Connector"; if(componentId=="bootsel")return "BOOTSEL"; if(componentId=="onboard_led")return "Onboard LED"; if(componentId=="debug_probe")return "Debug Probe"; return componentId;
}

int roboPicoGpio(const QString& resourceId)
{
    /**Reads board-specific GPIO assignments from the ROBO-PICO JSONC source of truth.*/
    QFile file(QDir(QStringLiteral(PVD_RUNTIME_ROOT)).filePath("assets/robo_pico_source_of_truth.jsonc"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    QString json = QString::fromUtf8(file.readAll());
    json.remove(QRegularExpression(QStringLiteral("^\\s*//.*$"), QRegularExpression::MultilineOption));
    json.replace(QRegularExpression(QStringLiteral(",\\s*([}\\]])")), QStringLiteral("\\1"));
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    for (const auto value : document.object().value("board_resources").toArray()) {
        const auto object = value.toObject();
        if (object.value("id").toString() == resourceId)
            return object.value("gpio").toInt(-1);
    }
    return -1;
}
}
