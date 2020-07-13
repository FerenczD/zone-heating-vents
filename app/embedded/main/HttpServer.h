#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string.h>
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_err.h"
#include "tcpip_adapter.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "tcpip_adapter.h"

static EventGroupHandle_t s_connect_event_group;
static ip4_addr_t s_ip_addr;
static const char *s_connection_name;
static const char *TAG = "HTTP_SERVER";

#define GOT_IPV4_BIT BIT(0)
#define GOT_IPV6_BIT BIT(1)
#define CONNECTED_BITS (GOT_IPV4_BIT)

class HttpServer
{
    private:
        ventCmd_t       ventCmd;
        ventStatus_t    ventStatus;
    public:
        void onGotIP(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
        esp_err_t connect();
        esp_err_t disconnect();
        void onWifiDisconnect();
        void start();
        void stop();
};

#endif 