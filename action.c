#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <spiffs.h>
#include <esp_spiffs.h>

#include "config.h"

#ifdef PIN_RF433_EMITTER
    #include "rf433.h"
#endif

#include "utils.h"
#include "wget.h"
#include "action.h"

#ifdef UPNP
    #include "upnp.h"
#endif

void read_actions(char * name)
{
    const int max_size = 1024;
    char buf[max_size];
    uint8_t c[1];
    int pos = 0;
    char action[32];
    char path[128] = "actions_";
    strcat(path, name);

    spiffs_file fd = SPIFFS_open(&fs, path, SPIFFS_RDONLY, 0);
    if (fd < 0) {
        printf("---------spiff2 Error opening file %s\n", path);
        return;
    }

    while (SPIFFS_read(&fs, fd, c, 1)) {
        if (c[0] == '\n' || SPIFFS_eof(&fs, fd)) {
            buf[pos] = '\0'; 
            printf("-------------- line: %s\n", buf);
            char * next = str_extract(buf, 0, ' ', action) + 1;
            printf("--------::::::action: %s :param: %s\n\n", action, next);
            reducer(action, next);
            pos = 0;
            if (SPIFFS_eof(&fs, fd)) break;
        }
        else buf[pos++] = c[0];
    }

    SPIFFS_close(&fs, fd);
}

// this could be call from an internal queue
char * reducer(char * action, char * params)
{
    char * response = NULL;
    if (strcmp(action, "wget") == 0) {
        wget(params);
    }
    #ifdef PIN_RF433_EMITTER
    else if (strcmp(action, "rf433") == 0) {
        rf433_action(params);
    }
    #endif
    #ifdef UPNP
    else if (strcmp(action, "api") == 0) {
        response = upnp_action(params);
    }
    #endif
    else if (strcmp(action, "actions") == 0) {
        // run_actions(params);
        read_actions(params);
    } 
    else if (strcmp(action, "#") == 0) {
        printf("-> %s\n", params);
    }    
    else {
        printf("This action is not supported.\n");
    }

    return response;
}
