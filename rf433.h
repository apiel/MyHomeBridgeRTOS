// #define RF433protocolCount 1

struct MinMax {
    uint16_t min;
    uint16_t max;
};

struct RF433protocol
{
    struct MinMax latch;
    struct MinMax latch2;
    struct MinMax low;
    struct MinMax hight;
    uint8_t len;
    bool only01or10;
};

void rf433_init(void);
void rf433_action(char * request);
void rf433_task(void *pvParameters);
