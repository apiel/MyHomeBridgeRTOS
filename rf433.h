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
    bool manchester_code;
};

void rf433_init(void);
void rf433_action(char * request);
void rf433_task(void *pvParameters);
