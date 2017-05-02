
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include "config.h"
#include "rf433.h"
#include "pulse.h"

struct RF433protocol rf433protocols[] = {
    { 10500, 2500, 300, 1200, 64, true }
};

uint8_t rf433protocols_count = sizeof(rf433protocols) / sizeof(rf433protocols[0]);

void rf433_init(void)
{
    gpio_enable(PIN_RF433_EMITTER, GPIO_OUTPUT);
    printf("RF433 protocols initialise: %d\n", rf433protocols_count);
}

void rf433_pulse(uint16_t duration, uint16_t low)
{
    do_pulse(PIN_RF433_EMITTER, duration, low);
}


void rf433_send(int id_protocol, char * code) 
{
    uint16_t low = rf433protocols[id_protocol].low; 
    uint16_t hight = rf433protocols[id_protocol].hight; 
    uint16_t latch = rf433protocols[id_protocol].latch;
    uint16_t latch2 = rf433protocols[id_protocol].latch2;     

    // printf("rf433_send protocol: %d latch: %d latch2: %d low: %d hight: %d code: %s\n", id_protocol, latch, latch2, low, hight, code);   

    if (latch) rf433_pulse(latch, low);
    if (latch2) rf433_pulse(latch2, low);
    int end = strlen(code);
    for(int pos = 0; pos < end; pos++) {
      if (code[pos] == '0') {
        rf433_pulse(low, low);
      }
      else {
        rf433_pulse(hight, low);
      }
    }
}

void rf433_send_multi(int id_protocol, char * code) 
{
    rf433_send(id_protocol, code);
    rf433_send(id_protocol, code);
    rf433_send(id_protocol, code);
    rf433_send(id_protocol, code);
    rf433_send(id_protocol, code);
}

// on1: 0 1010100110101001011010100110011010100110011010011001011010101010

void rf433_action(char * request)
{
    char * id_protocol_str = strtok(request, " ");
    char * code = strtok(NULL, " ");

    if (id_protocol_str && code) {
        int id_protocol = atoi(id_protocol_str);
        printf("rf433_action id: %d code: %s\n", id_protocol, code);
        if (id_protocol < rf433protocols_count)
            rf433_send_multi(id_protocol, code);
        else
            printf("rf433_action invalid protocol.\n");
    }
    else {
        printf("rf433_action invalid parameters: %s\n", request);
    }
}

void rf433_task(void *pvParameters)
{
    unsigned long pulse;
    for(;;) {
        pulse = get_pulse(PIN_RF433_RECEIVER, 0, 9999999999999);
        if (pulse) printf("pulse %lu\n", pulse);
    }
}
