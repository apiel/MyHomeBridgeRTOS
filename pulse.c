
#include <espressif/esp_common.h>
#include <lwip/api.h>
#include <esp8266.h>

#define WAIT_FOR_PIN_STATE(state) \
    while (gpio_read(pin) != (state)) { \
        if (xthal_get_ccount() - start_ccount > timeout) { \
            return 0; \
        } \
    }   

unsigned long get_pulse(uint8_t pin, uint8_t state)
{
    uint8_t cpu_freq = sdk_system_get_cpu_freq();
    unsigned long timeout = 1000000 * cpu_freq; // 1 sec
    
    uint32_t start_ccount = xthal_get_ccount();
    WAIT_FOR_PIN_STATE(!state);
    WAIT_FOR_PIN_STATE(state);
    const uint32_t pulse_start_cycle_count = xthal_get_ccount();
    WAIT_FOR_PIN_STATE(!state);
    return (xthal_get_ccount() - pulse_start_cycle_count)/cpu_freq;
}

void do_pulse(const uint8_t gpio_num, uint16_t duration, uint16_t low)
{
    gpio_write(gpio_num, 1);
    sdk_os_delay_us(low);
    gpio_write(gpio_num, 0);
    sdk_os_delay_us(duration);
}
