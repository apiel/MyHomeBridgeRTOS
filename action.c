#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <spiffs.h>
#include <esp_spiffs.h>

#include "rf433.h"
#include "utils.h"

struct Actions_definition
{
    char ** list;
    size_t count;
    size_t size;
 } actions_def;

struct Actions
{
    size_t * list;
    size_t count;
    size_t size;
    char * name;    
};

struct Actions_database
{
    struct Actions * list;
    size_t count;
    size_t size;
 } actions_db;

void reducer(char * action, char * params);

size_t insert_action(char * action) {
    size_t size = strlen(action) * sizeof(char);
    char * _action = malloc(size);
    strcpy(_action, action);

    actions_def.size += size;
    actions_def.list = (char **)realloc(actions_def.list, actions_def.size);
    actions_def.list[actions_def.count++] = _action;

    return actions_def.count-1;
}

int search_action(char * action) {
    int index = 0;
    for (; index < actions_def.count; index++) {
        if (strcmp(actions_def.list[index], action) == 0) {
            return index;
        }
    }
    return -1;
}

void load_actions_file(char * path)
{
    const int max_size = 1024;
    char buf[max_size];
    uint8_t c[1];
    int pos = 0;
    char action[32];
    struct Actions * actions;
    size_t index_action;

    spiffs_file fd = SPIFFS_open(&fs, path, SPIFFS_RDONLY, 0);
    if (fd < 0) {
        printf("---------spiff2 Error opening file %s\n", path);
        return;
    }

    actions = malloc(sizeof(struct Actions));
    actions->size = 0;
    actions->count = 0;
    // we could remove actions_ from the name // maybe with path+=8;
    actions->name = malloc(strlen(path) * sizeof(char));
    strcpy(actions->name, path);

    while (SPIFFS_read(&fs, fd, c, 1)) {
        if (c[0] == '\n' || SPIFFS_eof(&fs, fd)) {
            buf[pos] = '\0'; 
            str_extract(buf, 0, ' ', action);
            printf("action line: %s\n", buf);
            if (!strcmp(action, "#") == 0) {
                if ((index_action = search_action(buf)) == -1) {
                    printf("-------- action not in list, insert.\n");
                    index_action = insert_action(buf);
                }
                printf("-- action index %d\n", index_action);
                actions->size += sizeof(size_t);
                if (!actions->count) actions->list = (size_t *)malloc(actions->size);
                else actions->list = (size_t *)realloc(actions->list, actions->size);
                actions->list[actions->count++] = index_action;
            }
            pos = 0;
            if (SPIFFS_eof(&fs, fd)) break;
        }
        else buf[pos++] = c[0];
    }
}

void load_actions()
{
  spiffs_DIR d;
  struct spiffs_dirent e;
  struct spiffs_dirent *pe = &e;
  char base[8]; // actions_

  SPIFFS_opendir(&fs, "/", &d);
  while ((pe = SPIFFS_readdir(&d, pe))) {
    printf("action file: %s [%04x] size:%i\n", pe->name, pe->obj_id, pe->size);
    memcpy(base, pe->name, 8);
    if (strcmp(base, "actions_") == 0) {
        load_actions_file((char *)pe->name);
    }
  }
  SPIFFS_closedir(&d);    
}

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

    actions_def.count = 0;
    actions_def.size = 0;

    actions_db.count = 0;
    actions_db.size = 0;

    load_actions();
}
