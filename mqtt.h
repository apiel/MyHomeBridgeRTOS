
#include <lwip/api.h>
#include <espressif/esp_common.h>

QueueHandle_t publish_queue;

struct MQTTMessage
{
    char topic[128];
    char msg[128];
 } mqttMessage;

void  mqtt_task(void *pvParameters);