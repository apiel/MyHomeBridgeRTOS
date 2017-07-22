// sdk_system_adc_read

#include "config.h"
#ifdef PHOTORESISTOR

#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>
#include <lwip/api.h>

#include "mqtt.h"

void photoresistor_task(void *pvParameters)
{
    struct MQTTMessage msg;
    struct MQTTMessage *pxMessage;    

    int prev_light = 0;

    printf("Start photoresistor reader.");

    while(1) {
        int light = sdk_system_adc_read();   

        if (abs(light - prev_light) > 25) {
            prev_light = light;
            printf("Light changed %d\n", light);

            pxMessage = & msg;
            strcpy(pxMessage->topic, "photoresistor");
            sprintf(pxMessage->msg, "%d", light);
            xQueueSend(publish_queue, ( void * ) &pxMessage, ( TickType_t ) 0);
        }   

        // 500ms
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

#endif

