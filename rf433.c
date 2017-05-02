
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include "config.h"
#include "rf433.h"
#include "pulse.h"

uint8_t latch_stage = 0;
char bits[256];
uint8_t bit;
uint8_t bit_error = 0;
struct RF433protocol * current_protocol;

struct RF433protocol rf433protocols[] = {
    { {9500 , 11500}, {2350, 2750}, {150, 450}, {950, 1450}, 64, true }
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

uint16_t rf433_middle(struct MinMax minmax) 
{
    return (minmax.max + minmax.min)*0.5;
}

void rf433_send(int id_protocol, char * code) 
{
    uint16_t low = rf433_middle(rf433protocols[id_protocol].low); 
    uint16_t hight = rf433_middle(rf433protocols[id_protocol].hight); 
    uint16_t latch = rf433_middle(rf433protocols[id_protocol].latch);
    uint16_t latch2 = rf433_middle(rf433protocols[id_protocol].latch2);     

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

void rf433_add_bit(char b)
{
    bit_error = 0;
    bits[bit] = b;

    if(bit % 2 == 1) { // we could add it only if it s valid bit combination
        if((bits[bit-1] ^ bits[bit]) == 0) { // must be either 01 or 10, cannot be 00 or 11
            latch_stage = 0;
            printf("invalid bit combination\n");
        }
    }

    bit++;

    if(bit == current_protocol->len) {
        latch_stage = 0;
        bits[bit] = '\0';
        printf("rf433 receive: %d %s\n", 0, bits);
    }    
}

void rf433_task(void *pvParameters)
{
    unsigned long pulse;
    for(;;) {
        pulse = get_pulse(PIN_RF433_RECEIVER, 0);
        // if (pulse) printf("pulse %lu\n", pulse);
        
        if (latch_stage == 0) {
            for(int i=0; i < rf433protocols_count; i++) {
                struct MinMax * minmax = &rf433protocols[i].latch;
                if (pulse > minmax->min && pulse < minmax->max) {
                    current_protocol = &rf433protocols[i];
                    latch_stage = current_protocol->latch2.max ? 1 : 2;
                    bit = 0;
                    break;
                }
            }
        }
        else if (latch_stage == 1) {
            if (pulse > current_protocol->latch2.min && pulse < current_protocol->latch2.max)
                latch_stage = 2;
        }
        else if (latch_stage == 2) {
            // printf("we rich the latch stage 2\n");

            if(pulse > current_protocol->low.min && pulse < current_protocol->low.max) {
                rf433_add_bit('0');
                // bit_error = 0;
                // bits[bit] = '0';
            }
            else if(pulse > current_protocol->hight.min && pulse < current_protocol->hight.max) {
                rf433_add_bit('1');
                // bit_error = 0;
                // bits[bit] = '1';                
            }
            else if (bit_error++ > 3) {
                latch_stage = 0;
                printf("out of range %lu\n", pulse);
                continue;
            }

            // if(bit % 2 == 1) {
            //     if((bits[bit-1] ^ bits[bit]) == 0) // must be either 01 or 10, cannot be 00 or 11
            //         latch_stage = 0;
            // }

            // bit++;

            // if(bit == current_protocol->len) {
            //     latch_stage = 0;
            //     bits[bit] = '\0';
            //     printf("rf433 receive: %d %s\n", 0, bits);
            // }               
        }        
    }
}
