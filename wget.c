#include <espressif/esp_common.h>

#include <espressif/esp_common.h>
#include <string.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

#include "utils.h"

void wget(char * url)
{
    char * next;
    char host[128];

    if (strstr(url, "http://") != NULL) {
        url += 7;
        next = str_extract(url, 0, '/', host);
        printf("-->> host: %s params: %s\n", host, next);

        const struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
        };
        struct addrinfo *res;

        printf("Running DNS lookup for %s...\r\n", host);
        int err = getaddrinfo(host, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            printf("DNS lookup failed err=%d res=%p\r\n", err, res);
        } else {
            struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
            printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

            int s = socket(res->ai_family, res->ai_socktype, 0);
            if(s < 0) {
                printf("... Failed to allocate socket.\r\n");
            } else {
                printf("... allocated socket\r\n");
                if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
                    printf("... socket connect failed.\r\n");
                } else {
                    printf("... connected\r\n");
                    char req[1024];
                    strcpy(req, "GET ");
                    strcat(req, next);
                    strcat(req, " HTTP/1.1\r\nHost: ");
                    strcat(req, host);
                    strcat(req, "\r\nUser-Agent: esp-open-rtos/0.1 esp8266\r\n\r\n\r\n");
                    // printf("... req:\n%s", req);
                    if (write(s, req, strlen(req)) < 0) {
                        printf("... socket send failed\r\n");
                    } else {
                        printf("... socket send success\r\n");                    
                    }
                }
                close(s);                    
            }
        }
        freeaddrinfo(res);
    }
}
