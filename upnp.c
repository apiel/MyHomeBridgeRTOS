
#include <espressif/esp_common.h>
// #include <esp8266.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

// #define LWIP_IGMP 1

#include <lwip/udp.h>
#include <lwip/igmp.h>
#include <lwip/dns.h>

#define DNS_MULTICAST_ADDRESS   "239.255.255.250"

static struct udp_pcb* pcb = NULL;
static ip_addr_t       gMulticastAddr;

typedef struct rsrc {
    struct rsrc*    rNext;
    u16_t     rType;
    u32_t    rTTL;
    u16_t    rKeySize;
    u16_t    rDataSize;
    char    rData[8];
} rsrc;

static void update_ipaddr(struct ip_info* ipInfo)
{
    rsrc* rp = NULL;
    while (rp != NULL) {
        if (rp->rType==DNS_RRTYPE_A) {
            #ifdef qDebugLog
                printf("Updating A record for '%s' to %d.%d.%d.%d\n", rp->rData,
                    ip4_addr1(&ipInfo->ip), ip4_addr2(&ipInfo->ip), ip4_addr3(&ipInfo->ip), ip4_addr4(&ipInfo->ip));
            #endif
            memcpy(&rp->rData[rp->rKeySize], &ipInfo->ip, sizeof(ip_addr_t));
        }
        rp = rp->rNext;
    }
}

static void recvFn(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
    printf(">>> recv..\n");
}

bool ok = false;

void yo() 
{
    struct ip_info ipInfo;

    printf("********** Try to load upnp\n\r");

    ipaddr_aton(DNS_MULTICAST_ADDRESS, &gMulticastAddr);

    update_ipaddr(&ipInfo);

    pcb = udp_new();
    if (!pcb) {
        printf(">>> mDNS_start: udp_new failed\n");
        return;
    }
    
    if (igmp_joingroup(&ipInfo.ip, &gMulticastAddr) != ERR_OK) {
        printf(">>> mDNS_start: igmp_join failed\n");
        return;
    }

    if (udp_bind(pcb, IP_ADDR_ANY, 1900) != ERR_OK) {
        printf(">>> mDNS_start: udp_bind failed\n");
        return;
    }
    
    udp_recv(pcb, recvFn, NULL);

    ok = true;
}

void yo2()
{
    printf("********** Try to load upnp2\n\r");

    ip_addr_t ifaddr;
    // ifaddr.addr = (uint32_t) interfaceAddr;
    IP4_ADDR(&ifaddr, 192, 168, 0, 23);
    ip_addr_t multicast_addr;
    // multicast_addr.addr = (uint32_t) multicast;
    IP4_ADDR(&ifaddr, 239, 255, 255, 250);

    err_t err;
    if ((err = igmp_joingroup(&ifaddr, &multicast_addr)) != ERR_OK) {
        printf(">>> mDNS_start: igmp_joingroup failed %d\n", err);
        return;
    }

    pcb = udp_new();
    if (!pcb) {
        printf(">>> mDNS_start: udp_new failed\n");
        return;
    }

    udp_recv(pcb, recvFn, NULL);
    if ((err = udp_bind(pcb, IP_ADDR_ANY, 1900)) != ERR_OK) {
        printf(">>> mDNS_start: udp_bind failed %d\n", err);
        return;
    }

    ok = true;
}

void upnp_task(void *pvParameters)
{
    printf("Upnp task\n\r");

    while (1) { 
        if (!ok) yo2();
        vTaskDelay(1000);
    }   
}
