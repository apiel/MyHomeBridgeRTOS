
#include <lwip/api.h>
#include <espressif/esp_common.h>

QueueHandle_t publish_queue;

struct MQTTMessage
{
    char topic[128];
    char msg[256];
 } mqttMessage;

void  mqtt_task(void *pvParameters);
void mqtt_init();
void insert_topic(char * topic);
