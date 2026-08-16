// FunctionCatalog.cpp
#include "systems/components/FunctionCatalog.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QDir>
#include <QFileInfo>
#include <utility>
namespace pvd {
FunctionCatalog::FunctionCatalog(QString rootDirectory):root_(std::move(rootDirectory))
{
    /**Stores the directory containing self-contained pin-function databases.*/
}

QVector<FunctionOption> FunctionCatalog::functionsForGpio(int gpio) const
{
    /**Discovers all function databases whose pin_mappings permit the requested GPIO.*/
    QVector<FunctionOption> result; QDir dir(root_); const auto folders=dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
    for(const auto& folder:folders){const QString db=dir.filePath(folder+"/function.sqlite"); if(!QFileInfo::exists(db))continue; const auto rows=SqliteUtil::rows(db,"SELECT 1 AS ok FROM pin_mappings WHERE gpio=? LIMIT 1",{gpio}); if(rows.isEmpty())continue; FunctionOption o; o.id=SqliteUtil::metadata(db,"function_id");o.name=SqliteUtil::metadata(db,"display_name");o.category=SqliteUtil::metadata(db,"category");o.description=SqliteUtil::metadata(db,"description");o.databasePath=db;result.push_back(o);} return result;
}

QVector<FunctionOption> FunctionCatalog::specialFunctions(const QString& componentId) const
{
    /**Returns non-GPIO functions for board-level components such as BOOTSEL and LED.*/
    QVector<FunctionOption> r; if(componentId=="rp2350a") r.push_back({"rp2350a.configure","Configure RP2350A","Processor","Configure RP2350A clock, voltage, cores, watchdog and stdio.",{}}); else if(componentId=="bootsel") r.push_back({"bootsel.use_button","Use BOOTSEL Button","BOOTSEL","Use the Pico 2 W BOOTSEL system button.",{}}); else if(componentId=="onboard_led") r.push_back({"onboard_led.output","Onboard LED","LED","Control the onboard wireless LED through CYW43.",{}}); else if(componentId=="debug_probe") r.push_back({"debug_probe.cmsis_dap","CMSIS-DAP Debug Probe","Debug","Configure CMSIS-DAP/OpenOCD debug tools.",{}}); else if(componentId=="wireless") r.push_back({"wireless.cyw43","Wi-Fi / Bluetooth","Wireless","Configure Pico 2 W wireless support.",{}}); return r;
}

QString FunctionCatalog::functionDatabase(const QString& functionId) const
{
    /**Locates a self-contained database by stable function identifier.*/
    QDir dir(root_); for(const auto& folder:dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {const QString db=dir.filePath(folder+"/function.sqlite"); if(QFileInfo::exists(db)&&SqliteUtil::metadata(db,"function_id")==functionId)return db;} return {};
}
}
