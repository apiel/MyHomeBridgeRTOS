/* Very basic example showing usage of access point mode and the DHCP server.

   The ESP in the example runs a telnet server on 172.16.0.1 (port 23) that
   outputs some status information if you connect to it, then closes
   the connection.

   This example code is in the public domain.
*/
#include <string.h>

#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <dhcpserver.h>

#include <lwip/api.h>

#include "config.h"
#include "wifi.h"
#include "httpd.h"

void user_init(void)
{
  uart_set_baud(0, 115200);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());
  printf("MyHomeBridge version:%s\n", VERSION);

  wifi_init(); 
  	
  xTaskCreate(&httpd_task, "http_server", 1024, NULL, 2, NULL);
}