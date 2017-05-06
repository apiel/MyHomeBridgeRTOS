
#include <espressif/esp_common.h>
#include <stdlib.h> // abs
#include <esp8266.h>
#include <string.h>

#include "config.h"
#include "rf433.h"
#include "pulse.h"

// uint8_t latch_stage = 0;
// char bits[256];
// uint8_t bit;
// uint8_t bit_error = 0;
struct RF433protocol2 * current_protocol;
int current_protocol_id;

struct RF433protocol2 rf433protocols[] = {
    // { {9500 , 11500}, {2350, 2750}, {130, 450}, {950, 1450}, 64, true },
    { {10000 , 11000}, {2350, 2750}, {130, 450}, {950, 1450}, 64, true },
    { {5670 , 5730}, {0, 0}, {150, 250}, {500, 610}, 24, false }
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

#define PULSE_LOW_LOW 0
#define PULSE_LOW_HIGH 1
#define PULSE_HIGH_LOW 2
#define PULSE_HIGH_HIGH 3

struct RF433protocol rf433protocolss[] = {
    { {9900 , 1000}, {2675, 180}, {275, 180}, {1225, 180}, PULSE_LOW_LOW, PULSE_LOW_HIGH, 64, true },
    { {5700 , 50}, {0, 0}, {180, 100}, {551, 100}, PULSE_LOW_HIGH, PULSE_HIGH_LOW, 0, false }
};

uint8_t rf433protocolss_count = sizeof(rf433protocolss) / sizeof(rf433protocolss[0]);

bool diff(long A, long B, int tolerance) {
    return abs(A - B) < tolerance;
}

unsigned long micros()
{
    return xthal_get_ccount() / sdk_system_get_cpu_freq();
}

bool is_valid(char aa, char bb, uint8_t cmp) {
    uint8_t a = aa == 49; // 49 is '1'
    uint8_t b = bb == 49;
    return ((a<<b)+(a|b)) == cmp; // 0<<0=0 0<<1=0 1<<0=1 1<<1=2, if not 0<<0 add +1 -> 0|0=0 1|0=1 0|1=1 1|1=1
}

bool is_invalid_manchester_code(struct RF433protocol * protocol, uint8_t bit, char * bits) {
    return (protocol->manchester_code 
                 && bit % 4 == 3 
                 && is_valid(bits[bit-3], bits[bit-2], protocol->zero) == is_valid(bits[bit-1], bits[bit], protocol->zero));
}

bool is_invalid_zero_or_one(struct RF433protocol * protocol, uint8_t bit, char * bits) {
    return (bit % 2 == 1 
                 && !is_valid(bits[bit-1], bits[bit], protocol->zero) 
                 && !is_valid(bits[bit-1], bits[bit], protocol->one));
}

int rf433_get_protocol(unsigned int duration)
{
    for(int i=0; i < rf433protocolss_count; i++) {
        struct Pulse * latch = &rf433protocolss[i].latch;
        if (diff(duration, latch->length, latch->tolerance)) {
            printf("Latch on protocol %d.\n", i);
            return i;
        }
    }
    return -1;
}

void rf433_task(void *pvParameters)
{
    int previous_pin_value = gpio_read(PIN_RF433_RECEIVER);
    unsigned long last_time = micros();
    int protocol_key = -1;
    struct Pulse * latch2;
    struct RF433protocol * protocol = NULL;
    unsigned int trigger = 0;
    uint8_t bit = 0;
    char bits[256];
    bool low;
    bool high;  
    uint8_t error = 0; 
   uint8_t end_bit = 0; 

    printf("Start rf433 receiver.");

    for(;;) {
        int pin_value = gpio_read(PIN_RF433_RECEIVER);
        // printf("%d", pin_value);

        if (pin_value != previous_pin_value) {
            previous_pin_value = pin_value;
            unsigned long time = micros();
            unsigned int duration = time - last_time;

            if (protocol_key != -1 && (diff(duration, trigger, 200) || (end_bit && bit > end_bit))) {
                bits[bit] = '\0';
                printf("End signal, should display result: %d %s\nreset.\n", bit, bits);
                protocol_key = -1;
            }

            if (protocol_key == -1) {
                protocol_key = rf433_get_protocol(duration);
                if (protocol_key > -1) {
                    trigger = duration;
                    protocol = NULL;
                    bit = 0;
                    error = 0;
                    end_bit = rf433protocolss[protocol_key].len*2;  
                    latch2 = &rf433protocolss[protocol_key].latch2;
                    // printf("--- key: %d duration: %d latch2: %d\n", protocol_key, duration, latch2->length);
                    if (!latch2->length) {
                        printf("No latch 2\n");
                        protocol = &rf433protocolss[protocol_key];
                    }
                }
            }
            else if (!protocol) {
                if (diff(duration, latch2->length, latch2->tolerance)) {
                    printf("Latch 2 detected\n");
                    protocol = &rf433protocolss[protocol_key];
                }
                else if (error++ > 0) {
                    printf("Error latch 2 %d\n", duration);
                    protocol_key = -1;
                }
            }
            else if ((low = diff(duration, protocol->low.length, protocol->low.tolerance))
             || (high = diff(duration, protocol->high.length, protocol->high.tolerance))) {
                bits[bit] = low ? '0' : '1';
                // printf(":: %c %d\n", bits[bit], duration);
                if (is_invalid_zero_or_one(protocol, bit, bits)
                 || is_invalid_manchester_code(protocol, bit, bits)) {
                    printf("Error invalid 0 or 1 or dont respect manchester code\n");
                    protocol_key = -1;
                }
                bit++;
            }
            else {
                // printf("Error ot of range %d\n", duration);
                protocol_key = -1;
            }

            last_time = time;                 
        }
    }
}
