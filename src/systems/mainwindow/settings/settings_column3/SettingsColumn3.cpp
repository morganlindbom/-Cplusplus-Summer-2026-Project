// SettingsColumn3.cpp
#include "systems/mainwindow/settings/settings_column3/SettingsColumn3.hpp"
#include "systems/components/FunctionCatalog.hpp"
#include "systems/database/SqliteUtil.hpp"
#include "systems/generation/FunctionExecutionModel.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
namespace pvd
{
    namespace
    {
        QString settingDetails(const QString &key, const QString &label, const QString &defaultValue, const QString &help, const QStringList &values)
        {
            QString example = QString("Exempel: standardvärdet är %1.").arg(defaultValue.isEmpty() ? QStringLiteral("tomt") : defaultValue);
            QString alternative = values.isEmpty() ? QStringLiteral("Alternativ användning: välj ett annat värde om din hårdvara eller applikation kräver det.") : QString("Alternativ användning: %1.").arg(values.join(", "));
            QString warning = QStringLiteral("Varning: ändra värdet försiktigt och kontrollera att det stöds av RP2350A och vald funktion.");
            QString recommended = values.isEmpty() ? QStringLiteral("Använd standardvärdet som startpunkt och håll dig inom hårdvarans dokumenterade gränser.") : QString("Rekommenderade val: %1.").arg(values.join(", "));
            if (key == "frequency_hz")
                recommended = QStringLiteral("1–125000000 Hz; normalt 1000–250000 Hz.");
            else if (key == "duty_percent")
                recommended = QStringLiteral("0–100 %; normalt 1–99 % när signalen ska ha tydliga flankmarginaler.");
            else if (key == "wrap")
                recommended = QStringLiteral("1–65535; normalt 255–10000.");
            else if (key == "clock_divider")
                recommended = QStringLiteral("1.0–256.0; normalt 1.0–32.0.");
            else if (key == "counter_start")
                recommended = QStringLiteral("0–wrap; normalt 0.");
            else if (key == "system_clock_khz")
                recommended = QStringLiteral("0–300000 kHz; normalt 150000 kHz.");
            else if (key == "vreg_voltage")
                recommended = QStringLiteral("0.85–1.30 V; normalt 1.10 V.");
            else if (key == "watchdog_timeout_ms")
                recommended = QStringLiteral("1–3600000 ms; normalt 1000–10000 ms.");
            else if (key == "uart_baud")
                recommended = QStringLiteral("1200–4000000 baud; normalt 9600, 115200 eller 1000000 baud.");
            else if (key == "startup_delay_ms")
                recommended = QStringLiteral("0–60000 ms; normalt 0–2000 ms.");
            if (key == "frequency_hz")
            {
                example = QStringLiteral("Exempel: 25000 Hz ger en 25 kHz PWM-signal.");
                alternative = QStringLiteral("Alternativ användning: lägre frekvens passar LED och motorstyrning; högre frekvens passar ljud eller snabb reglering.");
                warning = QStringLiteral("Varning: för hög frekvens kan ge sämre upplösning eftersom räknaren får färre steg.");
            }
            else if (key == "duty_percent")
            {
                example = QStringLiteral("Exempel: 25 % ger signalen aktiv en fjärdedel av varje period.");
                alternative = QStringLiteral("Alternativ användning: använd 50 % som neutral testnivå eller ändra värdet dynamiskt i den färdiga koden.");
                warning = QStringLiteral("Varning: 0 % är alltid av och 100 % är alltid på; polaritetsinvertering kan vända resultatet.");
            }
            else if (key == "wrap")
            {
                example = QStringLiteral("Exempel: wrap 1000 ger 1001 räknarsteg och påverkar PWM-upplösningen.");
                alternative = QStringLiteral("Alternativ användning: höj wrap för finare duty-upplösning eller sänk den för högre möjlig frekvens.");
                warning = QStringLiteral("Varning: wrap måste kombineras med frekvens och dividerare; extrema värden kan ge avrundning eller för låg upplösning.");
            }
            else if (key == "clock_divider")
            {
                example = QStringLiteral("Exempel: divider 5.0 delar 125 MHz-klockan till ungefär 25 MHz.");
                alternative = QStringLiteral("Alternativ användning: använd Manual timing för exakt dividerare, eller Automatic timing för att låta frekvensen styra den.");
                warning = QStringLiteral("Varning: giltigt område är ungefär 1.0–256.0; för stor dividerare begränsar frekvens och upplösning.");
            }
            else if (key == "phase_correct")
            {
                example = QStringLiteral("Exempel: aktiverad fas-korrekt PWM räknar upp och sedan ned för symmetrisk puls.");
                alternative = QStringLiteral("Alternativ användning: avaktivera för vanlig snabbare edge-aligned PWM.");
                warning = QStringLiteral("Varning: fas-korrekt läge halverar ungefär maximal frekvens vid samma wrap och dividerare.");
            }
            else if (key == "invert_a" || key == "invert_b")
            {
                example = QStringLiteral("Exempel: aktivera för att göra en aktiv låg utgång av kanalen.");
                alternative = QStringLiteral("Alternativ användning: använd invertering när drivsteget eller extern gate har omvänd logik.");
                warning = QStringLiteral("Varning: invertering påverkar polariteten, inte duty-värdet; kontrollera alltid vad som är säkert för ansluten last.");
            }
            else if (key == "system_clock_khz")
            {
                example = QStringLiteral("Exempel: 150000 kHz ställer systemklockan till 150 MHz.");
                alternative = QStringLiteral("Alternativ användning: använd 0 för SDK-standard eller välj lägre klocka för lägre effekt och värme.");
                warning = QStringLiteral("Varning: överklockning kan kräva högre spänning, ge instabilitet och öka effektförbrukningen.");
            }
            else if (key == "vreg_voltage")
            {
                example = QStringLiteral("Exempel: 1.10 V är en balanserad standardnivå för normal systemklocka.");
                alternative = QStringLiteral("Alternativ användning: lägre spänning kan minska effekt; högre spänning kan behövas vid högre klockfrekvens.");
                warning = QStringLiteral("Varning: fel spänning kan orsaka instabilitet eller skada. Höj aldrig spänningen utan att verifiera RP2350A-gränserna.");
            }
            else if (key == "core1_enabled")
            {
                example = QStringLiteral("Exempel: aktiverad startar core 1 en separat bakgrundsloop.");
                alternative = QStringLiteral("Alternativ användning: lämna av för ett enklare single-core-program och mindre resursanvändning.");
                warning = QStringLiteral("Varning: core 0 och core 1 delar minne och resurser; synkronisering krävs i egen applikationskod.");
            }
            else if (key == "watchdog_enabled" || key == "watchdog_timeout_ms")
            {
                example = QStringLiteral("Exempel: 2000 ms återställer systemet om programmet inte matar watchdog i tid.");
                alternative = QStringLiteral("Alternativ användning: stäng av under felsökning eller använd kortare timeout i säkerhetskritisk drift.");
                warning = QStringLiteral("Varning: en aktiv watchdog kan starta om systemet under långvariga operationer eller debugging.");
            }
            else if (key == "stdio_usb" || key == "stdio_uart")
            {
                example = QStringLiteral("Exempel: USB stdio aktiverad gör att loggar kan läsas via USB-anslutningen.");
                alternative = QStringLiteral("Alternativ användning: UART stdio passar för seriell konsol när USB inte är tillgänglig.");
                warning = QStringLiteral("Varning: stdio-kanaler använder resurser och kan påverka starttid eller vilka pinnar/periferier som är lediga.");
            }
            else if (key == "uart_baud")
            {
                example = QStringLiteral("Exempel: 115200 baud är ett vanligt värde för seriell kommunikation.");
                alternative = QStringLiteral("Alternativ användning: välj 9600 för långsammare utrustning eller högre hastighet om mottagaren stöder det.");
                warning = QStringLiteral("Varning: sändare och mottagare måste använda samma baud rate, annars blir data oläsbar.");
            }
            else if (key == "startup_delay_ms")
            {
                example = QStringLiteral("Exempel: 1000 ms väntar en sekund efter initiering innan applikationen fortsätter.");
                alternative = QStringLiteral("Alternativ användning: använd för att ge USB, sensorer eller externa kretsar tid att starta.");
                warning = QStringLiteral("Varning: en lång fördröjning kan se ut som att kortet hängt sig vid uppstart.");
            }
            return QString("<b>%1</b><br><br>%2<br><br><b>Rekommenderat min/max</b><br>%3<br><br><b>Exempel</b><br>%4<br><br><b>Alternativ användning</b><br>%5<br><br><b>Varning</b><br>%6").arg(label, help, recommended, example, alternative, warning);
        }
    }
    SettingsColumn3::SettingsColumn3(const QString &db, FunctionCatalog *catalog, QWidget *parent) : QWidget(parent), catalog_(catalog)
    {
        auto *l = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Settings"));
        auto *host = new QWidget(this);
        form_ = new QFormLayout(host);
        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setWidget(host);
        l->addWidget(scroll, 2);
        l->addWidget(makePanelTitle("Function Information", this));
        info_ = new QLabel("Select a configured function.", this);
        info_->setWordWrap(true);
        info_->setAlignment(Qt::AlignTop);
        l->addWidget(info_, 1);
    }
    void SettingsColumn3::clearForm()
    {
        while (form_->rowCount() > 0)
            form_->removeRow(0);
    }
    void SettingsColumn3::addSetting(const QString &key, const QString &label, const QString &type, const QString &defaultValue, const QString &help, const QStringList &values)
    {
        const QString value = current_.value(key, defaultValue);
        const QString details = settingDetails(key, label, defaultValue, help, values);
        QWidget *editor = nullptr;
        auto *labelButton = new QPushButton(label);
        labelButton->setFlat(true);
        labelButton->setCursor(Qt::PointingHandCursor);
        labelButton->setStyleSheet("QPushButton { text-align: left; border: none; padding: 2px; color: palette(text); } QPushButton:hover { text-decoration: underline; }");
        connect(labelButton, &QPushButton::clicked, this, [this, details]()
                { info_->setText(details); });
        if (type == "bool")
        {
            auto *c = new QCheckBox();
            c->setObjectName("setting_" + key);
            c->setAccessibleName(label);
            c->setAccessibleDescription(help);
            c->setChecked(value == "true" || value == "Enabled");
            connect(c, &QCheckBox::toggled, this, [this, key, details](bool v)
                    {emit settingChanged(componentId_,key,v?"true":"false");info_->setText(details); });
            editor = c;
        }
        else if (type == "int")
        {
            auto *c = new QComboBox();
            c->setEditable(true);
            QStringList choices;
            if (key == "frequency_hz")
                choices = {"1000", "5000", "10000", "25000", "50000", "100000", "250000", "Custom..."};
            else if (key == "duty_percent")
                choices = {"0", "10", "25", "50", "75", "90", "100", "Custom..."};
            else if (key == "system_clock_khz")
                choices = {"0", "125000", "133000", "150000", "200000", "250000", "300000", "Custom..."};
            else if (key == "watchdog_timeout_ms")
                choices = {"100", "500", "1000", "2000", "5000", "10000", "60000", "Custom..."};
            else if (key == "uart_baud")
                choices = {"9600", "19200", "38400", "57600", "115200", "1000000", "Custom..."};
            else if (key == "startup_delay_ms")
                choices = {"0", "100", "500", "1000", "2000", "5000", "Custom..."};
            else if (key == "counter_start")
                choices = {"0", "1", "10", "100", "500", "1000", "Custom..."};
            else
                choices = {"0", "1", "10", "50", "100", "500", "1000", "5000", "10000", "50000", "100000", "Custom..."};
            if (!value.isEmpty() && !choices.contains(value))
                choices.prepend(value);
            c->addItems(choices);
            c->setCurrentText(value);
            connect(c, &QComboBox::currentTextChanged, this, [this, key, details](const QString &v)
                    {if(v=="Custom...")return;emit settingChanged(componentId_,key,v);info_->setText(details); });
            connect(c, qOverload<int>(&QComboBox::activated), this, [c](int index)
                    {if(index==c->count()-1&&c->itemText(index)=="Custom..."){c->setEditText(QString{});c->lineEdit()->setFocus();} });
            editor = c;
        }
        else if (type == "combo")
        {
            auto *c = new QComboBox();
            c->addItems(values);
            const int i = c->findText(value);
            if (i >= 0)
                c->setCurrentIndex(i);
            connect(c, &QComboBox::currentTextChanged, this, [this, key, details](const QString &v)
                    {current_[key]=v;emit settingChanged(componentId_,key,v);info_->setText(details);if(key=="operation_mode"){FunctionSelection updated;updated.componentId=componentId_;updated.functionId=functionId_;updated.functionName=functionName_;updated.gpio=gpio_;updated.settings=current_;showSelection(updated);} });
            editor = c;
        }
        else
        {
            auto *c = new QComboBox();
            c->setEditable(true);
            QStringList choices;
            if (key == "clock_divider")
                choices = {"1.0", "2.0", "4.0", "5.0", "8.0", "16.0", "32.0", "64.0", "128.0", "256.0", "Custom..."};
            else if (key == "pio_program")
                choices = {"Mogge", "Custom..."};
            else
                choices = {"Custom..."};
            if (!value.isEmpty() && !choices.contains(value))
                choices.prepend(value);
            c->addItems(choices);
            c->setCurrentText(value);
            connect(c, &QComboBox::currentTextChanged, this, [this, key, details](const QString &v)
                    {if(v=="Custom...")return;emit settingChanged(componentId_,key,v);info_->setText(details); });
            connect(c, qOverload<int>(&QComboBox::activated), this, [c](int index)
                    {if(index==c->count()-1&&c->itemText(index)=="Custom..."){c->setEditText(QString{});c->lineEdit()->setFocus();} });
            editor = c;
        }
        editor->setToolTip(details);
        labelButton->setToolTip(details);
        form_->addRow(labelButton, editor);
    }
    void SettingsColumn3::showSelection(const FunctionSelection &selection)
    {
        componentId_ = selection.componentId;
        functionId_ = selection.functionId;
        functionName_ = selection.functionName;
        gpio_ = selection.gpio;
        current_ = selection.settings;
        clearForm();
        if (core1Enabled_ && FunctionExecutionModel::supportsCoreSelection(functionId_))
            addSetting("execution_core", "Execution core", "combo", "Core 0", FunctionExecutionModel::coreSelectionReason(functionId_), {"Core 0", "Core 1"});
        else if (core1Enabled_ && functionId_ != "rp2350a.configure" && functionId_ != "debug_probe.cmsis_dap")
        {
            auto *coreReason = new QLabel("Core 0 only — " + FunctionExecutionModel::coreSelectionReason(functionId_), this);
            coreReason->setWordWrap(true);
            form_->addRow("Execution core", coreReason);
        }
        const QString db = catalog_->functionDatabase(selection.functionId);
        if (!db.isEmpty())
        {
            info_->setText(selection.functionName + "\n\n" + SqliteUtil::metadata(db, "description"));
            for (const auto &row : SqliteUtil::rows(db, "SELECT setting_key,label,editor_type,default_value,help_text,enum_values FROM settings ORDER BY sort_order"))
                addSetting(row.value("setting_key").toString(), row.value("label").toString(), row.value("editor_type").toString(), row.value("default_value").toString(), row.value("help_text").toString(), row.value("enum_values").toString().split('|', Qt::SkipEmptyParts));
            if (selection.functionId.startsWith("pwm"))
            {
                addSetting("timing_mode", "Timing mode", "combo", "Automatic", "Automatic uses frequency and wrap; Manual uses the divider directly.", {"Automatic", "Manual"});
                addSetting("wrap", "Counter top / wrap", "int", "1000", "PWM counter TOP value. Resolution is wrap + 1 steps.");
                addSetting("clock_divider", "Clock divider", "text", "1.0", "PWM clock divider from 1.0 to 256.0. Used in Manual timing mode.");
                addSetting("divider_mode", "Divider mode", "combo", "Free-running", "How the PWM clock divider advances.", {"Free-running", "B rises", "B high"});
                addSetting("phase_correct", "Phase-correct", "bool", "false", "Use dual-slope phase-correct PWM.");
                addSetting("invert_a", "Invert channel A", "bool", "false", "Invert PWM output polarity on channel A.");
                addSetting("invert_b", "Invert channel B", "bool", "false", "Invert PWM output polarity on channel B.");
                addSetting("counter_start", "Counter start", "int", "0", "Initial PWM counter value after configuration.");
            }
            if (selection.functionId.startsWith("pio"))
            {
                if (selection.settings.value("operation_mode", "Single Pin") != "Single Pin")
                {
                    addSetting("pin_count", "Pin count", "int", "8", "Number of consecutive pins controlled by the PIO state machine.");
                    addSetting("base_pin", "Base GPIO", "int", QString::number(selection.gpio), "First GPIO in the PIO pin range.");
                    addSetting("shift_direction", "Shift direction", "combo", "Right", "Bit shift direction.", {"Left", "Right"});
                    addSetting("autopush", "Auto-push", "bool", "false", "Automatically push RX data at the selected threshold.");
                    addSetting("autopull", "Auto-pull", "bool", "false", "Automatically pull TX data at the selected threshold.");
                    addSetting("shift_threshold", "Shift threshold", "int", "32", "Auto-push/auto-pull threshold in bits.");
                    addSetting("fifo_join", "FIFO join", "combo", "None", "Optional FIFO join mode.", {"None", "RX", "TX"});
                    addSetting("clock_divider", "Clock divider", "text", "1.0", "PIO state-machine clock divider.");
                }
                addSetting("dead_time_cycles", "Dead time (PIO cycles)", "int", "0", "Extra PIO delay inserted in the program loop. 0 disables dead time.");
            }
        }
        else if (selection.functionId == "rp2350a.configure")
        {
            info_->setText("RP2350A\n\nProcessor and system-level configuration.");
            addSetting("enabled", "Enabled", "bool", "true", "Enable RP2350A system configuration.");
            addSetting("system_clock_khz", "System clock (kHz)", "int", "150000", "RP2350A system clock frequency. Set 0 to keep the SDK default.");
            addSetting("vreg_voltage", "Core voltage (V)", "combo", "1.10", "Core voltage used before changing the system clock.", {"0.85", "0.90", "0.95", "1.00", "1.05", "1.10", "1.15", "1.20", "1.25", "1.30"});
            addSetting("core1_enabled", "Enable core 1", "bool", "false", "Launch a background loop on the second RP2350A core.");
            addSetting("watchdog_enabled", "Enable watchdog", "bool", "false", "Enable the RP2350A watchdog timer.");
            addSetting("watchdog_timeout_ms", "Watchdog timeout (ms)", "int", "2000", "Watchdog timeout before an automatic reset.");
            addSetting("stdio_usb", "USB stdio", "bool", "true", "Enable USB stdio in the generated project.");
            addSetting("stdio_uart", "UART stdio", "bool", "false", "Enable UART stdio in the generated project.");
            addSetting("uart_baud", "UART baud rate", "int", "115200", "UART stdio baud rate.");
            addSetting("startup_delay_ms", "Startup delay (ms)", "int", "0", "Optional delay after SDK initialization.");
        }
        else if (selection.functionId == "gpio.input")
        {
            info_->setText("GPIO Input\n\nSIO input configuration.");
            addSetting("pull", "Pull resistor", "combo", "None", "Internal GPIO pull resistor.", {"None", "Pull-up", "Pull-down"});
            addSetting("debounce_ms", "Debounce (ms)", "int", "0", "Optional software debounce interval.");
        }
        else if (selection.functionId == "gpio.output")
        {
            info_->setText("GPIO Output\n\nSIO output configuration.");
            addSetting("initial_state", "Initial state", "combo", "Low", "Initial output level.", {"Low", "High"});
            addSetting("blink_enabled", "Blink output", "bool", "false", "Toggle this GPIO periodically on the selected execution core.");
            addSetting("blink_interval_ms", "Blink interval (ms)", "int", "500", "Time between GPIO output state changes.");
            addSetting("drive_strength", "Drive strength (mA)", "combo", "4", "GPIO output drive strength.", {"2", "4", "8", "12"});
            addSetting("slew_rate", "Slew rate", "combo", "Fast", "GPIO output slew rate.", {"Slow", "Fast"});
        }
        else if (selection.functionId == "bootsel.use_button")
        {
            info_->setText("BOOTSEL\n\nUses the physical system BOOTSEL button.");
            addSetting("mode", "Mode", "combo", "Press", "BOOTSEL interaction mode.", {"Press", "Hold", "Toggle"});
            addSetting("debounce_ms", "Debounce (ms)", "int", "50", "Software debounce interval.");
        }
        else if (selection.functionId == "onboard_led.output")
        {
            info_->setText("Onboard LED\n\nControls the wireless LED through CYW43.");
            addSetting("initial_state", "Initial state", "combo", "Off", "Initial LED state.", {"Off", "On"});
            addSetting("blink_enabled", "Blink LED", "bool", "false", "Run non-blocking LED blinking on the selected execution core.");
            addSetting("blink_interval_ms", "Blink interval (ms)", "int", "500", "Time between LED state changes.");
        }
        else if (selection.functionId == "wireless.cyw43")
        {
            info_->setText("Wireless\n\nConfigures Wi-Fi/Bluetooth.");
            addSetting("wifi_mode", "Wi-Fi mode", "combo", "Station", "Wireless operating mode.", {"Station", "Access Point", "Station + AP"});
            addSetting("ssid", "SSID", "text", "", "Wireless network SSID.");
            addSetting("password", "Password", "text", "", "Wireless network password.");
        }
        else if (selection.functionId == "debug_probe.cmsis_dap")
        {
            info_->setText("Debug Probe\n\nConfigures CMSIS-DAP and OpenOCD.");
            addSetting("adapter_speed", "Adapter speed (kHz)", "int", "5000", "OpenOCD adapter speed.");
            addSetting("reset_method", "Reset method", "combo", "run reset", "Debug-session reset method.", {"run reset", "halt reset", "none"});
        }
        else if (selection.functionId == "robo.neopixel")
        {
            info_->setText("ROBO-PICO NeoPixel template\n\nWS2812 RGB LED on GPIO18 from robo_pico_source_of_truth.jsonc. Pixel count 2 supports two serially connected LEDs.");
            addSetting("enabled", "Enabled", "bool", "true", "Enable the NeoPixel template.");
            addSetting("pixel_count", "Pixel count", "int", "2", "Number of daisy-chained WS2812 pixels.");
            addSetting("red", "Pixel 1 red", "int", "0", "Pixel 1 red component from 0 to 255.");
            addSetting("green", "Pixel 1 green", "int", "32", "Pixel 1 green component from 0 to 255.");
            addSetting("blue", "Pixel 1 blue", "int", "0", "Pixel 1 blue component from 0 to 255.");
            addSetting("red2", "Pixel 2 red", "int", "32", "Pixel 2 red component from 0 to 255.");
            addSetting("green2", "Pixel 2 green", "int", "0", "Pixel 2 green component from 0 to 255.");
            addSetting("blue2", "Pixel 2 blue", "int", "0", "Pixel 2 blue component from 0 to 255.");
            addSetting("brightness", "Brightness", "int", "64", "Global brightness from 0 to 255.");
        }
        else if (selection.functionId == "robo.buzzer")
        {
            info_->setText("ROBO-PICO sound template\n\nPiezo buzzer on GPIO22 from robo_pico_source_of_truth.jsonc.");
            addSetting("enabled", "Enabled", "bool", "true", "Enable the sound template.");
            addSetting("frequency_hz", "Tone frequency (Hz)", "int", "1000", "Tone frequency for the piezo buzzer.");
            addSetting("duration_ms", "Tone duration (ms)", "int", "250", "Tone length for each non-blocking pulse.");
            addSetting("repeat_interval_ms", "Tone repeat interval (ms)", "int", "2000", "Time from one tone start to the next tone start.");
        }
        else
        {
            info_->setText(selection.functionName);
            addSetting("enabled", "Enabled", "bool", "true", "Enables this function in generated code.");
        }
    }
    void SettingsColumn3::setCore1Enabled(bool enabled)
    {
        /**Controls whether per-function execution-core selection is available.*/
        core1Enabled_ = enabled;
    }
}
