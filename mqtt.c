#include <lwip/api.h>
#include <string.h>
#include <espressif/esp_common.h>

#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>

#include "wifi.h"
#include "config.h"
#include "mqtt.h"
#include "action.h"
#include "trigger.h"

mqtt_client_t client = mqtt_client_default;
char mqtt_client_id[20];

struct Topics
{
    char ** list;
    size_t count;
    size_t size;
} topics;

bool is_topic_already_inserted(char * topic) {
    int index = 0;
    for (; index < topics.count; index++) {
        char * topicCmp = topics.list[index];
        size_t len = strlen(topicCmp);
        if (topicCmp[len-1] == '+') {
            len--;
            topicCmp = malloc(len * sizeof(char));
            memcpy(topicCmp, topics.list[index], len);
            topicCmp[len] = '\0';

            char * found = strstr(topic, topicCmp);
            if (found && (topic - found) == 0) {
                // printf("Topic to compare A: %s %s %d\n", topicCmp, found, topic - found);            
                return true;
            }
        } 
        else {
            printf("Topic to compare B: %s\n", topicCmp);
        }
    }

    return false;
}

void insert_topic(char * topic) {
    if (!is_topic_already_inserted(topic)) {
        printf("Insert topic: %s\n", topic);
        size_t size = strlen(topic) * sizeof(char);
        char * _topic = malloc(size);
        strcpy(_topic, topic);

        topics.size += size;
        topics.list = (char **)realloc(topics.list, topics.size);
        topics.list[topics.count++] = _topic;
    }
}

void mqtt_init() 
{
    topics.count = 0;
    topics.size = 0;

    memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
    strcpy(mqtt_client_id, get_uid());    

    char * topic;
    topic = malloc(strlen(MHB_USER)+strlen(MHB_ZONE)+strlen(mqtt_client_id)+4*sizeof(char));
    strcpy(topic, MHB_USER);
    strcat(topic, "/");
    strcat(topic, MHB_ZONE);    
    strcat(topic, "/");
    strcat(topic, mqtt_client_id);
    strcat(topic, "/+");
    insert_topic(topic);
    free(topic);

    topic = malloc(strlen(MHB_USER)+strlen(MHB_ZONE)+5*sizeof(char));
    strcpy(topic, MHB_USER);
    strcat(topic, "/");
    strcat(topic, MHB_ZONE);
    strcat(topic, "/-/+");
    insert_topic(topic);
    free(topic);

    topic = malloc(strlen(MHB_USER)+6*sizeof(char));
    strcpy(topic, MHB_USER);
    strcat(topic, "/-/-/+");
    insert_topic(topic);
    free(topic);
}

static void  topic_received(mqtt_message_data_t *md)
{
    char * msg = malloc(md->message->payloadlen*sizeof(char));
    memcpy(msg, md->message->payload, md->message->payloadlen);
    msg[md->message->payloadlen] = '\0';
    
    char * topic = strrchr(md->topic->lenstring.data, '/') + 1;
    printf("Msg received on topic: %s\nMsg: %s\n\n", topic, msg);    // , (char *)md->message->payload

    // should we provide the whole topic to reducer??
    reducer(topic, msg);
    // but actually we should be able to trigger even if not connected!!
    trigger(md->topic->lenstring.data, msg);

    free(msg);
}

void subscribe_to_topics() {
    int index = 0;
    for (; index < topics.count; index++) {
        printf("Subscribe to topic: %s\n", topics.list[index]);
        mqtt_subscribe(&client, topics.list[index], MQTT_QOS1, topic_received);
    }
}

void  mqtt_task(void *pvParameters)
{
    int ret = 0;
    struct mqtt_network network;
    uint8_t mqtt_buf[100];
    uint8_t mqtt_readbuf[100];
    mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;

    struct MQTTMessage *pxMessage;

    mqtt_network_new( &network );

    while(1) {
        // xSemaphoreTake(wifi_alive, portMAX_DELAY);
        printf("%s: started\n\r", __func__);
        printf("%s: (Re)connecting to MQTT server %s ... ",__func__,
               MQTT_HOST);
        ret = mqtt_network_connect(&network, MQTT_HOST, MQTT_PORT);
        if( ret ){
            printf("error: %d\n\r", ret);
            taskYIELD();
            vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 sec
            continue;
        }
        printf("done\n\r");
        mqtt_client_new(&client, &network, 5000, 
                        mqtt_buf, 200,
                        mqtt_readbuf, 200);

        data.willFlag       = 0;
        data.MQTTVersion    = 3;
        data.clientID.cstring   = mqtt_client_id;
        data.username.cstring   = MQTT_USER;
        data.password.cstring   = MQTT_PASS;
        data.keepAliveInterval  = 10;
        data.cleansession   = 0;
        printf("Send MQTT connect ... ");
        ret = mqtt_connect(&client, &data);
        if(ret){
            printf("error: %d\n\r", ret);
            mqtt_network_disconnect(&network);
            taskYIELD();
            continue;
        }
        printf("done\r\n");

        subscribe_to_topics();
        xQueueReset(publish_queue);                

        while(1){

            while(xQueueReceive(publish_queue, &( pxMessage ), ( TickType_t ) 0) ==
                  pdTRUE){
                mqtt_message_t message;
                message.payload = pxMessage->msg;
                message.payloadlen = strlen(pxMessage->msg);
                message.dup = 0;
                message.qos = MQTT_QOS1;
                message.retained = 1;

                char * topic = pxMessage->topic;
                // char * topic = malloc((strlen(pxMessage->topic) + strlen(topics.list[0])) * sizeof(char));
                // strcpy(topic, topics.list[0]);
                // topic[strlen(topics.list[0]) - 1] = '\0';
                // strcat(topic, pxMessage->topic);

                printf("got message to publish %s: %s\r\n", topic, pxMessage->msg);
                ret = mqtt_publish(&client, topic, &message);
                if (ret != MQTT_SUCCESS ){
                    printf("error while publishing message: %d\n", ret );
                    break;
                }
                // no need to put it here since we received on subscribe
                // trigger(topic, pxMessage->msg);

                // free(topic);
            }

            ret = mqtt_yield(&client, 1000);
            if (ret == MQTT_DISCONNECTED)
                break;
        }
        printf("Connection dropped, request restart\n\r");
        mqtt_network_disconnect(&network);
        taskYIELD();
    }
}

