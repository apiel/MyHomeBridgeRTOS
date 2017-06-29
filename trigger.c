#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include <spiffs.h>
#include <esp_spiffs.h>

#include "utils.h"
#include "config.h"
#include "mqtt.h"

#define state_size 24

size_t states_count = 0;

struct State
{
    char name[128];
    char value[256];
};
struct State states[state_size];

// struct States
// {
//     struct State ** list;
//     size_t count;
//     size_t size;
// } states;

void list_states() {
    int index = 0;
    for (; index < states_count; index++) {
        printf("state: %s val: %s\n", states[index].name, states[index].value);
    }
}

int search_state(char * state_name) {
    int index = 0;
    for (; index < states_count; index++) {
        // printf("cmp: %s vs %s\n", states.list[index]->name, state_name);
        if (strcmp(states[index].name, state_name) == 0) {
            return index;
        }
    }
    return -1;
}

bool update_state(char * state_name, char * value) {
    int index = search_state(state_name);
    if (index > -1 && strcmp(value, states[index].value) != 0) {
        strcpy(states[index].value, value);
        states[index].value[strlen(value)] = '\0';
        return true;
    }
    return false;
}

// we should rename it... and maybe state_name should called state or event or ??
void watch_state(char * state_name) {
    printf("watch state_name: %s\n", state_name);

    if (search_state(state_name) == -1) {
        printf("state not in list, insert it\n");
        if (states_count < state_size) {
            strcpy(states[states_count].name, state_name);
            strcpy(states[states_count].value, "\0");
            states_count++;

            insert_topic(state_name);
        } else {
            printf("state cannot be inserted, limit of %d reached.\n", state_size);
        }
    }
}

bool is_valid = true;

void parser_init_triggers(char * line) 
{
    char type[2];
    char state_name[32];

    char * next = str_extract(line, 0, ' ', type) + 1;
    // printf("trigger type: %s\n", type);
    if (strcmp(type, "if") == 0) {
        // printf("next: %s\n", next);
        str_extract(next, 0, ' ', state_name);                     
        is_valid = false;
        watch_state(state_name);
    }
}

void parser_triggers(char * line) 
{
    char type[2];
    char state_name[32];
    char operator[2];

    if (is_valid) {
        char * next = str_extract(line, 0, ' ', type) + 1;
        printf("trigger type: %s\n", type);
        if (strcmp(type, "if") == 0) {
            printf("next: %s\n", next);
            next = str_extract(next, 0, ' ', state_name) + 1;
            int index = search_state(state_name); 
            printf("Search for state: %s (%d)\n", state_name, index);            
            is_valid = index > -1;
            if (is_valid) {
                char * value = states[index].value;
                next = str_extract(next, 0, ' ', operator) + 1;
                printf("operator: '%s'\n", operator);
                if (strcmp(operator, "is") == 0) {
                    printf("is operator, %s\n", next);
                    is_valid = strcmp(next, value) == 0;
                }
                else if (strcmp(operator, ">") == 0) {
                    printf("> operator, need to convert to numeric\n");
                    is_valid = atof(value) > atof(next); // we should maybe verify that it is numeric value
                }
                else if (strcmp(operator, "<") == 0) {
                    printf("< operator, need to convert to numeric\n");
                    is_valid = atof(value) < atof(next); // we should maybe verify that it is numeric value
                }
                else {
                    printf("unknown operator.\n");
                }             
            }
            else {
                printf("Something strange happen, state not watched.\n");
            }
            printf("is valid %d\n", is_valid);
        }
        else if (strcmp(type, "do") == 0) {
            printf("do state_name: %s\n", next);
        }
    }      
}

void parse_triggers_file(void (*parser_callback)(char * line))
{
    const int max_size = 1024;
    char buf[max_size];
    uint8_t c[1];
    int pos = 0;

    spiffs_file fd = SPIFFS_open(&fs, "triggers", SPIFFS_RDONLY, 0);
    if (fd < 0) {
        printf("---------spiff2 Error opening triggers file\n");
        return;
    }

    is_valid = true;
    while (SPIFFS_read(&fs, fd, c, 1)) {
        // we should do something with the max_size!!
        if (c[0] == '\n' || SPIFFS_eof(&fs, fd)) {
            buf[pos] = '\0';             
            // printf("trigger line (%d): %s\n", pos, buf);
            if (pos == 0) {
                is_valid = true;
                // printf("got 2 \\n, set is first trigger to true\n");
            } else {
                pos = 0; 
                parser_callback(buf);
            }
            if (SPIFFS_eof(&fs, fd)) break;
        }
        else buf[pos++] = c[0];
    }

    SPIFFS_close(&fs, fd);
}

void trigger_init() 
{
    parse_triggers_file(parser_init_triggers);
}

void trigger(char * state_name, char * value) {
    if (update_state(state_name, value)) {
        printf("State changed, we should check for trigger.\n");
        parse_triggers_file(parser_triggers);
        // list_states();        
    }
}
