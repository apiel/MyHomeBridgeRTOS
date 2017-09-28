// esp open rtos commit 5e04c890d1b63495008c58550bcd85e0f99f9fcc

#include <string.h>

#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <dhcpserver.h>

#include <lwip/api.h>
#include <ssid_config.h>

// #include <esp_spiffs.h>

#include "config.h"
#include "wifi.h"
#include "wget.h"

#include "mqtt.h"
#ifdef HTTPD_PORT
    #include "httpd.h"
#endif
#ifdef UPNP
    #include "upnp.h"
#endif
#if defined(PIN_RF433_EMITTER) || defined(PIN_RF433_RECEIVER)
    #include "rf433.h"
#endif
#ifdef PIN_DHT
    #include "dht.h"
#endif
#ifdef PIN_PIR
    #include "pir.h"
#endif
#ifdef PHOTORESISTOR
    #include "photoresistor.h"
#endif

// #include "trigger.h"

// void yo_task(void *pvParameters)
// {
//     vTaskDelay(10000 / portTICK_PERIOD_MS);
//     while(1) {
//         vTaskDelay(5000 / portTICK_PERIOD_MS);
//         wget("http://192.168.0.13/helo");
//     }    
// }

// void spiffs_init(void)
// {
// #if SPIFFS_SINGLETON == 1
//     esp_spiffs_init();
// #else
//     // for run-time configuration when SPIFFS_SINGLETON = 0
//     esp_spiffs_init(0x200000, 0x10000);
// #endif

//     if (esp_spiffs_mount() != SPIFFS_OK) {
//         printf("Error mount SPIFFS\n");
//     }
// }

void user_init(void)
{
  uart_set_baud(0, 115200);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());
  printf("MyHomeBridge version:%s\n", VERSION);

  wifi_init(); 
  wifi_new_connection(WIFI_SSID, WIFI_PASS);

//   spiffs_init(); // for action and triggers
  mqtt_init();
// //  trigger_init();
  
  #if defined(PIN_RF433_EMITTER) || defined(PIN_RF433_RECEIVER)
  rf433_init();
  xTaskCreate(&rf433_task, "rf433_receiver", 1024, NULL, 1, NULL);
  #endif

  publish_queue = xQueueCreate(3, sizeof( struct MQTTMessage * ) );
  xTaskCreate(&mqtt_task, "mqtt_task", 1024, NULL, 4, NULL);  

  #ifdef PIN_DHT
  xTaskCreate(&dht_task, "dht_task", 1024, NULL, 5, NULL);
  #endif
  #ifdef PIN_PIR
  xTaskCreate(&pir_task, "pir_task", 1024, NULL, 5, NULL);
  #endif
  #ifdef PHOTORESISTOR
  xTaskCreate(&photoresistor_task, "photoresistor_task", 1024, NULL, 5, NULL);
  #endif

  // xTaskCreate(&yo_task, "yo_task", 1024, NULL, 1, NULL);
  #ifdef HTTPD_PORT
  xTaskCreate(&httpd_task, "http_server", 1024, NULL, 2, NULL);
  #endif

  #ifdef UPNP
  xTaskCreate(&upnp_task, "upnp_task", 1024, NULL, 5, NULL);
  #endif
}

/*

ToDo

Fix wget
rf receiver > fix pulse
MQ-9
OTA
remote debugging
unit test
trigger call reducer
trigger should work without mqtt --> thermostat

we should make rf433 action two times...
*/