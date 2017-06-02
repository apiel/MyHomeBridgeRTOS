#include <espressif/esp_common.h>
#include <esp8266.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <spiffs.h>
#include <esp_spiffs.h>

#include "rf433.h"

// this could be call from an internal queue
void reducer(char * action, char * params)
{
    if (strcmp(action, "rf433") == 0) {
        rf433_action(params);
    }
    else if (strcmp(action, "#") == 0) {
        printf("-> %s\n", params);
    }    
    else {
        printf("This action is not supported.\n");
    }
}

static void example_read_file_spiffs2()
{
    const int buf_size = 0xFF;
    uint8_t buf[buf_size];

    spiffs_file fd = SPIFFS_open(&fs, "actions_off", SPIFFS_RDONLY, 0);
    if (fd < 0) {
        printf("\n\n\n---------spiff2 Error opening file\n\n\n");
        return;
    }

    int read_bytes = SPIFFS_read(&fs, fd, buf, buf_size);
    printf("\n\n-------spiff2 Read %d bytes\n\n\n", read_bytes);

    buf[read_bytes] = '\0';    // zero terminate string
    printf("\n\n-------------- Data spiff2: %s\n\n\n", buf);

    SPIFFS_close(&fs, fd);
}

static void example_read_file_spiffs()
{
    const int buf_size = 0xFF;
    uint8_t buf[buf_size];

    spiffs_file fd = SPIFFS_open(&fs, "other.txt", SPIFFS_RDONLY, 0);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }

    int read_bytes = SPIFFS_read(&fs, fd, buf, buf_size);
    printf("Read %d bytes\n", read_bytes);

    buf[read_bytes] = '\0';    // zero terminate string
    printf("Data: %s\n", buf);

    SPIFFS_close(&fs, fd);
}

static void example_write_file()
{
    uint8_t buf[] = "Example data, written by ESP8266";

    int fd = open("other.txt", O_WRONLY|O_CREAT, 0);
    if (fd < 0) {
        printf("Error opening file\n");
        return;
    }

    int written = write(fd, buf, sizeof(buf));
    printf("Written %d bytes\n", written);

    close(fd);
}

static void example_fs_info()
{
    uint32_t total, used;
    SPIFFS_info(&fs, &total, &used);
    printf("Total: %d bytes, used: %d bytes", total, used);
}





void test()
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

    example_fs_info();
    example_read_file_spiffs2();
    example_write_file();
    example_read_file_spiffs();
    example_fs_info();    
}
