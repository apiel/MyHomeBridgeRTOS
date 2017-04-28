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

void user_init(void)
{
  uart_set_baud(0, 115200);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());
  printf("MyHomeBridge version:%s\n", VERSION);

  wifi_init(); 
  // wifi_new_connection(WIFI_SSID, WIFI_PASS);
  	
  xTaskCreate(&httpd_task, "http_server", 1024, NULL, 2, NULL);
}