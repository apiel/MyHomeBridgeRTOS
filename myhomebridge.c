#include <string.h>

#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <dhcpserver.h>

#include <lwip/api.h>
#include <ssid_config.h>

#include "config.h"
#include "wifi.h"
#include "httpd.h"
#include "mqtt.h"
#include "rf433.h"

void  beat_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    struct MQTTMessage *pxMessage;
    int count = 0;

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, 10000 / portTICK_PERIOD_MS);
        printf("beat\r\n");
        pxMessage = & mqttMessage;

        strcpy(pxMessage->topic, "helo");
        snprintf(pxMessage->msg, strlen(pxMessage->msg), "Beat %d\r\n", count++);
        if (xQueueSend(publish_queue, ( void * ) &pxMessage, ( TickType_t ) 0) == pdFALSE) {
            printf("Publish queue overflow.\r\n");
        }
    }
}

void user_init(void)
{
  uart_set_baud(0, 115200);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());
  printf("MyHomeBridge version:%s\n", VERSION);

  wifi_init(); 
  // wifi_new_connection(WIFI_SSID, WIFI_PASS);

  rf433_init();
  	
  publish_queue = xQueueCreate(3, sizeof( struct MQTTMessage * ) );

  xTaskCreate(&httpd_task, "http_server", 1024, NULL, 2, NULL);
//   xTaskCreate(&beat_task, "beat_task", 256, NULL, 3, NULL);
  xTaskCreate(&mqtt_task, "mqtt_task", 1024, NULL, 4, NULL);  
}