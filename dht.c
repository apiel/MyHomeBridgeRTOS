#include <espressif/esp_common.h>
#include <dht/dht.h>
#include <esp8266.h>
#include <string.h>
#include <lwip/api.h>

#include "config.h"
#include "mqtt.h"

void dht_task(void *pvParameters)
{
    struct MQTTMessage tempMsg;
    struct MQTTMessage *pxMessage;

    struct MQTTMessage humidityMsg;
    struct MQTTMessage *px2Message;    
    
    int16_t temperature = 0;
    int16_t humidity = 0;

    printf("Start dht reader.");

    // DHT sensors that come mounted on a PCB generally have
    // pull-up resistors on the data pin.  It is recommended
    // to provide an external pull-up resistor otherwise...
    gpio_set_pullup(PIN_DHT, false, false);

    while(1) {
        if (dht_read_data(DHT_TYPE, PIN_DHT, &humidity, &temperature)) {
            printf("Humidity: %.1f%% Temp: %.1fC\n", 
                    (float)humidity / 10, 
                    (float)temperature / 10);

            pxMessage = & tempMsg;
            strcpy(pxMessage->topic, "temperature");
            sprintf(pxMessage->msg, "%.1f", (float)temperature / 10);
            xQueueSend(publish_queue, ( void * ) &pxMessage, ( TickType_t ) 0);

            px2Message = & humidityMsg;
            strcpy(px2Message->topic, "humidity");
            sprintf(px2Message->msg, "%.1f", (float)humidity / 10);
            xQueueSend(publish_queue, ( void * ) &px2Message, ( TickType_t ) 0);
        } else {
            printf("Could not read data from sensor\n");
        }

        // 30 second delay...
        vTaskDelay(30000 / portTICK_PERIOD_MS);
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
