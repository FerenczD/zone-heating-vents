#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

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

#ifdef __cplusplus
extern "C" {
#endif
static EventGroupHandle_t s_connect_event_group;
static ip4_addr_t s_ip_addr;
static const char *s_connection_name;
static const char *TAG = "HTTP_SERVER";

#define GOT_IPV4_BIT BIT(0)
#define GOT_IPV6_BIT BIT(1)
#define CONNECTED_BITS (GOT_IPV4_BIT)
#define EXAMPLE_INTERFACE TCPIP_ADAPTER_IF_STA

static void onGotIP(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
esp_err_t httpServerConnect();
esp_err_t httpServerDisonnect();
static void onWifiDisconnect(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
static void start();
static void stop();

#ifdef __cplusplus
} // extern "C"
#endif

#endif 