#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include <spiffs.h>
#include <esp_spiffs.h>

#include "utils.h"
#include "config.h"
#include "mqtt.h"

struct Variable
{
    char * name;
    char value[256];
};

struct Variables
{
    struct Variable ** list;
    size_t count;
    size_t size;
} variables;

struct Triggers
{
    char ** list;
    size_t count;
    size_t size;
} triggers;

void list_variables() {
    int index = 0;
    for (; index < variables.count; index++) {
        printf("var: %s val: %s\n", variables.list[index]->name, variables.list[index]->value);
    }
}

// rename variable to state
int search_variable(char * action) {
    int index = 0;
    for (; index < variables.count; index++) {
        // printf("cmp: %s vs %s\n", variables.list[index]->name, action);
        if (strcmp(variables.list[index]->name, action) == 0) {
            return index;
        }
    }
    return -1;
}

bool update_variable(char * action, char * value) {
    int index = search_variable(action);
    if (index > -1 && strcmp(value, variables.list[index]->value) != 0) {
        strcpy(variables.list[index]->value, value);
        variables.list[index]->value[strlen(value)] = '\0';
        return true;
    }
    return false;
}

// we should rename it... and maybe action should called state or event or ??
void watch_action(char * action) {
    printf("watch action: %s\n", action);

    if (search_variable(action) == -1) {
        printf("Variable not in list, insert it\n");
        size_t size = sizeof(struct Variable);
        struct Variable * variable = malloc(size);
        variable->name = malloc(strlen(action) * sizeof(char));
        strcpy(variable->name, action);
        strcpy(variable->value, "\0");
        // maybe we should put a default value

        variables.size += size;
        variables.list = (struct Variable **)realloc(variables.list, variables.size);
        variables.list[variables.count++] = variable;
        insert_topic(variable->name);
    }
}

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

bool is_valid = true;

void parser_init_triggers(char * line) 
{
    char type[2];
    char action[32];

    char * next = str_extract(line, 0, ' ', type) + 1;
    // printf("trigger type: %s\n", type);
    if (strcmp(type, "if") == 0) {
        // printf("next: %s\n", next);
        str_extract(next, 0, ' ', action);
        if (is_valid && search_trigger(action) == -1) {
            // printf("-------- action not in list, insert.\n");
            insert_trigger(action);
        }                        
        is_valid = false;
        watch_action(action);
    }
}

void parser_triggers(char * line) 
{
    char type[2];
    char action[32];
    char operator[2];

    if (is_valid) {
        char * next = str_extract(line, 0, ' ', type) + 1;
        printf("trigger type: %s\n", type);
        if (strcmp(type, "if") == 0) {
            printf("next: %s\n", next);
            next = str_extract(next, 0, ' ', action) + 1;
            int index = search_variable(action); 
            printf("Search for trigger: %s (%d)\n", action, index);            
            is_valid = index > -1;
            if (is_valid) {
                char * value = variables.list[index]->value;
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
                printf("Something strange happen, action not watched.\n");
            }
            printf("is valid %d\n", is_valid);
        }
        else if (strcmp(type, "do") == 0) {
            printf("do action: %s\n", next);
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
    triggers.count = 0;
    triggers.size = 0;

    variables.count = 0;
    variables.size = 0;

    parse_triggers_file(parser_init_triggers);
}

void trigger(char * action, char * value) {
    if (update_variable(action, value)) {
        printf("Variable changed, we should check for trigger.\n");
        parse_triggers_file(parser_triggers);
        // list_variables();        
    }
}
