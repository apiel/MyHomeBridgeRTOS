#include <lwip/api.h>
#include <string.h>
#include <espressif/esp_common.h>

#include "wifi.h"
#include "action.h"
#include "utils.h"

inline int ishex(int x)
{
	return	(x >= '0' && x <= '9')	||
		(x >= 'a' && x <= 'f')	||
		(x >= 'A' && x <= 'F');
}
 
int decode(const char *s, char *dec)
{
	char *o;
	const char *end = s + strlen(s);
	int c;
 
	for (o = dec; s <= end; o++) {
		c = *s++;
		if (c == '+') c = ' ';
		else if (c == '%' && (	!ishex(*s++)	||
					!ishex(*s++)	||
					!sscanf(s - 2, "%2x", &c)))
			return -1;
 
		if (dec) *o = c;
	}
 
	return o - dec;
}

void parse_request(void *data)
{
    // printf("Received data:\n%s\n", (char*) data);
    if (!strncmp(data, "GET ", 4)) {
        char uri[128];
        str_extract(data, '/', ' ', uri);
        printf("uri: %s\n", uri);

        if (strchr(uri, '/')) {
            char action[32];
            char * next = str_extract(uri, 0, '/', action) + 1;
            char_replace(next, '/', ' ');
            printf("::::::action: %s :param: %s\n\n", action, next);
            reducer(action, next);
        }
        else if (strchr(uri, '?')) {
            char ssid[32], password[64];
            char * next = str_extract(uri, '=', '&', ssid);
            // printf(":ssid: %s :next: %s\n", ssid, next);
            str_extract(next, '=', '\0', password);
            // printf(":ssid: %s :pwd: %s\n\n", ssid, password);

            char ssid2[32], password2[64];
            decode(ssid, ssid2);
            decode(password, password2);

            printf(":ssid: %s :pwd: %s\n\n", ssid2, password2);

            wifi_new_connection(ssid2, password2);
        }
    }
}

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
                void *data = NULL;
                u16_t len;
                if (netbuf_data(nb, &data, &len) == ERR_OK) {
                    // printf("Received data:\n%.*s\n", len, (char*) data);
                    parse_request(data);                         
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