// #define RF433protocolCount 1

struct MinMax {
    uint16_t min;
    uint16_t max;
};

struct RF433protocol2
{
    struct MinMax latch;
    struct MinMax latch2;
    struct MinMax low;
    struct MinMax hight;
    uint8_t len;
    bool only01or10;
};

struct Pulse {
    uint16_t length;
    uint16_t tolerance;
};

struct RF433protocol
{
    struct Pulse latch;
    struct Pulse latch2;
    struct Pulse low;
    struct Pulse high;
    uint8_t zero;
    uint8_t one;
    uint8_t len; // not really necessary
    bool manchester_code;
};

void rf433_init(void);
void rf433_action(char * request);
void rf433_task(void *pvParameters);
