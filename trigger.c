#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include <spiffs.h>
#include <esp_spiffs.h>

#include "utils.h"

struct Triggers
{
    char ** list;
    size_t count;
    size_t size;
} triggers;

size_t insert_trigger(char * action) {
    size_t size = strlen(action) * sizeof(char);
    char * _action = malloc(size);
    strcpy(_action, action);

    triggers.size += size;
    triggers.list = (char **)realloc(triggers.list, triggers.size);
    triggers.list[triggers.count++] = _action;

    return triggers.count-1;
}

int search_trigger(char * action) {
    int index = 0;
    for (; index < triggers.count; index++) {
        if (strcmp(triggers.list[index], action) == 0) {
            return index;
        }
    }
    return -1;
}

void load_triggers()
{
    const int max_size = 1024;
    char buf[max_size];
    uint8_t c[1];
    int pos = 0;
    char type[2];
    char action[32];
    bool is_first_trigger = true;

    spiffs_file fd = SPIFFS_open(&fs, "triggers", SPIFFS_RDONLY, 0);
    if (fd < 0) {
        printf("---------spiff2 Error opening triggers file\n");
        return;
    }

    while (SPIFFS_read(&fs, fd, c, 1)) {
        // we should do something with the max_size!!
        if (c[0] == '\n' || SPIFFS_eof(&fs, fd)) {
            buf[pos] = '\0';             
            printf("trigger line (%d): %s\n", pos, buf);
            if (pos == 0) {
                is_first_trigger = true;
                printf("got 2 \\n, set is first trigger to true\n");
            } else {
                pos = 0;   
                if (is_first_trigger == true) {             
                    char * next = str_extract(buf, 0, ' ', type) + 1;
                    printf("trigger type: %s\n", type);
                    if (strcmp(type, "if") == 0) {
                        is_first_trigger = false;
                        printf("next: %s\n", next);
                        str_extract(next, 0, ' ', action);
                        printf("need to insert action: %s\n", action);
                        if (search_trigger(action) == -1) {
                            printf("-------- action not in list, insert.\n");
                            insert_trigger(action);
                        }                        
                    } else if (strcmp(type, "do") == 0) {
                        is_first_trigger = false;
                    }
                }
            }
            if (SPIFFS_eof(&fs, fd)) break;
        }
        else buf[pos++] = c[0];
    }

    SPIFFS_close(&fs, fd);
}

void trigger_init() 
{
    triggers.count = 0;
    triggers.size = 0;

    load_triggers();
}
