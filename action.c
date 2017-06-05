#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <spiffs.h>
#include <esp_spiffs.h>

#include "rf433.h"
#include "utils.h"

void reducer(char * action, char * params);

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
}

// this could be call from an internal queue
void reducer(char * action, char * params)
{
    if (strcmp(action, "rf433") == 0) {
        rf433_action(params);
    }
    else if (strcmp(action, "actions") == 0) {
        read_actions(params);
    }    
    else if (strcmp(action, "#") == 0) {
        printf("-> %s\n", params);
    }    
    else {
        printf("This action is not supported.\n");
    }
}

void action_init()
{
#if SPIFFS_SINGLETON == 1
    esp_spiffs_init();
#else
    // for run-time configuration when SPIFFS_SINGLETON = 0
    esp_spiffs_init(0x200000, 0x10000);
#endif

    if (esp_spiffs_mount() != SPIFFS_OK) {
        printf("Error mount SPIFFS\n");
    }

    // read_action();
}
