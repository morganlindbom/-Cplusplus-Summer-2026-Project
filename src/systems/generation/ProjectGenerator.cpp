// ProjectGenerator.cpp
#include "systems/generation/ProjectGenerator.hpp"
#include "systems/generation/BoardStartupSanitation.hpp"
#include "systems/generation/FunctionExecutionModel.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>
namespace
{
QString explainGeneratedMain(QString source)
{
    /**Adds beginner-facing explanations to generated SDK and peripheral calls.*/
    source.replace("    PIO neopixel_pio = pio0;\n", "    // Select PIO block 0. PIO handles the precise NeoPixel "
                                                     "timing in hardware.\n    PIO neopixel_pio = pio0;\n");
    source.replace("    uint neopixel_offset = pio_add_program(neopixel_pio, &robo_neopixel_program);\n",
                   "    // Copy the PIO program into the PIO instruction memory.\n    uint neopixel_offset = "
                   "pio_add_program(neopixel_pio, &robo_neopixel_program);\n");
    source.replace("    pio_sm_config neopixel_cfg = robo_neopixel_program_get_default_config(neopixel_offset);\n",
                   "    // Create the state-machine configuration defined by the PIO program.\n    pio_sm_config "
                   "neopixel_cfg = robo_neopixel_program_get_default_config(neopixel_offset);\n");
    source.replace("    sm_config_set_sideset_pins(&neopixel_cfg, ",
                   "    // Connect the PIO side-set signal to the selected data GPIO.\n    "
                   "sm_config_set_sideset_pins(&neopixel_cfg, ");
    source.replace("    sm_config_set_out_shift(&neopixel_cfg, false, true, 24);\n",
                   "    // Shift 24-bit color values from the TX FIFO into the PIO program.\n    "
                   "sm_config_set_out_shift(&neopixel_cfg, false, true, 24);\n");
    source.replace("    sm_config_set_fifo_join(&neopixel_cfg, PIO_FIFO_JOIN_TX);\n",
                   "    // Join the TX FIFO so the PIO state machine can receive more data.\n    "
                   "sm_config_set_fifo_join(&neopixel_cfg, PIO_FIFO_JOIN_TX);\n");
    source.replace("    sm_config_set_clkdiv(&neopixel_cfg, 15.625f);\n",
                   "    // Set the PIO clock divider for the NeoPixel timing protocol.\n    "
                   "sm_config_set_clkdiv(&neopixel_cfg, 15.625f);\n");
    source.replace("    pio_gpio_init(neopixel_pio, ",
                   "    // Hand the selected GPIO over to the PIO peripheral.\n    pio_gpio_init(neopixel_pio, ");
    source.replace("    pio_sm_set_consecutive_pindirs(neopixel_pio, 0, ",
                   "    // Configure one consecutive GPIO as an output for state machine 0.\n    "
                   "pio_sm_set_consecutive_pindirs(neopixel_pio, 0, ");
    source.replace("    pio_sm_init(neopixel_pio, 0, neopixel_offset, &neopixel_cfg);\n",
                   "    // Load the program and configuration into state machine 0.\n    pio_sm_init(neopixel_pio, 0, "
                   "neopixel_offset, &neopixel_cfg);\n");
    source.replace("    pio_sm_set_enabled(neopixel_pio, 0, true);\n",
                   "    // Start the state machine. It now waits for color data in the TX FIFO.\n    "
                   "pio_sm_set_enabled(neopixel_pio, 0, true);\n");
    source.replace(
        "    sleep_us(80);\n",
        "    // Keep the data line low long enough for the NeoPixel to latch the colors.\n    sleep_us(80);\n");
    source.replace(QRegularExpression("    gpio_set_function\\((\\d+), GPIO_FUNC_PWM\\);\\n"),
                   "    // Select the PWM peripheral for GPIO \\1.\n    gpio_set_function(\\1, GPIO_FUNC_PWM);\n");
    source.replace(QRegularExpression("    pwm_gpio_to_slice_num\\((\\d+)\\);\\n"),
                   "    // Find the PWM hardware slice connected to GPIO \\1.\n    pwm_gpio_to_slice_num(\\1);\n");
    source.replace("    pwm_config buzzer_cfg_",
                   "    // Create a configuration object for this buzzer's PWM slice.\n    pwm_config buzzer_cfg_");
    source.replace("    pwm_config_set_clkdiv(&buzzer_cfg_",
                   "    // Set the PWM timing divider used to create the requested frequency.\n    "
                   "pwm_config_set_clkdiv(&buzzer_cfg_");
    source.replace("    pwm_config_set_wrap(&buzzer_cfg_",
                   "    // Define the end of one PWM counting period.\n    pwm_config_set_wrap(&buzzer_cfg_");
    source.replace("    pwm_init(buzzer_slice_",
                   "    // Apply the PWM configuration and start the slice.\n    pwm_init(buzzer_slice_");
    source.replace(
        "    stdio_init_all();\n",
        "    // Enable USB/UART standard I/O so the Pico can communicate with a computer.\n    stdio_init_all();\n");
    source.replace(QRegularExpression("    // Send the first pixel color\\.[\\s\\S]*?    sleep_us\\(80\\);\\n"),
                   "    // Color data is submitted after initialization by the selected core runtime handler.\n");
    source.replace(
        QRegularExpression("    // A level of 500 out of 1000 gives a 50% duty cycle\\.\\n"
                           "    pwm_set_gpio_level\\((\\d+), 500\\);\\n"
                           "(?:    // Keep the tone active[\\s\\S]*?    pwm_set_gpio_level\\(\\1, 0\\);\\n)?"),
        "    // Keep the buzzer silent until its selected runtime handler starts the tone.\n"
        "    pwm_set_gpio_level(\\1, 0);\n");
    source.replace(QRegularExpression("(    // Configure the pin as an output and set its initial level\\.\\n"
                                      "    gpio_init\\((\\d+)\\);\\n"
                                      "    gpio_set_dir\\(\\2, GPIO_OUT\\);\\n)"
                                      "    gpio_put\\(\\2, (?:true|false)\\);\\n"),
                   "\\1    // Use a safe inactive level until the selected runtime handler applies Settings.\n"
                   "    gpio_put(\\2, false);\n");
    return source;
}

void emitRuntimeBody(QTextStream& o, const pvd::FunctionSelection& sel, bool core1)
{
    const auto setting = [&](const QString& key, const QString& fallback) { return sel.settings.value(key, fallback); };
    if (sel.functionId == "onboard_led.output")
    {
        const bool ledOn = setting("initial_state", "Off") == "On";
        const bool blink = setting("blink_enabled", "false") == "true";
        const int interval = std::clamp(setting("blink_interval_ms", "500").toInt(), 10, 60000);
        o << "    // " << (core1 ? "Core 1" : "Core 0") << " owns the onboard LED runtime behaviour.\n"
          << "    static bool led_state = " << (ledOn ? "true" : "false") << ";\n"
          << "    static bool state_applied = false;\n"
          << "    static uint64_t last_toggle_us = 0;\n"
          << "    const uint64_t now_us = time_us_64();\n"
          << "    bool update_required = !state_applied;\n";
        if (blink)
            o << "    if (state_applied && now_us - last_toggle_us >= " << (interval * 1000) << "u) {\n"
              << "        led_state = !led_state;\n"
              << "        update_required = true;\n"
              << "    }\n";
        if (core1)
            o << "    if (update_required && multicore_fifo_wready()) {\n"
              << "        // Core 1 owns timing/state; Core 0 owns the CYW43 driver call.\n"
              << "        multicore_fifo_push_blocking(led_state ? 1u : 0u);\n"
              << "        state_applied = true;\n"
              << "        last_toggle_us = now_us;\n"
              << "    }\n";
        else
            o << "    if (update_required) {\n"
              << "        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);\n"
              << "        state_applied = true;\n"
              << "        last_toggle_us = now_us;\n"
              << "    }\n";
    }
    else if (sel.functionId == "gpio.output")
    {
        const bool high = setting("initial_state", "Low") == "High";
        const bool blink = setting("blink_enabled", "false") == "true";
        const int interval = std::clamp(setting("blink_interval_ms", "500").toInt(), 10, 60000);
        o << "    // " << (core1 ? "Core 1" : "Core 0") << " owns GPIO" << sel.gpio << " runtime updates.\n"
          << "    static bool gpio_state = " << (high ? "true" : "false") << ";\n"
          << "    static bool state_applied = false;\n"
          << "    static uint64_t last_toggle_us = 0;\n"
          << "    const uint64_t now_us = time_us_64();\n"
          << "    if (!state_applied";
        if (blink)
            o << " || now_us - last_toggle_us >= " << (interval * 1000) << "u";
        o << ") {\n";
        if (blink)
            o << "        if (state_applied) gpio_state = !gpio_state;\n";
        o << "        gpio_put(" << sel.gpio << ", gpio_state);\n"
          << "        state_applied = true;\n"
          << "        last_toggle_us = now_us;\n"
          << "    }\n";
    }
    else if (sel.functionId == "robo.buzzer")
    {
        const int duration = std::clamp(setting("duration_ms", "250").toInt(), 0, 60000);
        const int repeat = std::clamp(setting("repeat_interval_ms", "2000").toInt(), duration, 600000);
        o << "    // Start the configured tone on " << (core1 ? "Core 1" : "Core 0")
          << " without blocking the dispatcher.\n"
          << "    static bool tone_active = false;\n"
          << "    static uint64_t tone_start_us = 0;\n"
          << "    static uint64_t last_tone_us = 0;\n"
          << "    const uint64_t now_us = time_us_64();\n"
          << "    if (!tone_active && (last_tone_us == 0 || now_us - last_tone_us >= " << (repeat * 1000) << "u)) {\n"
          << "        pwm_set_gpio_level(" << sel.gpio << ", 500);\n"
          << "        tone_start_us = now_us;\n"
          << "        tone_active = true;\n"
          << "    }\n";
        if (duration > 0)
            o << "    if (tone_active && now_us - tone_start_us >= " << (duration * 1000) << "u) {\n"
              << "        pwm_set_gpio_level(" << sel.gpio << ", 0);\n"
              << "        tone_active = false;\n"
              << "        last_tone_us = now_us;\n"
              << "    }\n";
    }
    else if (sel.functionId == "robo.neopixel")
    {
        const int count = std::clamp(setting("pixel_count", "2").toInt(), 1, 256);
        const int brightness = std::clamp(setting("brightness", "64").toInt(), 0, 255);
        const int r = std::clamp(setting("red", "0").toInt(), 0, 255) * brightness / 255;
        const int g = std::clamp(setting("green", "32").toInt(), 0, 255) * brightness / 255;
        const int b = std::clamp(setting("blue", "0").toInt(), 0, 255) * brightness / 255;
        const int r2 = std::clamp(setting("red2", "32").toInt(), 0, 255) * brightness / 255;
        const int g2 = std::clamp(setting("green2", "0").toInt(), 0, 255) * brightness / 255;
        const int b2 = std::clamp(setting("blue2", "0").toInt(), 0, 255) * brightness / 255;
        const QString pio = setting("pio_block", "pio0");
        const int sm = std::clamp(setting("pio_state_machine", "0").toInt(), 0, 3);
        o << "    // Submit the selected colors once from " << (core1 ? "Core 1" : "Core 0")
          << "; PIO then drives the waveform.\n"
          << "    static bool colors_sent = false;\n"
          << "    if (colors_sent) return;\n"
          << "    pio_sm_put_blocking(" << pio << ", " << sm << ", ((uint32_t)" << g << " << 24) | ((uint32_t)" << r
          << " << 16) | ((uint32_t)" << b << " << 8));\n";
        if (count > 1)
            o << "    pio_sm_put_blocking(" << pio << ", " << sm << ", ((uint32_t)" << g2 << " << 24) | ((uint32_t)"
              << r2 << " << 16) | ((uint32_t)" << b2 << " << 8));\n";
        if (count > 2)
            o << "    for (uint pixel = 2; pixel < " << count << "; ++pixel) pio_sm_put_blocking(" << pio << ", " << sm
              << ", 0);\n";
        o << "    sleep_us(80);\n"
          << "    colors_sent = true;\n";
    }
}

bool writeFile(const QString& path, const QString& text, QString* error)
{
    /**Writes a complete generated project file.*/
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        if (error)
            *error = f.errorString();
        return false;
    }
    f.write(text.toUtf8());
    return true;
}
} // namespace
namespace pvd
{
bool ProjectGenerator::generate(ApplicationState* state, const QHash<QString, QString>& pioPrograms, QString* error)
{
    /**Generates a runnable Pico SDK project from current project state.*/
    if (!state || state->projectPath.isEmpty())
    {
        if (error)
            *error = "Create or open a project first.";
        return false;
    }
    QString validation;
    if (!validate(*state, &validation))
    {
        if (error)
            *error = validation;
        return false;
    }
    const QString out = QDir(state->projectPath).filePath("generated");
    QDir().mkpath(out);
    QStringList files;
    const QString ext = state->language == "C" ? "c" : "cpp";
    const QString main = QDir(out).filePath("main." + ext);
    if (!writeFile(main, explainGeneratedMain(generateMain(*state)), error))
        return false;
    files << main;
    QStringList pioFiles;
    QHash<QString, QString> normalizedPrograms;
    const QString fallbackProgram =
        pioPrograms.value("Mogge", pioPrograms.isEmpty() ? QString() : pioPrograms.constBegin().value());
    const bool boardSanitationNeeded = BoardStartupSanitation::needsRoboPicoNeoPixelClear(*state);
    for (const auto& sel : state->selections)
        if (sel.functionId == "robo.neopixel" &&
            (sel.settings.value("enabled", "true") == "true" || boardSanitationNeeded))
        {
            const QString p = QDir(out).filePath("robo_neopixel.pio");
            const QString source = "; robo_neopixel.pio\n"
                                   "; This PIO program sends one bit at a time to a WS2812/NeoPixel LED.\n"
                                   "; PIO is a small, deterministic processor inside the RP2350.\n"
                                   "; Each instruction normally takes one PIO clock cycle.\n"
                                   "; The comments below explain the timing-sensitive protocol.\n\n"
                                   "; Give this program a name. The C/C++ code refers to robo_neopixel_program.\n"
                                   ".program robo_neopixel\n\n"
                                   "; Use one side-set bit to control the NeoPixel data output pin.\n"
                                   "; side 1 drives the signal high; side 0 drives it low.\n"
                                   ".side_set 1\n\n"
                                   "; The main loop starts here. After reaching .wrap, execution returns here.\n"
                                   ".wrap_target\n\n"
                                   "; Read one bit from the output shift register into scratch register X.\n"
                                   "; The [2] delay keeps the signal high/low for the required protocol time.\n"
                                   "bitloop:\n"
                                   "    out x, 1 side 0 [2]\n\n"
                                   "; If the bit was zero, jump to do_zero and use the shorter high pulse.\n"
                                   "; The !x condition means: jump when X contains zero.\n"
                                   "    jmp !x do_zero side 1 [1]\n\n"
                                   "; If the bit was one, jump back to bitloop with the longer high pulse.\n"
                                   "; This difference in pulse length is how NeoPixels distinguish 0 from 1.\n"
                                   "    jmp bitloop side 1 [4]\n\n"
                                   "; Zero bits have a shorter high pulse and then return to the main loop.\n"
                                   "do_zero:\n"
                                   "    nop side 0 [4]\n\n"
                                   "; End of the loop. The assembler wraps execution back to .wrap_target.\n"
                                   ".wrap\n";
            if (!writeFile(p, source, error))
                return false;
            files << p;
            pioFiles << QFileInfo(p).fileName();
            break;
        }
    for (const auto& sel : state->selections)
    {
        if (!sel.functionId.startsWith("pio") || sel.settings.value("enabled", "true") != "true")
            continue;
        const QString desired = sel.settings.value("pio_program", "Mogge");
        if (normalizedPrograms.contains(desired))
            continue;
        QString source = pioPrograms.value(desired, fallbackProgram);
        if (source.isEmpty())
            continue;
        const QString original = pioPrograms.contains(desired) ? desired : "Mogge";
        if (original != desired)
        {
            source.replace("; " + original + ".pio", "; " + desired + ".pio");
            source.replace(".program " + original, ".program " + desired);
        }
        const int dead = std::clamp(sel.settings.value("dead_time_cycles", "0").toInt(), 0, 31);
        if (dead > 0 && !source.contains("; PVD dead time"))
        {
            const QString marker = QString("    nop [%1] ; PVD dead time\n").arg(dead);
            const int wrap = source.indexOf(".wrap\n");
            if (wrap >= 0)
                source.insert(wrap, marker);
            else
                source.append("\n" + marker);
        }
        normalizedPrograms.insert(desired, source);
    }
    for (auto it = normalizedPrograms.cbegin(); it != normalizedPrograms.cend(); ++it)
    {
        const QString p = QDir(out).filePath(it.key() + ".pio");
        if (!writeFile(p, it.value(), error))
            return false;
        files << p;
        pioFiles << QFileInfo(p).fileName();
    }
    const QString lwip = QDir(out).filePath("lwipopts.h");
    if (!writeFile(lwip,
                   "#pragma once\n#define NO_SYS 1\n#define LWIP_SOCKET 0\n#define LWIP_NETCONN 0\n#define LWIP_RAW "
                   "1\n#define LWIP_ARP 1\n#define LWIP_ETHERNET 1\n#define LWIP_ICMP 1\n#define LWIP_DHCP 1\n#define "
                   "LWIP_DNS 1\n#define LWIP_TCP 1\n#define LWIP_UDP 1\n#define LWIP_TCP_KEEPALIVE 1\n#define "
                   "LWIP_NETIF_HOSTNAME 1\n#define LWIP_STATS 0\n#define LWIP_PROVIDE_ERRNO 1\n#define "
                   "LWIP_TIMEVAL_PRIVATE 0\n",
                   error))
        return false;
    files << lwip;
    const QString cmake = QDir(out).filePath("CMakeLists.txt");
    if (!writeFile(cmake, generateCMake(*state, pioFiles), error))
        return false;
    files << cmake;
    const QString import = QDir(out).filePath("pico_sdk_import.cmake");
    if (!writeFile(import, "# pico_sdk_import.cmake\ninclude(\"$ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake\")\n",
                   error))
        return false;
    files << import;
    const QString info = QDir(out).filePath("information.md");
    if (!writeFile(info,
                   "<!-- information.md -->\n# Generated by Pico Visual Designer reconstruction\n\nTarget: pico2_w\n",
                   error))
        return false;
    files << info;
    state->generatedFiles = files;
    return true;
}
bool ProjectGenerator::validate(const ApplicationState& state, QString* report)
{
    /**Validates project inputs and the ownership policy of generated hardware resources.*/
    QStringList issues;
    if (state.projectPath.isEmpty())
        issues << "Project path is empty.";
    if (state.projectName.trimmed().isEmpty())
        issues << "Project name is empty.";
    if (state.selections.isEmpty())
        issues << "No hardware functions selected.";

    const auto rpSelection = state.selections.value("rp2350a");
    const bool core1Enabled = rpSelection.functionId == "rp2350a.configure" &&
                              rpSelection.settings.value("enabled", "true") == "true" &&
                              rpSelection.settings.value("core1_enabled", "false") == "true";
    QHash<int, QString> gpioOwners;
    QHash<QString, QString> exclusiveOwners;
    QHash<QString, QPair<QString, QString>> sharedConfigurations;
    QHash<QString, int> pioInstructionWords;

    auto conflict = [&](const QString& resource, const QString& first, const QString& second, const QString& reason)
    { issues << QString("Resource conflict: '%1' and '%2' both require %3. %4").arg(first, second, resource, reason); };
    auto claimExclusive = [&](const QString& resource, const QString& owner, const QString& reason)
    {
        if (exclusiveOwners.contains(resource))
            conflict(resource, exclusiveOwners.value(resource), owner, reason);
        else
            exclusiveOwners.insert(resource, owner);
    };
    auto claimGpio = [&](int gpio, const QString& owner)
    {
        if (gpio < 0 || gpio > 47)
        {
            issues << QString("Invalid GPIO: '%1' requests GPIO%2; RP2350 GPIO must be 0-47.").arg(owner).arg(gpio);
            return;
        }
        if (gpioOwners.contains(gpio))
            conflict(QString("GPIO%1").arg(gpio), gpioOwners.value(gpio), owner,
                     "A GPIO remains exclusive even when functions run on different cores.");
        else
            gpioOwners.insert(gpio, owner);
    };
    auto claimSharedConfiguration = [&](const QString& resource, const QString& owner, const QString& signature)
    {
        if (sharedConfigurations.contains(resource) && sharedConfigurations.value(resource).second != signature)
            conflict(resource, sharedConfigurations.value(resource).first, owner,
                     "Functions may share this instance only when baud/format settings are identical.");
        else if (!sharedConfigurations.contains(resource))
            sharedConfigurations.insert(resource, {owner, signature});
    };

    const auto rp = state.selections.value("rp2350a");
    if (rp.settings.value("stdio_uart", "false") == "true")
        claimSharedConfiguration("UART0 configuration", "RP2350A UART stdio",
                                 rp.settings.value("uart_baud", "115200") + "/8/1");

    for (auto it = state.selections.cbegin(); it != state.selections.cend(); ++it)
    {
        const auto& sel = it.value();
        if (sel.functionId == "disabled" || sel.settings.value("enabled", "true") != "true")
            continue;
        const QString label = sel.displayName.isEmpty() ? sel.functionId : sel.displayName;
        const bool systemFunction = sel.functionId == "rp2350a.configure" ||
                                    sel.functionId == "debug_probe.cmsis_dap" ||
                                    sel.functionId == "bootsel.use_button" || sel.functionId == "onboard_led.output" ||
                                    sel.functionId == "wireless.cyw43";
        if (!systemFunction)
        {
            if (sel.functionId.startsWith("pio"))
            {
                const QString mode = sel.settings.value("operation_mode", "Single Pin");
                const int count =
                    std::clamp(sel.settings.value("pin_count", mode == "Single Pin" ? "1" : "8").toInt(), 1, 32);
                const int base = std::clamp(sel.settings.value("base_pin", QString::number(sel.gpio)).toInt(), 0, 47);
                if (base + count > 48)
                    issues << QString("Invalid PIO pin range: '%1' requests GPIO%2-GPIO%3.")
                                  .arg(label)
                                  .arg(base)
                                  .arg(base + count - 1);
                else
                    for (int gpio = base; gpio < base + count; ++gpio)
                        claimGpio(gpio, label);
            }
            else
                claimGpio(sel.gpio, label);
        }

        QRegularExpressionMatch match = QRegularExpression("^pwm([0-9]+)([ab])$").match(sel.functionId);
        if (match.hasMatch() || sel.functionId == "robo.buzzer")
        {
            const int slice = match.hasMatch() ? match.captured(1).toInt() : (sel.gpio / 2) % 12;
            const QString channel = match.hasMatch() ? match.captured(2).toUpper() : ((sel.gpio & 1) ? "B" : "A");
            claimExclusive(QString("PWM slice %1 channel %2").arg(slice).arg(channel), label,
                           "A PWM channel has one output owner.");
            const QString signature = sel.functionId == "robo.buzzer"
                                          ? QString("%1/999/false").arg(sel.settings.value("frequency_hz", "1000"))
                                          : sel.settings.value("frequency_hz", "25000") + "/" +
                                                sel.settings.value("wrap", "1000") + "/" +
                                                sel.settings.value("phase_correct", "false");
            claimSharedConfiguration(QString("PWM slice %1 timing").arg(slice), label, signature);
        }

        match = QRegularExpression("^(uart[01])_(tx|rx|cts|rts)(?:_aux)?$").match(sel.functionId);
        if (match.hasMatch())
        {
            const QString bus = match.captured(1).toUpper(), role = match.captured(2).toUpper();
            claimExclusive(bus + " " + role + " signal", label,
                           "Each serial signal role may be mapped to only one GPIO.");
            claimSharedConfiguration(bus + " configuration", label,
                                     sel.settings.value("baud", "115200") + "/" + sel.settings.value("data_bits", "8") +
                                         "/" + sel.settings.value("stop_bits", "1"));
        }
        match = QRegularExpression("^(spi[01])_(rx|tx|sck|csn)$").match(sel.functionId);
        if (match.hasMatch())
        {
            const QString bus = match.captured(1).toUpper(), role = match.captured(2).toUpper();
            claimExclusive(bus + " " + role + " signal", label, "Each SPI signal role may be mapped to only one GPIO.");
            claimSharedConfiguration(bus + " configuration", label,
                                     sel.settings.value("baud", "1000000") + "/" +
                                         sel.settings.value("data_bits", "8"));
        }
        match = QRegularExpression("^(i2c[01])_(sda|scl)$").match(sel.functionId);
        if (match.hasMatch())
        {
            const QString bus = match.captured(1).toUpper(), role = match.captured(2).toUpper();
            claimExclusive(bus + " " + role + " signal", label, "Each I2C signal role may be mapped to only one GPIO.");
            claimSharedConfiguration(bus + " configuration", label,
                                     sel.settings.value("baud", "100000") + "/" +
                                         sel.settings.value("pull_up", "true"));
        }
        if (QRegularExpression("^adc[0-3]$").match(sel.functionId).hasMatch())
        {
            claimExclusive("ADC active channel selector", label,
                           "The current generated ADC template selects one active channel; add a scanner before "
                           "selecting multiple channels.");
            claimExclusive(sel.functionId.toUpper() + " channel", label, "An ADC channel has one generated owner.");
        }

        if (sel.settings.value("enabled", "true") != "true")
            continue;
        if (sel.functionId.startsWith("pio") || sel.functionId == "robo.neopixel")
        {
            const QString block =
                sel.functionId == "robo.neopixel" ? sel.settings.value("pio_block", "pio0") : sel.functionId;
            QString sm = sel.settings.value("pio_state_machine", sel.settings.value("state_machine", "0"));
            if (sm == "Auto")
                sm = "0";
            claimExclusive(block.toUpper() + " state machine " + sm, label,
                           "A PIO state machine cannot be shared across functions or cores.");
            const int words = sel.functionId == "robo.neopixel"
                                  ? 4
                                  : std::clamp(sel.settings.value("instruction_words", "8").toInt(), 1, 32);
            pioInstructionWords[block] += words;
            if (pioInstructionWords.value(block) > 32)
                issues << QString("PIO instruction-memory conflict: '%1' makes %2 require %3 words; each PIO block has "
                                  "32 words.")
                              .arg(label, block.toUpper())
                              .arg(pioInstructionWords.value(block));
        }
    }

    if (report)
        *report = issues.isEmpty() ? QStringLiteral("Validation PASS") : issues.join("\n");
    return issues.isEmpty();
}
QString ProjectGenerator::generateMain(const ApplicationState& state)
{
    /**Generates conservative Pico SDK C/C++ setup code for selected functions.*/
    QString s;
    QTextStream o(&s);
    const bool cpp = state.language != "C";
    const auto rp2350 = state.selections.value("rp2350a");
    const auto rpSetting = [&](const QString& key, const QString& fallback)
    { return rp2350.settings.value(key, fallback); };
    const bool rpEnabled = rp2350.functionId == "rp2350a.configure" && rpSetting("enabled", "true") == "true";
    const bool core1Enabled = rpEnabled && rpSetting("core1_enabled", "false") == "true";
    bool core1RuntimeNeeded = false;
    for (const auto& sel : state.selections)
        if (sel.settings.value("enabled", "true") == "true" &&
            FunctionExecutionModel::supportsCoreSelection(sel.functionId) &&
            FunctionExecutionModel::effectiveCore(sel, core1Enabled) == "Core 1")
            core1RuntimeNeeded = true;
    o << (cpp ? "// main.cpp\n" : "// main.c\n");
    o << "// Generated by Pico Visual Designer.\n"
      << "// This file is intentionally kept simple and heavily commented so it is easy to learn from.\n"
      << "// Hardware settings are generated from the selections made in the designer.\n\n";
    o << "// Pico SDK and hardware driver headers.\n"
      << "#include \"pico/stdlib.h\"\n"
      << "#include \"pico/cyw43_arch.h\"\n"
      << "#include \"hardware/gpio.h\"\n"
      << "#include \"hardware/pwm.h\"\n"
      << "#include \"hardware/uart.h\"\n"
      << "#include \"hardware/spi.h\"\n"
      << "#include \"hardware/i2c.h\"\n"
      << "#include \"hardware/adc.h\"\n"
      << "#include \"hardware/pio.h\"\n"
      << "#include \"hardware/clocks.h\"\n"
      << "#include \"hardware/vreg.h\"\n"
      << "#include \"hardware/watchdog.h\"\n";
    if (core1RuntimeNeeded)
        o << "#include \"pico/multicore.h\"\n";
    o << "\n";
    const bool boardSanitationNeeded = BoardStartupSanitation::needsRoboPicoNeoPixelClear(state);
    for (const auto& sel : state.selections)
        if ((sel.functionId == "robo.neopixel" &&
             (sel.settings.value("enabled", "true") == "true" || boardSanitationNeeded)) ||
            (sel.settings.value("enabled", "true") == "true" && sel.functionId.startsWith("pio")))
            o << "#include \""
              << (sel.functionId == "robo.neopixel" ? QStringLiteral("robo_neopixel")
                                                    : sel.settings.value("pio_program", "Mogge"))
              << ".pio.h\"\n";
    if (!state.selections.isEmpty())
        o << "// GPIO numbers selected in the visual designer.\n";
    for (const auto& sel : state.selections)
    {
        if (sel.gpio < 0 || sel.functionId == "disabled")
            continue;
        o << "#define PIN_" << sel.physicalPin << "_GPIO " << sel.gpio << " // " << sel.displayName << "\n";
    }
    bool core1Cyw43Runtime = false;
    for (const auto& sel : state.selections)
        if (rpEnabled && core1Enabled && sel.functionId == "onboard_led.output" &&
            sel.settings.value("enabled", "true") == "true" &&
            FunctionExecutionModel::effectiveCore(sel, true) == "Core 1")
            core1Cyw43Runtime = true;
    int core1HandlerCount = 0;
    if (core1RuntimeNeeded)
    {
        o << "// Core 1 runtime handlers are initialized by Core 0 and invoked by one shared dispatcher.\n"
          << "using pvd_runtime_handler_t = void (*)(void);\n";
    }
    for (const auto& sel : state.selections)
    {
        if (!rpEnabled || !core1Enabled || sel.settings.value("enabled", "true") != "true" ||
            !FunctionExecutionModel::supportsCoreSelection(sel.functionId) ||
            FunctionExecutionModel::effectiveCore(sel, true) != "Core 1")
            continue;
        const int handlerIndex = core1HandlerCount++;
        o << "static void pvd_runtime_handler_" << handlerIndex << "(void)\n{\n";
        emitRuntimeBody(o, sel, true);
        o << "}\n\n";
    }
    if (core1RuntimeNeeded)
    {
        o << "static void rp2350_core1_entry(void)\n{\n"
          << "    // All handlers execute cooperatively; no handler owns Core 1 indefinitely.\n";
        if (core1HandlerCount > 0)
        {
            o << "    static const pvd_runtime_handler_t handlers[] = {\n";
            for (int i = 0; i < core1HandlerCount; ++i)
                o << "        pvd_runtime_handler_" << i << (i + 1 < core1HandlerCount ? "," : "") << "\n";
            o << "    };\n"
              << "    constexpr uint handler_count = sizeof(handlers) / sizeof(handlers[0]);\n"
              << "    while (true) {\n"
              << "        for (uint i = 0; i < handler_count; ++i) handlers[i]();\n"
              << "        tight_loop_contents();\n"
              << "    }\n";
        }
        else
        {
            o << "    while (true) tight_loop_contents();\n";
        }
        o << "}\n\n";
    }
    int core0HandlerCount = 0;
    for (const auto& sel : state.selections)
    {
        if (sel.settings.value("enabled", "true") != "true")
            continue;
        if (!FunctionExecutionModel::supportsCoreSelection(sel.functionId) ||
            FunctionExecutionModel::effectiveCore(sel, rpEnabled && core1Enabled) != "Core 0")
            continue;

        const int handlerIndex = core0HandlerCount++;
        o << "// Core 0 runtime handler generated from the function's Settings.\n"
          << "static void pvd_core0_runtime_handler_" << handlerIndex << "(void)\n{\n";
        emitRuntimeBody(o, sel, false);
        o << "}\n\n";
    }
    o << "\n// Program entry point. The Pico starts executing here after reset.\n"
      << "int main(void)\n{\n"
      << "    // Initialize the Pico SDK and all selected hardware.\n";
    if (rpEnabled)
    {
        const auto voltage = rpSetting("vreg_voltage", "1.10").replace('.', '_');
        const int clock = std::clamp(rpSetting("system_clock_khz", "150000").toInt(), 0, 300000);
        o << "    // RP2350A system configuration from Project > Settings.\n"
          << "    // Voltage: " << rpSetting("vreg_voltage", "1.10") << " V; system clock: " << clock << " kHz.\n";
        if (voltage == "0_85" || voltage == "0_90" || voltage == "0_95" || voltage == "1_00" || voltage == "1_05" ||
            voltage == "1_10" || voltage == "1_15" || voltage == "1_20" || voltage == "1_25" || voltage == "1_30")
            o << "    vreg_set_voltage(VREG_VOLTAGE_" << voltage << "); // Set the core voltage.\n";
        if (clock > 0)
            o << "    set_sys_clock_khz(" << clock << ", true); // Set the CPU clock.\n";
        if (rpSetting("watchdog_enabled", "false") == "true")
            o << "    watchdog_enable(" << std::clamp(rpSetting("watchdog_timeout_ms", "2000").toInt(), 1, 3600000)
              << ", true); // Restart if the program stops responding.\n";
        const int delay = std::clamp(rpSetting("startup_delay_ms", "0").toInt(), 0, 60000);
        if (delay > 0)
            o << "    sleep_ms(" << delay << "); // Wait before continuing startup.\n";
    }
    o << "    // Start the standard input/output drivers (USB serial and UART).\n"
      << "    stdio_init_all();\n";
    if (rpEnabled && rpSetting("stdio_uart", "false") == "true")
        o << "    uart_init(uart0, " << std::clamp(rpSetting("uart_baud", "115200").toInt(), 1200, 4000000) << ");\n";
    bool cyw43Needed = false;
    for (const auto& sel : state.selections)
        if ((sel.functionId == "onboard_led.output" || sel.functionId == "wireless.cyw43") &&
            sel.settings.value("enabled", "true") == "true")
            cyw43Needed = true;
    if (cyw43Needed)
        o << "\n    // Initialize the CYW43 wireless chip used by the Pico 2 W onboard LED/Wi-Fi.\n"
          << "    // This must run on Core 0 before any CYW43 LED or Wi-Fi call.\n"
          << "    int cyw43_status = cyw43_arch_init();\n"
          << "    if (cyw43_status != 0) {\n"
          << "        // Initialization failed, so stop here instead of using an uninitialized driver.\n"
          << "        while (true) {\n"
          << "            tight_loop_contents();\n"
          << "        }\n"
          << "    }\n";
    if (boardSanitationNeeded)
        o << "\n" << BoardStartupSanitation::generateRoboPicoNeoPixelClear(state);
    auto setting = [&](const FunctionSelection& sel, const QString& key, const QString& fallback)
    { return sel.settings.value(key, fallback); };
    for (const auto& sel : state.selections)
    {
        o << "\n    // ------------------------------------------------------------\n"
          << "    // Configure " << sel.displayName << " (" << sel.functionName << ").\n"
          << "    // ------------------------------------------------------------\n";
        if (sel.functionId == "robo.neopixel" && setting(sel, "enabled", "true") == "true")
        {
            const int count = std::clamp(setting(sel, "pixel_count", "2").toInt(), 1, 256);
            const int r = std::clamp(setting(sel, "red", "0").toInt(), 0, 255);
            const int g = std::clamp(setting(sel, "green", "32").toInt(), 0, 255);
            const int b = std::clamp(setting(sel, "blue", "0").toInt(), 0, 255);
            const int r2 = std::clamp(setting(sel, "red2", "32").toInt(), 0, 255);
            const int g2 = std::clamp(setting(sel, "green2", "0").toInt(), 0, 255);
            const int b2 = std::clamp(setting(sel, "blue2", "0").toInt(), 0, 255);
            const int brightness = std::clamp(setting(sel, "brightness", "64").toInt(), 0, 255);
            o << "    // NeoPixel data is sent by the PIO program generated in the PIO section.\n"
              << "    // GPIO: " << sel.gpio << "; pixels: " << count << "; brightness: " << brightness << " / 255.\n"
              << "    // First color after brightness scaling: R=" << (r * brightness / 255)
              << ", G=" << (g * brightness / 255) << ", B=" << (b * brightness / 255) << ".\n"
              << "    PIO neopixel_pio = pio0;\n"
              << "    uint neopixel_offset = pio_add_program(neopixel_pio, &robo_neopixel_program);\n"
              << "    pio_sm_config neopixel_cfg = robo_neopixel_program_get_default_config(neopixel_offset);\n"
              << "    sm_config_set_sideset_pins(&neopixel_cfg, " << sel.gpio << ");\n"
              << "    sm_config_set_out_shift(&neopixel_cfg, false, true, 24);\n"
              << "    sm_config_set_fifo_join(&neopixel_cfg, PIO_FIFO_JOIN_TX);\n"
              << "    sm_config_set_clkdiv(&neopixel_cfg, 15.625f);\n"
              << "    pio_gpio_init(neopixel_pio, " << sel.gpio << ");\n"
              << "    pio_sm_set_consecutive_pindirs(neopixel_pio, 0, " << sel.gpio << ", 1, true);\n"
              << "    pio_sm_init(neopixel_pio, 0, neopixel_offset, &neopixel_cfg);\n"
              << "    pio_sm_set_enabled(neopixel_pio, 0, true);\n"
              << "    // Send the first pixel color. The PIO program converts this into the NeoPixel signal.\n"
              << "    pio_sm_put_blocking(neopixel_pio, 0, ((uint32_t)" << (g * brightness / 255)
              << " << 24) | ((uint32_t)" << (r * brightness / 255) << " << 16) | ((uint32_t)" << (b * brightness / 255)
              << " << 8));\n";
            if (count > 1)
                o << "    // Send the second pixel color.\n"
                  << "    pio_sm_put_blocking(neopixel_pio, 0, ((uint32_t)" << (g2 * brightness / 255)
                  << " << 24) | ((uint32_t)" << (r2 * brightness / 255) << " << 16) | ((uint32_t)"
                  << (b2 * brightness / 255) << " << 8));\n";
            if (count > 2)
                o << "    // Remaining pixels are turned off.\n"
                  << "    for (uint pixel = 2; pixel < " << count << "; ++pixel) {\n"
                  << "        pio_sm_put_blocking(neopixel_pio, 0, 0);\n"
                  << "    }\n";
            o << "    // NeoPixels need a short reset/latch pulse after the data.\n" << "    sleep_us(80);\n";
        }
        else if (sel.functionId == "robo.buzzer" && setting(sel, "enabled", "true") == "true")
        {
            const int frequency = std::clamp(setting(sel, "frequency_hz", "1000").toInt(), 20, 20000);
            const int duration = std::clamp(setting(sel, "duration_ms", "250").toInt(), 0, 60000);
            o << "    // Generate a tone using PWM. Frequency: " << frequency << " Hz; duration: " << duration
              << " ms.\n"
              << "    // Select PWM mode instead of ordinary digital GPIO mode.\n"
              << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_PWM);\n"
              << "    // Find the PWM slice that controls this GPIO.\n"
              << "    uint buzzer_slice_" << sel.gpio << " = pwm_gpio_to_slice_num(" << sel.gpio << ");\n"
              << "    // Start with the SDK's default PWM configuration.\n"
              << "    pwm_config buzzer_cfg_" << sel.gpio << " = pwm_get_default_config();\n"
              << "    // Calculate timing from the actual configured system clock.\n"
              << "    pwm_config_set_clkdiv(&buzzer_cfg_" << sel.gpio << ", (float)clock_get_hz(clk_sys) / ("
              << frequency << ".0f * 1000.0f));\n"
              << "    // The counter counts from 0 to 999, giving 1000 timing steps.\n"
              << "    pwm_config_set_wrap(&buzzer_cfg_" << sel.gpio << ", 999);\n"
              << "    // Apply the configuration and start the PWM slice.\n"
              << "    pwm_init(buzzer_slice_" << sel.gpio << ", &buzzer_cfg_" << sel.gpio << ", true);\n"
              << "    // A level of 500 out of 1000 gives a 50% duty cycle.\n"
              << "    pwm_set_gpio_level(" << sel.gpio << ", 500);\n";
            if (duration > 0)
                o << "    // Keep the tone active for the selected duration.\n"
                  << "    sleep_ms(" << duration << ");\n"
                  << "    // A level of 0 turns the buzzer off.\n"
                  << "    pwm_set_gpio_level(" << sel.gpio << ", 0);\n";
        }
    }
    for (const auto& sel : state.selections)
    {
        if (sel.functionId == "disabled" || setting(sel, "enabled", "true") != "true")
            continue;
        if (sel.functionId == "rp2350a.configure")
        {
        }
        else if (sel.functionId == "debug_probe.cmsis_dap")
        {
        }
        else if (sel.functionId == "robo.neopixel" || sel.functionId == "robo.buzzer")
        {
        }
        else if (sel.functionId == "bootsel.use_button")
        {
            o << "    // BOOTSEL is handled by the RP2350 ROM; its selected mode is configured in CMake.\n";
        }
        else if (sel.functionId == "onboard_led.output")
        {
            o << "    // Core 0 initializes CYW43 to a safe Off state before either runtime starts.\n"
              << "    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);\n";
        }
        else if (sel.functionId == "wireless.cyw43")
        {
            const auto mode = setting(sel, "wifi_mode", "Station");
            const auto ssid = setting(sel, "ssid", "").replace("\"", "\\\"");
            const auto password = setting(sel, "password", "").replace("\"", "\\\"");
            o << "    // CYW43 was initialized above on Core 0. Configure the selected Wi-Fi mode here.\n";
            if (mode == "Access Point")
                o << "    cyw43_arch_enable_ap_mode(\"" << ssid << "\", \"" << password
                  << "\", CYW43_AUTH_WPA2_AES_PSK);\n";
            else
            {
                o << "    cyw43_arch_enable_sta_mode();\n";
                o << "    cyw43_arch_wifi_connect_timeout_ms(\"" << ssid << "\", \"" << password
                  << "\", CYW43_AUTH_WPA2_AES_PSK, 30000);\n";
            }
        }
        else if (sel.functionId == "gpio.input")
        {
            const auto pull = setting(sel, "pull", "None");
            o << "    // Configure the pin as an input.\n"
              << "    gpio_init(" << sel.gpio << ");\n"
              << "    gpio_set_dir(" << sel.gpio << ", GPIO_IN);\n"
              << "    gpio_set_pulls(" << sel.gpio << ", " << (pull == "Pull-up" ? "true" : "false") << ", "
              << (pull == "Pull-down" ? "true" : "false") << ");\n";
        }
        else if (sel.functionId == "gpio.output")
        {
            const auto level = setting(sel, "initial_state", "Low");
            o << "    // Configure the pin as an output and set its initial level.\n"
              << "    gpio_init(" << sel.gpio << ");\n"
              << "    gpio_set_dir(" << sel.gpio << ", GPIO_OUT);\n"
              << "    gpio_put(" << sel.gpio << ", " << (level == "High" ? "true" : "false") << ");\n"
              << "    gpio_set_drive_strength(" << sel.gpio << ", GPIO_DRIVE_STRENGTH_"
              << setting(sel, "drive_strength", "4") << "MA);\n"
              << "    gpio_set_slew_rate(" << sel.gpio << ", GPIO_SLEW_RATE_"
              << setting(sel, "slew_rate", "Fast").toUpper() << ");\n";
        }
        else if (sel.functionId == "sio")
        {
            const auto direction = setting(sel, "direction", "Input");
            const auto pull = setting(sel, "pull", "None");
            o << "    // Configure a general-purpose digital input or output.\n"
              << "    gpio_init(" << sel.gpio << ");\n"
              << "    gpio_set_dir(" << sel.gpio << ", " << (direction == "Output" ? "GPIO_OUT" : "GPIO_IN") << ");\n"
              << "    gpio_set_pulls(" << sel.gpio << ", " << (pull == "Pull-up" ? "true" : "false") << ", "
              << (pull == "Pull-down" ? "true" : "false") << ");\n";
        }
        else if (sel.functionId.startsWith("pwm"))
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_PWM);\n";
        else if (sel.functionId.startsWith("adc"))
        {
            const int rate = std::max(1, setting(sel, "sample_frequency", "1000").toInt());
            o << "    adc_init(); adc_gpio_init(" << sel.gpio << "); adc_select_input(" << std::max(0, sel.gpio - 26)
              << ");\n";
            o << "    adc_set_clkdiv(48000000.0f / " << rate << ".0f);\n";
            if (setting(sel, "input_mode", "Single") == "Continuous")
                o << "    adc_fifo_setup(true, true, 1, false, false); adc_run(true);\n";
        }
        else if (sel.functionId.startsWith("spi"))
        {
            const auto bus = sel.functionId.startsWith("spi0") ? "spi0" : "spi1";
            const int baud = std::max(1, setting(sel, "baud", "1000000").toInt());
            const int bits = std::clamp(setting(sel, "data_bits", "8").toInt(), 8, 16);
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_SPI);\n";
            o << "    spi_init(" << bus << ", " << baud << ");\n";
            o << "    spi_set_format(" << bus << ", " << bits << ", SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);\n";
        }
        else if (sel.functionId.startsWith("uart"))
        {
            const auto bus = sel.functionId.startsWith("uart0") ? "uart0" : "uart1";
            const int baud = std::max(1, setting(sel, "baud", "115200").toInt());
            const int bits = std::clamp(setting(sel, "data_bits", "8").toInt(), 5, 8);
            const int stops = std::clamp(setting(sel, "stop_bits", "1").toInt(), 1, 2);
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_UART);\n";
            o << "    uart_init(" << bus << ", " << baud << ");\n";
            o << "    uart_set_format(" << bus << ", " << bits << ", " << stops << ", UART_PARITY_NONE);\n";
        }
        else if (sel.functionId.startsWith("i2c"))
        {
            const auto bus = sel.functionId.startsWith("i2c0") ? "i2c0" : "i2c1";
            const int baud = std::max(1, setting(sel, "baud", "100000").toInt());
            const auto pull = setting(sel, "pull_up", "true") == "true";
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_I2C);\n";
            o << "    i2c_init(" << bus << ", " << baud << ");\n";
            o << "    gpio_set_pulls(" << sel.gpio << ", " << (pull ? "true" : "false") << ", "
              << (pull ? "true" : "false") << ");\n";
        }
        else if (sel.functionId.startsWith("pio"))
        {
            const auto pio = sel.functionId == "pio0" ? "pio0" : (sel.functionId == "pio1" ? "pio1" : "pio2");
            const auto fn = sel.settings.value("pio_program", "Mogge");
            const auto sm = setting(sel, "state_machine", "Auto");
            const auto mode = setting(sel, "operation_mode", "Single Pin");
            const int machine = sm == "Auto" ? 0 : sm.toInt();
            const int count = std::clamp(setting(sel, "pin_count", mode == "Single Pin" ? "1" : "8").toInt(), 1, 32);
            const int base = std::clamp(setting(sel, "base_pin", QString::number(sel.gpio)).toInt(), 0, 47);
            const int threshold = std::clamp(setting(sel, "shift_threshold", "32").toInt(), 1, 32);
            const auto shift = setting(sel, "shift_direction", "Right") == "Right";
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_" << sel.functionId.toUpper() << ");\n";
            o << "    uint pio_offset_" << sel.gpio << " = pio_add_program(" << pio << ", &" << fn << "_program);\n";
            o << "    for (uint pin = " << base << "; pin < " << base << " + " << count << "; ++pin) pio_gpio_init("
              << pio << ", pin);\n";
            o << "    pio_sm_config pio_cfg_" << sel.gpio << " = " << fn << "_program_get_default_config(pio_offset_"
              << sel.gpio << ");\n";
            if (mode == "Multi-Pin Input")
                o << "    sm_config_set_in_pins(&pio_cfg_" << sel.gpio << ", " << base << ");\n";
            else if (mode == "Multi-Pin Output")
                o << "    sm_config_set_out_pins(&pio_cfg_" << sel.gpio << ", " << base << ", " << count << ");\n";
            else
            {
                o << "    sm_config_set_in_pins(&pio_cfg_" << sel.gpio << ", " << base << ");\n";
                o << "    sm_config_set_out_pins(&pio_cfg_" << sel.gpio << ", " << base << ", " << count << ");\n";
            }
            o << "    sm_config_set_in_shift(&pio_cfg_" << sel.gpio << ", " << (shift ? "true" : "false") << ", "
              << (setting(sel, "autopush", "false") == "true" ? "true" : "false") << ", " << threshold << ");\n";
            o << "    sm_config_set_out_shift(&pio_cfg_" << sel.gpio << ", " << (shift ? "true" : "false") << ", "
              << (setting(sel, "autopull", "false") == "true" ? "true" : "false") << ", " << threshold << ");\n";
            o << "    sm_config_set_clkdiv(&pio_cfg_" << sel.gpio << ", " << setting(sel, "clock_divider", "1.0")
              << ");\n";
            const auto fifo = setting(sel, "fifo_join", "None");
            if (fifo == "RX")
                o << "    sm_config_set_fifo_join(&pio_cfg_" << sel.gpio << ", PIO_FIFO_JOIN_RX);\n";
            else if (fifo == "TX")
                o << "    sm_config_set_fifo_join(&pio_cfg_" << sel.gpio << ", PIO_FIFO_JOIN_TX);\n";
            o << "    pio_sm_init(" << pio << ", " << machine << ", pio_offset_" << sel.gpio << ", &pio_cfg_"
              << sel.gpio << ");\n";
            o << "    pio_sm_set_enabled(" << pio << ", " << machine << ", true);\n";
        }
        else if (sel.functionId == "clock_gpin0" || sel.functionId == "clock_gpin1" ||
                 sel.functionId.startsWith("clock_gpout"))
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_GPCK);\n";
        else if (sel.functionId == "hstx")
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_HSTX);\n";
        else if (sel.functionId == "qmi_cs1n")
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_XIP_CS1);\n";
        else if (sel.functionId.startsWith("trace"))
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_CORESIGHT_TRACE);\n";
        else if (sel.functionId.startsWith("usb_"))
            o << "    gpio_set_function(" << sel.gpio << ", GPIO_FUNC_USB);\n";
        else if (sel.gpio >= 0)
            o << "    // " << sel.functionName << " uses RP2350 alternate-function mapping on GPIO" << sel.gpio
              << ".\n";
    }
    for (const auto& sel : state.selections)
    {
        if (sel.functionId.startsWith("pwm") && setting(sel, "enabled", "true") == "true")
        {
            o << "    uint slice_" << sel.gpio << " = pwm_gpio_to_slice_num(" << sel.gpio << ");\n";
            o << "    uint chan_" << sel.gpio << " = pwm_gpio_to_channel(" << sel.gpio << ");\n";
            const int frequency = std::max(1, setting(sel, "frequency_hz", "25000").toInt());
            const int duty = std::clamp(setting(sel, "duty_percent", "50").toInt(), 0, 100);
            const int wrap = std::clamp(setting(sel, "wrap", "1000").toInt(), 1, 65535);
            const int counter = std::clamp(setting(sel, "counter_start", "0").toInt(), 0, wrap);
            const bool phaseCorrect = setting(sel, "phase_correct", "false") == "true";
            const bool invertA = setting(sel, "invert_a", "false") == "true";
            const bool invertB = setting(sel, "invert_b", "false") == "true";
            QString divider = setting(sel, "clock_divider", "1.0").trimmed();
            if (!QRegularExpression("^[0-9]+(\\.[0-9]+)?$").match(divider).hasMatch())
                divider = "1.0";
            const bool automatic = setting(sel, "timing_mode", "Automatic") != "Manual";
            const QString dividerMode = setting(sel, "divider_mode", "Free-running");
            o << "    pwm_config cfg_" << sel.gpio << " = pwm_get_default_config();\n";
            if (automatic)
                o << "    pwm_config_set_clkdiv(&cfg_" << sel.gpio << ", (float)clock_get_hz(clk_sys) / (" << frequency
                  << ".0f * " << (wrap + 1) << ".0f * " << (phaseCorrect ? 2 : 1) << ".0f));\n";
            else
                o << "    pwm_config_set_clkdiv(&cfg_" << sel.gpio << ", " << divider << "f);\n";
            if (dividerMode == "B rises")
                o << "    pwm_config_set_clkdiv_mode(&cfg_" << sel.gpio << ", PWM_DIV_B_RISING);\n";
            else if (dividerMode == "B high")
                o << "    pwm_config_set_clkdiv_mode(&cfg_" << sel.gpio << ", PWM_DIV_B_HIGH);\n";
            o << "    pwm_config_set_phase_correct(&cfg_" << sel.gpio << ", " << (phaseCorrect ? "true" : "false")
              << ");\n";
            o << "    pwm_config_set_output_polarity(&cfg_" << sel.gpio << ", " << (invertA ? "true" : "false") << ", "
              << (invertB ? "true" : "false") << ");\n";
            o << "    pwm_config_set_wrap(&cfg_" << sel.gpio << ", " << wrap << ");\n";
            o << "    pwm_init(slice_" << sel.gpio << ", &cfg_" << sel.gpio << ", true);\n";
            o << "    pwm_set_counter(slice_" << sel.gpio << ", " << counter << ");\n";
            o << "    pwm_set_chan_level(slice_" << sel.gpio << ", chan_" << sel.gpio << ", (uint16_t)(((uint32_t)"
              << wrap << " * " << duty << ") / 100u));\n";
            o << "    pwm_set_enabled(slice_" << sel.gpio << ", "
              << (setting(sel, "enabled", "true") == "true" ? "true" : "false") << ");\n";
        }
    }
    if (core1RuntimeNeeded)
        o << "\n    // All Core 0 hardware and dependencies are ready before Core 1 starts.\n"
          << "    multicore_launch_core1(rp2350_core1_entry); // Start the Core 1 dispatcher.\n";
    o << "\n    // Main loop: keep the program running on the Pico.\n"
      << "    // Runtime functions selected in Settings are called cooperatively here.\n"
      << "    while (true) {\n";
    for (int i = 0; i < core0HandlerCount; ++i)
        o << "        pvd_core0_runtime_handler_" << i << "();\n";
    if (core1Cyw43Runtime)
        o << "        // Core 0 consumes Core 1 LED commands and accesses CYW43 hardware.\n"
          << "        if (multicore_fifo_rvalid()) {\n"
          << "            const uint32_t led_command = multicore_fifo_pop_blocking();\n"
          << "            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_command != 0);\n"
          << "        }\n";
    if (rpEnabled && rpSetting("watchdog_enabled", "false") == "true")
        o << "        watchdog_update();\n";
    o << "        tight_loop_contents();\n    }\n}\n";
    return s;
}
QString ProjectGenerator::generateCMake(const ApplicationState& state, const QStringList& pioFiles)
{
    /**Generates Pico SDK CMake integration for the selected language and PIO programs.*/
    QString s;
    QTextStream o(&s);
    const QString ext = state.language == "C" ? "c" : "cpp";
    QString target = state.projectName;
    target.replace(QRegularExpression("[^A-Za-z0-9_]"), "_");
    if (target.isEmpty())
        target = "PICO2W";
    const auto rp2350 = state.selections.value("rp2350a");
    const bool core1Enabled = rp2350.functionId == "rp2350a.configure" &&
                              rp2350.settings.value("enabled", "true") == "true" &&
                              rp2350.settings.value("core1_enabled", "false") == "true";
    bool core1RuntimeNeeded = false;
    for (const auto& sel : state.selections)
        if (sel.settings.value("enabled", "true") == "true" &&
            FunctionExecutionModel::supportsCoreSelection(sel.functionId) &&
            FunctionExecutionModel::effectiveCore(sel, core1Enabled) == "Core 1")
            core1RuntimeNeeded = true;
    const bool usb = rp2350.settings.value("stdio_usb", "true") == "true";
    const bool uart = rp2350.settings.value("stdio_uart", "false") == "true";
    const int uartBaud = std::clamp(rp2350.settings.value("uart_baud", "115200").toInt(), 1200, 4000000);
    o << "# CMakeLists.txt\ncmake_minimum_required(VERSION 3.13)\ninclude(pico_sdk_import.cmake)\nproject(" << target
      << " LANGUAGES C CXX ASM)\nset(CMAKE_C_STANDARD 11)\nset(CMAKE_CXX_STANDARD 17)\npico_sdk_init()\nadd_executable("
      << target << " main." << ext << ")\ntarget_link_libraries(" << target << " pico_stdlib";
    if (core1RuntimeNeeded)
        o << " pico_multicore";
    o << " hardware_gpio hardware_pwm hardware_uart hardware_spi hardware_i2c hardware_adc hardware_pio "
         "pico_cyw43_arch_lwip_threadsafe_background pico_lwip_nosys)\n";
    for (const auto& sel : state.selections)
        if (sel.functionId == "bootsel.use_button" && sel.settings.value("mode", "Press") != "Press")
        {
            const int timeout = std::clamp(sel.settings.value("debounce_ms", "50").toInt(), 1, 10000);
            o << "target_link_libraries(" << target << " pico_bootsel_via_double_reset)\ntarget_compile_definitions("
              << target << " PRIVATE PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS=" << timeout << ")\n";
        }
    o << "target_include_directories(" << target << " PRIVATE ${CMAKE_CURRENT_LIST_DIR})\n";
    for (const auto& p : pioFiles)
        o << "pico_generate_pio_header(" << target << " ${CMAKE_CURRENT_LIST_DIR}/" << p << ")\n";
    o << "pico_enable_stdio_usb(" << target << " " << (usb ? 1 : 0) << ")\npico_enable_stdio_uart(" << target << " "
      << (uart ? 1 : 0) << ")\n";
    if (uart)
        o << "target_compile_definitions(" << target << " PRIVATE PVD_UART_BAUD=" << uartBaud << ")\n";
    o << "pico_add_extra_outputs(" << target << ")\n";
    return s;
}
} // namespace pvd
