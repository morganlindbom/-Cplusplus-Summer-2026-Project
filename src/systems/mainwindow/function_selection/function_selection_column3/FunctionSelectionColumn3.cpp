// FunctionSelectionColumn3.cpp
#include "systems/mainwindow/function_selection/function_selection_column3/FunctionSelectionColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include "systems/components/PicoPinMap.hpp"
#include <QComboBox>
#include <QLabel>
#include <QRegularExpression>
#include <QSignalBlocker>
namespace pvd
{
namespace
{
QString componentDetails(const QString& id)
{
    QString purpose = QStringLiteral("Den här komponenten kan kopplas till en funktion i projektet.");
    QString example = QStringLiteral("Exempel: välj en funktion och kontrollera sedan dess inställningar i Settings.");
    QString alternative =
        QStringLiteral("Alternativ användning: välj Disabled om komponenten inte ska användas i den genererade koden.");
    QString warning = QStringLiteral("Varning: välj bara funktioner som passar komponenten och kontrollera elektriska "
                                     "nivåer innan du ansluter hårdvara.");
    if (id == "rp2350a")
    {
        purpose = QStringLiteral(
            "RP2350A är projektets huvudprocessor och styr systemklocka, spänning, cores, watchdog och stdio.");
        example = QStringLiteral("Exempel: ställ systemklockan till 150000 kHz och aktivera USB stdio för loggning.");
        alternative = QStringLiteral(
            "Alternativ användning: aktivera core 1 för bakgrundsarbete eller watchdog för automatisk återställning.");
        warning = QStringLiteral(
            "Varning: högre klocka och spänning kan ge instabilitet, högre effektförbrukning eller värme.");
    }
    else if (id.startsWith("pin_"))
    {
        purpose = QString("GPIO%1 är ansluten till fysisk Pin %2 och kan tilldelas en periferifunktion.")
                      .arg(gpioForPhysicalPin(id.mid(4).toInt()))
                      .arg(id.mid(4));
        example =
            QStringLiteral("Exempel: välj PWM, UART, SPI, I2C, ADC eller vanlig GPIO beroende på den externa kretsen.");
        alternative = QStringLiteral("Alternativ användning: samma fysiska pin kan användas för olika funktioner i "
                                     "olika projekt, men bara en åt gången.");
        warning = QStringLiteral("Varning: kontrollera RP2350A:s pin-multiplexering, riktning, pull-resistor och "
                                 "spänningsnivå innan anslutning.");
    }
    else if (id == "wireless")
    {
        purpose = QStringLiteral("Wireless hanterar Pico 2 W:s Wi-Fi- och Bluetooth-funktioner.");
        example =
            QStringLiteral("Exempel: välj Station och ange SSID och lösenord för anslutning till ett Wi-Fi-nätverk.");
        alternative =
            QStringLiteral("Alternativ användning: välj Access Point för att låta kortet skapa ett eget nätverk.");
        warning = QStringLiteral(
            "Varning: spara inte riktiga lösenord i delade projekt utan att först kontrollera projektets säkerhet.");
    }
    else if (id == "usb_connector")
    {
        purpose = QStringLiteral("USB Connector beskriver kortets USB-anslutning och dess ström/datafunktion.");
        example = QStringLiteral("Exempel: använd USB för programmering, seriell loggning och strömförsörjning.");
        alternative =
            QStringLiteral("Alternativ användning: använd extern UART eller Debug Probe när USB inte ska användas.");
        warning =
            QStringLiteral("Varning: kontrollera VBUS och strömförsörjning innan extern matning ansluts samtidigt.");
    }
    else if (id == "bootsel")
    {
        purpose =
            QStringLiteral("BOOTSEL är RP2350A:s fysiska boot-knapp för att välja start- och återställningsbeteende.");
        example = QStringLiteral("Exempel: välj Press för normal knapphantering eller Hold för ett håll-beteende.");
        alternative =
            QStringLiteral("Alternativ användning: lämna Disabled om knappen ska reserveras för bootloadern.");
        warning =
            QStringLiteral("Varning: BOOTSEL är en systemfunktion och kan påverka programmering och återställning.");
    }
    else if (id == "onboard_led")
    {
        purpose = QStringLiteral("Onboard LED styr Pico 2 W:s inbyggda trådlösa status-LED.");
        example = QStringLiteral("Exempel: välj On som startläge för att indikera att applikationen har startat.");
        alternative = QStringLiteral("Alternativ användning: styr LED:n från applikationslogik som statusindikator.");
        warning =
            QStringLiteral("Varning: LED:n är kopplad till den trådlösa CYW43-funktionen och är inte en vanlig GPIO.");
    }
    return QString("<b>%1</b><br><br>%2<br><br><b>Exempel</b><br>%3<br><br><b>Alternativ "
                   "användning</b><br>%4<br><br><b>Varning</b><br>%5")
        .arg(componentDisplayName(id), purpose, example, alternative, warning);
}
QString functionDetails(const FunctionOption& option)
{
    return QString("<b>%1</b><br><br>%2<br><br><b>Exempel</b><br>Välj funktionen när %3 ska "
                   "användas.<br><br><b>Alternativ användning</b><br>Välj Disabled eller en annan kompatibel funktion "
                   "för samma komponent.<br><br><b>Varning</b><br>Kontrollera pin-mappning, elektriska nivåer och "
                   "funktionens inställningar innan generering.")
        .arg(option.name, option.description, option.category);
}
} // namespace
FunctionSelectionColumn3::FunctionSelectionColumn3(const QString& db, FunctionCatalog* catalog, QWidget* parent)
    : QWidget(parent), catalog_(catalog)
{
    /**Builds the Functions selector and context information area.*/
    auto* l = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Functions"));
    selector_ = new QComboBox(this);
    selector_->setObjectName("function_selector");
    l->addWidget(selector_);
    info_ = new QLabel("Select a component to inspect valid functions.", this);
    info_->setWordWrap(true);
    info_->setAlignment(Qt::AlignTop);
    l->addWidget(info_, 1);
    connect(selector_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                /**Forwards one function choice to System.*/
                if (index < 0 || index >= options_.size())
                    return;
                const auto& o = options_[index];
                int physical = 0;
                int gpio = -1;
                if (componentId_.startsWith("pin_"))
                {
                    physical = componentId_.mid(4).toInt();
                    gpio = gpioForPhysicalPin(physical);
                }
                info_->setText(functionDetails(o));
                emit functionChanged(componentId_, o, gpio, physical);
            });
}
void FunctionSelectionColumn3::setComponent(const QString& id)
{
    /**Changes the component context without owning cross-component state.*/
    selector_->setObjectName("function_selector");
    componentId_ = id;
    populate();
}
void FunctionSelectionColumn3::populate()
{
    /**Loads valid functions and ROBO-PICO board templates for the selected pin.*/
    QSignalBlocker blocker(selector_);
    selector_->clear();
    options_.clear();
    options_.push_back(
        {"disabled", "Disabled", "Disabled", "Leaves this component without an active program function.", {}});
    if (componentId_.startsWith("pin_"))
    {
        const int gpio = gpioForPhysicalPin(componentId_.mid(4).toInt());
        if (gpio >= 0)
        {
            if (gpio == roboPicoGpio("neopixel_rgb"))
                options_.push_back({"robo.neopixel",
                                    "NeoPixel template",
                                    "ROBO-PICO",
                                    "WS2812 RGB LED template from the ROBO-PICO source of truth (GPIO18).",
                                    {}});
            if (gpio == roboPicoGpio("piezo_buzzer"))
                options_.push_back({"robo.buzzer",
                                    "Sound / buzzer template",
                                    "ROBO-PICO",
                                    "Piezo buzzer tone template from the ROBO-PICO source of truth (GPIO22).",
                                    {}});
            for (const auto& o : catalog_->functionsForGpio(gpio))
                options_.push_back(o);
        }
    }
    else
    {
        for (const auto& o : catalog_->specialFunctions(componentId_))
            options_.push_back(o);
    }
    for (const auto& o : options_)
    {
        selector_->addItem(o.name, o.id);
        const int index = selector_->count() - 1;
        selector_->setItemData(index, o.name, Qt::AccessibleTextRole);
        selector_->setItemData(index, o.id, Qt::AccessibleDescriptionRole);
    }
    selector_->setCurrentIndex(0);
    info_->setText(componentDetails(componentId_));
}
void FunctionSelectionColumn3::setSelectedFunction(const QString& functionId)
{
    /**Restores a persisted function selection without changing available options.*/
    const int idx = selector_->findData(functionId);
    if (idx >= 0)
    {
        const QSignalBlocker blocker(selector_);
        selector_->setCurrentIndex(idx);
    }
}
} // namespace pvd
