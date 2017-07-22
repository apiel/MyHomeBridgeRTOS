#include "config.h"

#if defined(PIN_RF433_EMITTER) || defined(PIN_RF433_RECEIVER)

#include <espressif/esp_common.h>
#include <stdlib.h> // abs
#include <esp8266.h>
#include <string.h>

#include "rf433.h"

#define PULSE_LOW_LOW 0
#define PULSE_LOW_HIGH 1
#define PULSE_HIGH_LOW 2
#define PULSE_HIGH_HIGH 3

struct RF433protocol rf433protocolss[] = {
    { {9900 , 1000}, {2675, 180}, {275, 180}, {1225, 180}, PULSE_LOW_LOW, PULSE_LOW_HIGH, true },
    { {5700 , 50}, {0, 0}, {180, 100}, {551, 100}, PULSE_LOW_HIGH, PULSE_HIGH_LOW, false }
    // , { {55000 , 5000}, {0, 0}, {2000, 100}, {21600, 100}, PULSE_LOW_HIGH, PULSE_HIGH_LOW, false }
};

uint8_t rf433protocols_count = sizeof(rf433protocolss) / sizeof(rf433protocolss[0]);

#endif

#ifdef PIN_RF433_EMITTER

void rf433_init(void)
{
    gpio_enable(PIN_RF433_EMITTER, GPIO_OUTPUT);
    printf("RF433 protocols initialise: %d\n", rf433protocols_count);
}

void rf433_pulse(uint16_t first, uint16_t second)
{
    gpio_write(PIN_RF433_EMITTER, 1);
    sdk_os_delay_us(first);
    gpio_write(PIN_RF433_EMITTER, 0);
    sdk_os_delay_us(second);    
}

void rf433_send(int id_protocol, char * code) 
{
    uint16_t low = rf433protocolss[id_protocol].low.length; 
    uint16_t high = rf433protocolss[id_protocol].high.length; 
    uint16_t latch = rf433protocolss[id_protocol].latch.length;
    uint16_t latch2 = rf433protocolss[id_protocol].latch2.length;     

    if (latch) rf433_pulse(low, latch);
    if (latch2) rf433_pulse(low, latch2);
    int end = strlen(code);
    for(int pos = 0; pos < end; pos+=2) {
        rf433_pulse((code[pos] == '0'?low:high), (code[pos+1] == '0'?low:high));
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

#endif

#ifdef PIN_RF433_RECEIVER

bool diff(long A, long B, int tolerance) {
    return abs(A - B) < tolerance;
}

unsigned long micros()
{
    return xthal_get_ccount() / sdk_system_get_cpu_freq();
}

// unsigned long bin2int(const char *bin) 
// {
//     unsigned long dec = 0;
//     while (*bin != '\0') {
//         dec *= 2;
//         if (*bin == '1') dec += 1;
//         bin++;
//     }
//     printf("bin2int: %lu\n", dec);
//     return dec;
// }

// void int2bin(unsigned long dec)
// {
//     char bits[256];
//     for (int i = 0; dec; i++) {
//         bits[i] = dec%2 ? '1' : '0';
//         dec/=2;
//     }
//     printf("int2bin: %s\n", bits);
// }

bool is_valid_combination(char aa, char bb, uint8_t cmp) {
    uint8_t a = aa == 49; // 49 is '1'
    uint8_t b = bb == 49;
    return ((a<<b)+(a|b)) == cmp; // 0<<0=0 0<<1=0 1<<0=1 1<<1=2, if not 0<<0 add +1 -> 0|0=0 1|0=1 0|1=1 1|1=1
}

bool is_invalid_manchester_code(struct RF433protocol * protocol, uint8_t bit, char * bits) {
    return (protocol->manchester_code 
                 && bit % 4 == 3 
                 && is_valid_combination(bits[bit-3], bits[bit-2], protocol->zero) == is_valid_combination(bits[bit-1], bits[bit], protocol->zero));
}

bool is_invalid_zero_or_one(struct RF433protocol * protocol, uint8_t bit, char * bits) {
    return (bit % 2 == 1 
                 && !is_valid_combination(bits[bit-1], bits[bit], protocol->zero) 
                 && !is_valid_combination(bits[bit-1], bits[bit], protocol->one));
}

int rf433_get_protocol(unsigned int duration)
{
    for(int i=0; i < rf433protocols_count; i++) {
        struct Pulse * latch = &rf433protocolss[i].latch;
        if (diff(duration, latch->length, latch->tolerance)) {
            // printf("Latch on protocol %d.\n", i);
            return i;
        }
    }
    return -1;
}

void rf433_task(void *pvParameters)
{
    gpio_enable(PIN_RF433_RECEIVER, GPIO_INPUT);
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

    printf("Start rf433 receiver.");

    for(;;) {
        int pin_value = gpio_read(PIN_RF433_RECEIVER);
        // printf("%d", pin_value);

        if (pin_value != previous_pin_value) {
            previous_pin_value = pin_value;
            unsigned long time = micros();
            unsigned int duration = time - last_time;
            // printf("d: %d\n", duration);

            if (protocol_key != -1 && diff(duration, trigger, 200)) {
                bits[bit] = '\0';
                printf("End signal, should display result: %d %s\nreset.\n", bit, bits);
                // int2bin(bin2int(bits));
                protocol_key = -1;
            }

            if (protocol_key == -1) {
                protocol_key = rf433_get_protocol(duration);
                if (protocol_key > -1) {
                    trigger = duration;
                    protocol = NULL;
                    bit = 0;
                    error = 0; 
                    latch2 = &rf433protocolss[protocol_key].latch2;
                    // printf("--- key: %d duration: %d latch2: %d\n", protocol_key, duration, latch2->length);
                    if (!latch2->length) {
                        // printf("No latch 2\n");
                        protocol = &rf433protocolss[protocol_key];
                    }
                }
            }
            else if (!protocol) {
                if (diff(duration, latch2->length, latch2->tolerance)) {
                    // printf("Latch 2 detected\n");
                    protocol = &rf433protocolss[protocol_key];
                }
                else if (error++ > 0) {
                    // printf("Error latch 2 %d\n", duration);
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

#endif


// protocol 1
// WC on 0101010101100110010101100110011001011010010110100
// WC off 0101010101100110010101100110011001011010101001010
// wall on 0101010101100110010101100110011010100101010110100
// wall off 0101010101100110010101100110011010100101101001010
// kitchen on 0101010101100110010101100110101001010101010110100
// kitchen off 0101010101100110010101100110101001010101101001010
// chill on 0101010101100101011001100110011001011010010110100
// chill off 0101010101100101011001100110011001011010101001010
// windows on 0101010101100101011001100110101001010101010110100
// windows off 0101010101100101011001100110101001010101101001010
// table on 0101010101100101011001100110011010100101010110100
// table off 0101010101100101011001100110011010100101101001010
