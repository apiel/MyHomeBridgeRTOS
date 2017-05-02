
#include <espressif/esp_common.h>
#include <lwip/api.h>
#include <esp8266.h>

extern uint32_t xthal_get_ccount();

#ifndef F_CPU
#define F_CPU 800000000L
#endif

#ifndef UINT_MAX
#define UINT_MAX 65535
#endif

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )

#define WAIT_FOR_PIN_STATE(state) \
    while (gpio_read(pin) != (state)) { \
        if (xthal_get_ccount() - start_cycle_count > timeout_cycles) { \
            return 0; \
        } \
        taskYIELD(); \
    }   

// sdk_os_get_cpu_frequency()
// void sdk_os_delay_us(uint16_t us) {
//     uint32_t start_ccount = xthal_get_ccount();
//     uint32_t delay_ccount = cpu_freq * us;
//     while (xthal_get_ccount() - start_ccount < delay_ccount) {}
// }

// max timeout is 27 seconds at 160MHz clock and 54 seconds at 80MHz clock
unsigned long get_pulse(uint8_t pin, uint8_t state, unsigned long timeout)
{
    const uint32_t max_timeout_us = clockCyclesToMicroseconds(UINT_MAX);
    if (timeout > max_timeout_us) {
        timeout = max_timeout_us;
    }
    const uint32_t timeout_cycles = microsecondsToClockCycles(timeout);
    const uint32_t start_cycle_count = xthal_get_ccount();
    WAIT_FOR_PIN_STATE(!state);
    WAIT_FOR_PIN_STATE(state);
    const uint32_t pulse_start_cycle_count = xthal_get_ccount();
    WAIT_FOR_PIN_STATE(!state);
    return clockCyclesToMicroseconds(xthal_get_ccount() - pulse_start_cycle_count);
}

void do_pulse(const uint8_t gpio_num, uint16_t duration, uint16_t low)
{
    gpio_write(gpio_num, 1);
    sdk_os_delay_us(low);
    gpio_write(gpio_num, 0);
    sdk_os_delay_us(duration);
}
