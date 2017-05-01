#include <lwip/api.h>
#include <string.h>
#include <espressif/esp_common.h>

#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>

#include "wifi.h"
#include "config.h"
#include "mqtt.h"

static void  topic_received(mqtt_message_data_t *md)
{
    char * msg = malloc(md->message->payloadlen*sizeof(char));
    memcpy(msg, md->message->payload, md->message->payloadlen);
    msg[md->message->payloadlen] = '\0';
    printf("Msg received.\n%s\n%s\n\n", md->topic->lenstring.data, msg);    // , (char *)md->message->payload

    free(msg);
}

void  mqtt_task(void *pvParameters)
{
    int ret         = 0;
    struct mqtt_network network;
    mqtt_client_t client   = mqtt_client_default;
    char mqtt_client_id[20];
    uint8_t mqtt_buf[100];
    uint8_t mqtt_readbuf[100];
    mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;

    struct MQTTMessage *pxMessage;

    mqtt_network_new( &network );
    memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
    strcpy(mqtt_client_id, get_uid());

    char * topic = malloc(strlen(mqtt_client_id)+2*sizeof(char));
    strcpy(topic, mqtt_client_id);
    strcat(topic, "/#");    

    while(1) {
        // xSemaphoreTake(wifi_alive, portMAX_DELAY);
        printf("%s: started\n\r", __func__);
        printf("%s: (Re)connecting to MQTT server %s ... ",__func__,
               MQTT_HOST);
        ret = mqtt_network_connect(&network, MQTT_HOST, MQTT_PORT);
        if( ret ){
            printf("error: %d\n\r", ret);
            taskYIELD();
            continue;
        }
        printf("done\n\r");
        mqtt_client_new(&client, &network, 5000, mqtt_buf, 100,
                      mqtt_readbuf, 100);

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

        mqtt_subscribe(&client, topic, MQTT_QOS1, topic_received);
        // mqtt_subscribe(&client, "MHB_5CCF7F2B6E1E/lol", MQTT_QOS1, topic_received);
        xQueueReset(publish_queue);                

        while(1){

            while(xQueueReceive(publish_queue, &( pxMessage ), ( TickType_t ) 0) ==
                  pdTRUE){
                printf("got message to publish\r\n");
                mqtt_message_t message;
                message.payload = pxMessage->msg;
                message.payloadlen = strlen(pxMessage->msg);
                message.dup = 0;
                message.qos = MQTT_QOS1;
                message.retained = 1;
                ret = mqtt_publish(&client, pxMessage->topic, &message);
                if (ret != MQTT_SUCCESS ){
                    printf("error while publishing message: %d\n", ret );
                    break;
                }
            }

            ret = mqtt_yield(&client, 1000);
            if (ret == MQTT_DISCONNECTED)
                break;
        }
        printf("Connection dropped, request restart\n\r");
        mqtt_network_disconnect(&network);
        taskYIELD();
    }
    free(topic);
}

