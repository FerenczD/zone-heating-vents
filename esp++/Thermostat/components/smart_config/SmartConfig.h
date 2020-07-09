#ifndef SMARTCONFIG_H_
#define SMARTCONFIG_H_

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "../esp_supplicant/esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"

#ifdef __cplusplus
extern "C" {
#endif
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *SMART_CONFIG_TAG = "SMART CONFIG";
int smartConfigEnabled;

static EventGroupHandle_t s_wifi_event_group;

static void smartconfig_example_task(void * parm);
void initialise_wifi(void);
static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data);

                                

#ifdef __cplusplus
} // extern "C"
#endif

#endif 