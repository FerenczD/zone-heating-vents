#ifndef HTTPPOST_H_
#define HTTPPOST_H_

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
#include "driver/esp_http_client.h"


#ifdef __cplusplus
extern "C" {
#endif

#define WEB_URL "http://192.168.1.8"
#define SERVER_URL "http://192.168.1.8/venti/serverRequests.php"

#define ERROR_MESSAGE   "ERROR"

static const char *POST_TAG = "HTTP_POST";

        char * request(esp_err_t(* eventHandler)(esp_http_client_event_t *), char * query);
     
        
#ifdef __cplusplus
} // extern "C"
#endif

#endif 