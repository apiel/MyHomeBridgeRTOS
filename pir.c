#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>
#include <lwip/api.h>

#include "config.h"
#include "mqtt.h"

void pir_task(void *pvParameters)
{
    struct MQTTMessage pirMsg;
    struct MQTTMessage *pxMessage;    

    gpio_enable(PIN_PIR, GPIO_INPUT);
    // gpio_enable(PIN_PIR, GPIO_OUTPUT);
    // gpio_enable(PIN_PIR, GPIO_OUT_OPEN_DRAIN);    
    // gpio_set_pullup(PIN_PIR, false, false);

    bool prev_pir_value = gpio_read(PIN_PIR);

    printf("Start pir reader.");

    while(1) {
        int pir_value = gpio_read(PIN_PIR);
        if (pir_value != prev_pir_value) {
            prev_pir_value = pir_value;
            printf("Pir changed %d\n", pir_value);

            pxMessage = & pirMsg;
            strcpy(pxMessage->topic, "pir");
            sprintf(pxMessage->msg, "%d", pir_value);
            xQueueSend(publish_queue, ( void * ) &pxMessage, ( TickType_t ) 0);            
        }        

        // 100ms
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

