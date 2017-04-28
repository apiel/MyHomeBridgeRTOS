#include <lwip/api.h>
#include <string.h>
#include <espressif/esp_common.h>

#include "wifi.h"

char * get_from_uri(char * str, int start, int end, char * ret) 
{
    char *str_start, *str_end;
    str_start = strchr(str, start)+1;
    str_end = strchr(str_start, end);
    int len = str_end - str_start;
    // if (len > strlen(ret)) len = strlen(ret);  // it could be out of memory?
    memcpy(ret, str_start, len);
    ret[len] = '\0'; 

    return str_end;
}

// void parse_request(void *data)
// {
//     // printf("Received data:\n%s\n", (char*) data);
//     if (!strncmp(data, "GET ", 4)) {
//         char uri[100];
//         get_from_uri(data, ' ', ' ', uri);
//         printf("uri: %s\n", uri);

//         if (strchr(uri, '?')) {
//             char ssid[32], password[64];
//             char * next = get_from_uri(uri, '=', '&', ssid);
//             // printf(":ssid: %s :next: %s\n", ssid, next);
//             get_from_uri(next, '=', '\0', password);
//             printf(":ssid: %s :pwd: %s\n\n", ssid, password);

//             // wifi_new_connection(ssid, password);
//         }
//     }
// }

void httpd_task(void *pvParameters)
{
    struct netconn *client = NULL;
    struct netconn *nc = netconn_new(NETCONN_TCP);
    if (nc == NULL) {
        printf("Failed to allocate socket\n");
        vTaskDelete(NULL);
    }
    netconn_bind(nc, IP_ADDR_ANY, 80);
    netconn_listen(nc);
    char buf[512];
    while (1) {
        err_t err = netconn_accept(nc, &client);
        if (err == ERR_OK) {
            struct netbuf *nb;
            if ((err = netconn_recv(client, &nb)) == ERR_OK) {
                void *data;
                u16_t len;
                netbuf_data(nb, &data, &len);
                // printf("Received data:\n%.*s\n", len, (char*) data);
                // parse_request(&data);

    if (!strncmp(data, "GET ", 4)) {
        char uri[128];
        get_from_uri(data, '/', ' ', uri);
        printf("uri: %s\n", uri);

        if (strchr(uri, '?')) {
            char ssid[32], password[64];
            char * next = get_from_uri(uri, '=', '&', ssid);
            // printf(":ssid: %s :next: %s\n", ssid, next);
            get_from_uri(next, '=', '\0', password);
            printf(":ssid: %s :pwd: %s\n\n", ssid, password);

            wifi_new_connection(ssid, password);
        }
    }                

                struct sdk_station_config config;
                sdk_wifi_station_get_config(&config);
                snprintf(buf, sizeof(buf),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-type: text/html\r\n\r\n"
                        "\
                        <form>\
                            <label>Wifi SSID</label><br />\
                            <input name='ssid' placeholder='SSID' value='%s' /><br /><br />\
                            <label>Wifi password</label><br />\
                            <input name='password' placeholder='password' value='%s' /><br /><br />\
                            <button type='submit'>Save</button><br />\
                        <form>", config.ssid, config.password);
                netconn_write(client, buf, strlen(buf), NETCONN_COPY);
            }
            netbuf_delete(nb);
        }
        printf("Closing connection\n");
        netconn_close(client);
        netconn_delete(client);
    }
}