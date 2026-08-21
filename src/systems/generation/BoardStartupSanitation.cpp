#include "systems/generation/BoardStartupSanitation.hpp"
#include "systems/components/PicoPinMap.hpp"
#include <algorithm>

namespace pvd {

namespace {
const FunctionSelection* fixedRoboPicoNeoPixel(const ApplicationState& state)
{
    const int fixedGpio = roboPicoGpio("neopixel_rgb");
    for (const auto& selection : state.selections) {
        if (selection.functionId == "robo.neopixel"
            && selection.gpio == fixedGpio
            && selection.settings.value("enabled", "true") != "true")
            return &selection;
    }
    return nullptr;
}
}

bool BoardStartupSanitation::needsRoboPicoNeoPixelClear(const ApplicationState& state)
{
    return fixedRoboPicoNeoPixel(state) != nullptr;
}

QString BoardStartupSanitation::generateRoboPicoNeoPixelClear(const ApplicationState& state)
{
    const auto* selection = fixedRoboPicoNeoPixel(state);
    if (!selection) return {};

    const int count = std::clamp(selection->settings.value("pixel_count", "2").toInt(), 1, 256);
    QString code;
    code += "    // Board startup sanitation: clear the fixed ROBO-PICO NeoPixel on GP18.\n";
    code += "    // This is not a selected NeoPixel function and is not part of any runtime dispatcher.\n";
    code += "    PIO board_sanitation_pio = pio0;\n";
    code += "    int board_sanitation_sm = pio_claim_unused_sm(board_sanitation_pio, true);\n";
    code += "    uint board_sanitation_offset = pio_add_program(board_sanitation_pio, &robo_neopixel_program);\n";
    code += "    pio_sm_config board_sanitation_cfg = robo_neopixel_program_get_default_config(board_sanitation_offset);\n";
    code += "    sm_config_set_sideset_pins(&board_sanitation_cfg, 18);\n";
    code += "    sm_config_set_out_shift(&board_sanitation_cfg, false, true, 24);\n";
    code += "    sm_config_set_fifo_join(&board_sanitation_cfg, PIO_FIFO_JOIN_TX);\n";
    code += "    sm_config_set_clkdiv(&board_sanitation_cfg, 15.625f);\n";
    code += "    pio_gpio_init(board_sanitation_pio, 18);\n";
    code += "    pio_sm_set_consecutive_pindirs(board_sanitation_pio, (uint)board_sanitation_sm, 18, 1, true);\n";
    code += "    pio_sm_init(board_sanitation_pio, (uint)board_sanitation_sm, board_sanitation_offset, &board_sanitation_cfg);\n";
    code += "    pio_sm_set_enabled(board_sanitation_pio, (uint)board_sanitation_sm, true);\n";
    code += "    for (uint pixel = 0; pixel < " + QString::number(count) + "; ++pixel) {\n";
    code += "        pio_sm_put_blocking(board_sanitation_pio, (uint)board_sanitation_sm, 0);\n";
    code += "    }\n";
    code += "    sleep_us(80);\n";
    code += "    pio_sm_set_enabled(board_sanitation_pio, (uint)board_sanitation_sm, false);\n";
    code += "    pio_remove_program_and_unclaim_sm(&robo_neopixel_program, board_sanitation_pio, (uint)board_sanitation_sm, board_sanitation_offset);\n";
    code += "    gpio_init(18);\n";
    code += "    gpio_set_dir(18, GPIO_OUT);\n";
    code += "    gpio_put(18, false);\n";
    return code;
}

}
