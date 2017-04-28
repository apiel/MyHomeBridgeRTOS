#include <lwip/api.h>
#include <string.h>

char * get_from_uri(char * str, int start, int end, char * ret) 
{
    const int max_len = strlen(ret);
    char *str_start, *str_end;
    str_start = memchr(str, start, max_len)+1;
    str_end = memchr(str_start, end, max_len);
    int len = str_end - str_start;
    memcpy(ret, str_start, len);
    ret[len] = '\0'; 

    return str_end;
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
                void *data;
                u16_t len;
                netbuf_data(nb, &data, &len);
                // printf("Received data:\n%.*s\n", len, (char*) data);


                if (!strncmp(data, "GET ", 4)) {
                    char uri[50];
                    get_from_uri(data, ' ', ' ', uri);
                    printf("uri: %s\n", uri);

                    if (strchr(uri, '?')) {
                        char ssid[50], password[50];
                        char * next = get_from_uri(uri, '=', '&', ssid);
                        // printf(":ssid: %s :next: %s\n", ssid, next);
                        get_from_uri(next, '=', '\0', password);
                        printf(":ssid: %s :pwd: %s\n\n", ssid, password);
                    }
                }



                snprintf(buf, sizeof(buf),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-type: text/html\r\n\r\n"
                        "\
                        <form>\
                            <label>Wifi SSID</label><br />\
                            <input name='ssid' placeholder='SSID' value='' /><br /><br />\
                            <label>Wifi password</label><br />\
                            <input name='password' placeholder='password' value='' /><br /><br />\
                            <button type='submit'>Save</button><br />\
                        <form>");
                netconn_write(client, buf, strlen(buf), NETCONN_COPY);
            }
            netbuf_delete(nb);
        }
        printf("Closing connection\n");
        netconn_close(client);
        netconn_delete(client);
    }
}