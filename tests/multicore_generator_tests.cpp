#include "systems/generation/ProjectGenerator.hpp"
#include "systems/generation/BoardStartupSanitation.hpp"
#include "systems/generation/FunctionExecutionModel.hpp"
#include "systems/project/ProjectStore.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

using namespace pvd;

namespace {
int failures=0;
void check(bool condition,const char* message)
{
    if(condition)return;
    std::cerr<<"FAIL: "<<message<<'\n';
    ++failures;
}
FunctionSelection selection(QString component,QString function,int gpio=-1)
{
    FunctionSelection value;
    value.componentId=component;
    value.displayName=component;
    value.functionId=function;
    value.functionName=function;
    value.gpio=gpio;
    value.physicalPin=gpio+1;
    return value;
}
ApplicationState baseState(const QString& path,bool core1)
{
    ApplicationState state;
    state.projectName="GEN_TEST";
    state.projectPath=path;
    auto rp=selection("rp2350a","rp2350a.configure");
    rp.settings={{"enabled","true"},{"core1_enabled",core1?"true":"false"},{"stdio_usb","true"}};
    state.selections.insert(rp.componentId,rp);
    return state;
}
QString read(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly|QIODevice::Text)?QString::fromUtf8(file.readAll()):QString{};
}
QString generate(ApplicationState& state)
{
    QString error;
    check(ProjectGenerator::generate(&state,{},&error),qPrintable("generation failed: "+error));
    return read(state.projectPath+"/generated/main.cpp");
}
}

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),false);
        auto gpio=selection("pin1","gpio.output",0);
        gpio.settings={{"execution_core","Core 1"},{"blink_enabled","true"}};
        state.selections.insert(gpio.componentId,gpio);
        const QString main=generate(state),cmake=read(dir.path()+"/generated/CMakeLists.txt");
        check(main.contains("pvd_core0_runtime_handler_0();"),"disabled Core 1 must force effective Core 0 runtime");
        check(!main.contains("pico/multicore.h")&&!main.contains("multicore_launch_core1"),"disabled Core 1 emitted multicore source");
        check(!cmake.contains("pico_multicore"),"disabled Core 1 linked pico_multicore");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto led=selection("onboard_led","onboard_led.output");
        led.settings={{"execution_core","Core 0"},{"blink_enabled","true"}};
        state.selections.insert(led.componentId,led);
        const QString main=generate(state);
        check(main.contains("pvd_core0_runtime_handler_0();"),"Core 0 LED handler missing");
        check(!main.contains("multicore_launch_core1"),"empty Core 1 dispatcher should not be generated");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        for(int gpioNumber=0;gpioNumber<2;++gpioNumber){
            auto gpio=selection("pin"+QString::number(gpioNumber),"gpio.output",gpioNumber);
            gpio.settings={{"execution_core","Core 1"},{"blink_enabled","true"}};
            state.selections.insert(gpio.componentId,gpio);
        }
        const QString main=generate(state),cmake=read(dir.path()+"/generated/CMakeLists.txt");
        check(main.contains("pvd_runtime_handler_0")&&main.contains("pvd_runtime_handler_1"),"multiple Core 1 handlers missing");
        check(main.contains("for (uint i = 0; i < handler_count; ++i) handlers[i]();"),"cooperative Core 1 dispatcher missing");
        check(main.indexOf("gpio_init(0)")<main.indexOf("multicore_launch_core1"),"Core 1 launched before GPIO initialization");
        check(cmake.contains("pico_multicore"),"Core 1 runtime did not link pico_multicore");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto led=selection("onboard_led","onboard_led.output");
        led.settings={{"execution_core","Core 1"},{"blink_enabled","true"}};
        state.selections.insert(led.componentId,led);
        const QString main=generate(state);
        check(main.indexOf("cyw43_arch_init()")<main.indexOf("multicore_launch_core1"),"CYW43 initialization must precede Core 1 launch");
        check(main.contains("multicore_fifo_push_blocking")&&main.contains("multicore_fifo_pop_blocking"),"Core 1 CYW43 ownership protocol missing");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto buzzer=selection("buzzer","robo.buzzer",22);
        buzzer.settings={{"execution_core","Core 1"},{"duration_ms","250"}};
        state.selections.insert(buzzer.componentId,buzzer);
        auto pixels=selection("pixels","robo.neopixel",18);
        pixels.settings={{"execution_core","Core 1"},{"pio_block","pio0"},{"pio_state_machine","0"}};
        state.selections.insert(pixels.componentId,pixels);
        const QString main=generate(state);
        check(main.contains("tone_active")&&main.contains("colors_sent"),"buzzer/NeoPixel real runtime missing");
        check(main.contains("last_tone_us")&&main.contains("2000000u"),"buzzer periodic runtime interval missing");
        check(!main.contains("handler reserved"),"placeholder Core 1 handler remains");
        check(main.indexOf("pio_sm_init")<main.indexOf("multicore_launch_core1"),"PIO initialization must precede runtime launch");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto pixels=selection("pixels","robo.neopixel",18);
        pixels.settings={{"enabled","false"},{"execution_core","Core 1"},{"pixel_count","2"}};
        state.selections.insert(pixels.componentId,pixels);
        auto gpio=selection("pin2","gpio.output",1);
        gpio.settings={{"enabled","true"},{"execution_core","Core 1"},{"blink_enabled","true"}};
        state.selections.insert(gpio.componentId,gpio);
        QString report;
        check(ProjectGenerator::validate(state,&report),"disabled fixed NeoPixel must not create a resource conflict");
        const QString main=generate(state),cmake=read(dir.path()+"/generated/CMakeLists.txt");
        check(BoardStartupSanitation::needsRoboPicoNeoPixelClear(state),"fixed ROBO-PICO NeoPixel sanitation was not selected");
        check(main.contains("pio_claim_unused_sm")&&main.contains("pio_remove_program_and_unclaim_sm"),"startup sanitation did not claim and release a temporary PIO state machine");
        check(!main.contains("colors_sent"),"disabled NeoPixel emitted a runtime handler");
        check(main.contains("for (uint pixel = 0; pixel < 2; ++pixel)"),"startup sanitation did not clear the configured pixel count");
        check(main.indexOf("pio_remove_program_and_unclaim_sm")<main.indexOf("multicore_launch_core1"),"startup sanitation was not completed before runtime startup");
        check(cmake.contains("robo_neopixel.pio"),"sanitation did not include the board-level clear program");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto first=selection("first","gpio.output",4),second=selection("second","gpio.output",4);
        state.selections.insert(first.componentId,first);state.selections.insert(second.componentId,second);
        QString report;
        check(!ProjectGenerator::validate(state,&report)&&report.contains("GPIO4")&&report.contains("first")&&report.contains("second"),"GPIO conflict report lacks owners/resource");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto first=selection("pwm-a","pwm0a",0),second=selection("buzzer","robo.buzzer",0);
        state.selections.insert(first.componentId,first);state.selections.insert(second.componentId,second);
        QString report;
        check(!ProjectGenerator::validate(state,&report)&&report.contains("PWM slice 0 channel A"),"PWM ownership collision not rejected");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto buzzer=selection("buzzer","robo.buzzer",22);
        buzzer.settings={{"execution_core","Core 1"},{"frequency_hz","2000"},{"duration_ms","200"}};
        auto pixels=selection("pixels","robo.neopixel",18);
        pixels.settings={{"execution_core","Core 1"},{"pio_block","pio0"},{"pio_state_machine","1"},{"red","255"}};
        state.selections.insert(buzzer.componentId,buzzer);
        state.selections.insert(pixels.componentId,pixels);
        const QString main=generate(state);
        check(main.contains("pvd_runtime_handler_0")&&main.contains("pvd_runtime_handler_1"),"different Core 1 function types missing from shared dispatcher");
        check(main.contains("tone_active")&&main.contains("colors_sent"),"different Core 1 function runtimes are empty");
        check(main.indexOf("pwm_init")<main.indexOf("multicore_launch_core1"),"PWM initialization must precede Core 1 launch");
        check(main.indexOf("pio_sm_init")<main.indexOf("multicore_launch_core1"),"PIO initialization must precede Core 1 launch");
    }
    {
        auto gpio=selection("gpio","gpio.output",0);
        check(FunctionExecutionModel::supportsCoreSelection("gpio.output"),"GPIO should support execution-core selection");
        check(!FunctionExecutionModel::supportsCoreSelection("adc0"),"ADC must not claim unsupported Core 1 execution");
        gpio.settings={{"execution_core","Core 1"}};
        check(FunctionExecutionModel::effectiveCore(gpio,false)=="Core 0","disabled Core 1 must force every function to Core 0");
        check(FunctionExecutionModel::effectiveCore(gpio,true)=="Core 1","enabled Core 1 must preserve a Core 1 assignment");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto pwm=selection("pwm","pwm0a",0);
        pwm.settings={{"frequency_hz","1000"},{"wrap","999"},{"timing_mode","Automatic"}};
        state.selections.insert(pwm.componentId,pwm);
        const QString main=generate(state);
        check(main.contains("clock_get_hz(clk_sys)"),"PWM still contains a fixed system-clock assumption");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto a=selection("pio-a","pio0",4),b=selection("pio-b","pio0",12);
        a.settings={{"instruction_words","20"},{"state_machine","0"}};
        b.settings={{"instruction_words","20"},{"state_machine","1"}};
        state.selections.insert(a.componentId,a);state.selections.insert(b.componentId,b);
        QString report;
        check(!ProjectGenerator::validate(state,&report)&&report.contains("instruction-memory"),"PIO instruction memory over-allocation not rejected");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto uart=selection("uart-a","uart0_tx",4),uart2=selection("uart-b","uart0_rx",5);
        uart.settings={{"baud","115200"}}; uart2.settings={{"baud","9600"}};
        state.selections.insert(uart.componentId,uart);state.selections.insert(uart2.componentId,uart2);
        QString report;
        check(!ProjectGenerator::validate(state,&report)&&report.contains("UART0 configuration"),"UART configuration conflict not rejected");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto spi=selection("spi-a","spi0_tx",4),spi2=selection("spi-b","spi0_rx",5);
        spi.settings={{"baud","1000000"}}; spi2.settings={{"baud","2000000"}};
        state.selections.insert(spi.componentId,spi);state.selections.insert(spi2.componentId,spi2);
        QString report;
        check(!ProjectGenerator::validate(state,&report)&&report.contains("SPI0 configuration"),"SPI configuration conflict not rejected");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto i2c=selection("i2c-a","i2c0_sda",4),i2c2=selection("i2c-b","i2c0_scl",5);
        i2c.settings={{"baud","100000"}}; i2c2.settings={{"baud","400000"}};
        state.selections.insert(i2c.componentId,i2c);state.selections.insert(i2c2.componentId,i2c2);
        QString report;
        check(!ProjectGenerator::validate(state,&report)&&report.contains("I2C0 configuration"),"I2C configuration conflict not rejected");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto adc=selection("adc-a","adc0",26),adc2=selection("adc-b","adc1",27);
        state.selections.insert(adc.componentId,adc);state.selections.insert(adc2.componentId,adc2);
        QString report;
        check(!ProjectGenerator::validate(state,&report)&&report.contains("ADC active channel selector"),"ADC active-selector conflict not rejected");
    }
    {
        QTemporaryDir dir;
        auto state=baseState(dir.path(),true);
        auto led=selection("onboard_led","onboard_led.output");
        led.settings={{"execution_core","Core 1"},{"blink_enabled","true"}};
        state.selections.insert(led.componentId,led);
        QString error;
        check(ProjectStore::save(state,&error),"project persistence save failed");
        ApplicationState reopened;
        check(ProjectStore::load(dir.path(),&reopened,&error),"project persistence reopen failed");
        check(reopened.selections.value("onboard_led").settings.value("execution_core")=="Core 1","execution_core was not restored after reopen");
    }
    if(failures==0)std::cout<<"All multicore generator tests passed.\n";
    return failures==0?0:1;
}
