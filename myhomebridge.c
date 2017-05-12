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
#include "dht.h"
#include "pir.h"
#include "photoresistor.h"

void user_init(void)
{
  uart_set_baud(0, 115200);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());
  printf("MyHomeBridge version:%s\n", VERSION);

  wifi_init(); 
  // wifi_new_connection(WIFI_SSID, WIFI_PASS);

  rf433_init();
  xTaskCreate(&rf433_task, "rf433_receiver", 1024, NULL, 1, NULL);

  publish_queue = xQueueCreate(3, sizeof( struct MQTTMessage * ) );
  xTaskCreate(&mqtt_task, "mqtt_task", 1024, NULL, 4, NULL);  

  xTaskCreate(&httpd_task, "http_server", 1024, NULL, 2, NULL);
  xTaskCreate(&dht_task, "dht_task", 1024, NULL, 5, NULL);
  xTaskCreate(&pir_task, "pir_task", 1024, NULL, 5, NULL);
  xTaskCreate(&photoresistor_task, "photoresistor_task", 1024, NULL, 5, NULL);
}