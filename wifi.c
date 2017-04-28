#include <string.h>

#include <espressif/esp_common.h>
#include <dhcpserver.h>

#include "config.h"
#include "wifi.h"

void wifi_init(void)
{
  sdk_wifi_set_opmode(STATIONAP_MODE); 

  wifi_connect();
  wifi_access_point();
}

void wifi_new_connection(char * ssid, char * password)
{
    printf("Connect to new wifi: %s %s", ssid, password);
    // struct sdk_station_config config = {
    //     .ssid = ssid,
    //     .password = password,
    // };

    struct sdk_station_config config;    
    if(ssid != NULL) {
        strcpy((char*)(config.ssid), ssid);
        
        if(password != NULL) {
            strcpy((char*)(config.password), password);
        }
        else {
            config.password[0] = '\0';
        }
    }
    else {
        config.ssid[0]     = '\0';
        config.password[0] = '\0';
    }    

    sdk_wifi_station_set_config(&config);
    // sdk_wifi_station_disconnect();
    sdk_wifi_station_connect();
}

void wifi_connect(void)
{
    struct sdk_station_config config;
    bool ret = sdk_wifi_station_get_config(&config);
    if(ret) printf("existing wifi settings: ssid = %s, password = %s\n", config.ssid, config.password);
    else printf("no wifi settings founds: ssid = %s, password = %s\n", config.ssid, config.password);

    // sdk_wifi_station_disconnect();
    sdk_wifi_station_connect();  
}

void wifi_access_point(void)
{
  struct ip_info ap_ip;
  IP4_ADDR(&ap_ip.ip, 172, 16, 0, 1);
  IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
  IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
  sdk_wifi_set_ip_info(1, &ap_ip);

  struct sdk_softap_config ap_config = {
      .ssid = AP_SSID,
      .ssid_hidden = 0,
      .channel = 3,
      .ssid_len = strlen(AP_SSID),
      .authmode = AUTH_WPA_WPA2_PSK,
      .password = AP_PSK,
      .max_connection = 1,
      .beacon_interval = 100,
  };
  sdk_wifi_softap_set_config(&ap_config);

  ip_addr_t first_client_ip;
  IP4_ADDR(&first_client_ip, 172, 16, 0, 2);
  dhcpserver_start(&first_client_ip, 4);     
}