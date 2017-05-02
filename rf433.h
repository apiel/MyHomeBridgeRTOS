// #define RF433protocolCount 1

struct RF433protocol
{
    uint16_t latch;
    uint16_t latch2;
    uint16_t low;
    uint16_t hight;
    uint8_t len;
    bool only01or10;
};

void rf433_init(void);
void rf433_action(char * request);
void rf433_task(void *pvParameters);
