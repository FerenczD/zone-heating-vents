
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "driver/gpio.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "uart_esp.h"
#include "cJSON.h"
#include "esp_tls.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512
#define GPIO_OUTPUT_IO_0    18
#define GPIO_OUTPUT_IO_1    19
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_0     4
#define GPIO_INPUT_IO_1     5
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0
#define GET_TEMP_REQUEST_INTERVAL_MICRO_S 5000000

#define WEB_PORT 80
#define WEB_URL "http://192.168.0.13"
#define SERVER_URL "http://192.168.0.13/venti/serverRequests.php"

bool mWifiSet = false;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
// static EventGroupHandle_t s_wifi_event_group;

// static const int CONNECTED_BIT = BIT0;
// static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "VENTI";

vent_cmd_t mVentCmd;                                    /* Instance of vent command structure */
uint8_t* mVentCmdPtr = (uint8_t*)&mVentCmd;             /* Pointer that stores vent command */

vent_status_cmd_t mVentStatus;                          /* Instance of vent status response structure */
uint8_t* mVentStatusPtr = (uint8_t*)&mVentStatus;       /* Pointer that stores vent status */


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}

static float parseString(char* buffer){
    float temp = atof(buffer);

    return temp;
}

static uint8_t commandSelect(float temp){

    uint8_t ret;
    if(temp < 25.0){
        ret= 0x01;
    }else{
        ret= 0x02;
    }

    ESP_LOGI(TAG, "Motor command: %x",ret);
    return ret;
}

static void postTemperatureForMac(uint16_t* temp,uint8_t mac[6])
{
    ESP_LOGI(TAG, "Updating server with temperature...");
    int tempInt = *temp;
    uint8_t tempChar[6];
    char query[255] = {0};

    sprintf(query,"action=thermSendTemp&temp=%d&mac=%02x%02x%02x%02x%02x%02x", tempInt, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    esp_http_client_config_t config = {
        .url = WEB_URL,
        .event_handler = _http_event_handler,
    };

    ESP_LOGI(TAG, "Post request payload: %s",query);
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_set_url(client, SERVER_URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, query, strlen(query));
    err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        char * buffer = (char *) malloc(esp_http_client_get_content_length(client));
        esp_http_client_read(client,buffer,esp_http_client_get_content_length(client));
        
        ESP_LOGI(TAG, "Server responed with: %s",buffer);
        ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, sizeof(buffer), ESP_LOG_INFO); 
        free(buffer);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
  
    esp_http_client_cleanup(client);
    gpio_set_level(GPIO_OUTPUT_IO_0,0);
}


static void getTemperatureForMac()
{
  ESP_LOGI(TAG, "Requesting server for temperature...");
    char query[255] = {0};

    sprintf(query,"action=thermRequestTemp&mac=aa170758cdf4");

    esp_http_client_config_t config = {
        .url = WEB_URL,
        .event_handler = _http_event_handler,
    };

    ESP_LOGI(TAG, "Post request payload: %s",query);
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_set_url(client, SERVER_URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, query, strlen(query));
    err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    } else {

        char * buffer = (char *) malloc(esp_http_client_get_content_length(client));
        esp_http_client_read(client,buffer,esp_http_client_get_content_length(client));
        
        ESP_LOGI(TAG, "Server responded with: %s",buffer);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client)); 

        float temp = parseString(buffer);
        uint8_t command = commandSelect(temp);
        uint8_t testUUID[CMD_UUID_LEN] = { 0x12 , 0x34 };
        uint8_t testBleAddr[BLE_ADDR_LEN] = { 0xAA , 0x17, 0x07, 0x58, 0xCD, 0xF4 };

        memcpy( &mVentCmd.cmd_uuid, &testUUID, CMD_UUID_LEN);
        memcpy(&mVentCmd.ble_addr, &testBleAddr, BLE_ADDR_LEN);
        mVentCmd.cmd = command;

        memcpy(mVentCmdPtr, &mVentCmd, sizeof(vent_cmd_t));

        ESP_LOG_BUFFER_HEXDUMP("UART Transmit", mVentCmdPtr, sizeof(vent_cmd_t), ESP_LOG_INFO);
        gpio_set_level(GPIO_OUTPUT_IO_1,1);
        uartSendData((char*)mVentCmdPtr);
        free(buffer);

    }
    esp_http_client_cleanup(client);
    gpio_set_level(GPIO_OUTPUT_IO_1,0);
    vTaskDelete(NULL);
}


static void tx_task()
{

    uint8_t testUUID[CMD_UUID_LEN] = { 0x12 , 0x34 };
    uint8_t testBleAddr[BLE_ADDR_LEN] = { 0xAA , 0x17, 0x07, 0x58, 0xCD, 0xF4 };
    uint8_t testCmd = 0x1;

    memcpy( &mVentCmd.cmd_uuid, &testUUID, CMD_UUID_LEN);
    memcpy(&mVentCmd.ble_addr, &testBleAddr, BLE_ADDR_LEN);
    mVentCmd.cmd = testCmd;

    memcpy(mVentCmdPtr, &mVentCmd, sizeof(vent_cmd_t));

    // ESP_LOG_BUFFER_HEXDUMP("TEST", mVentCmdPtr, sizeof(vent_cmd_t), ESP_LOG_INFO);  

    while (1) {
        
        uartSendData((char*)mVentCmdPtr);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
          
    }
}

static void rx_task()
{

    uint8_t* data = (uint8_t*)calloc(sizeof(vent_status_cmd_t), sizeof(uint8_t));
    vent_status_cmd_t receivedData;   

    while (1) {
        const int rxBytes = uartReadData(data, (portTickType)portMAX_DELAY); /* Blocking function add timeout */
        memcpy(&receivedData,data,sizeof(vent_status_cmd_t));

        // ESP_LOGI(TAG, "Received message through UART.\n\n");
        gpio_set_level(GPIO_OUTPUT_IO_0,1);
        ESP_LOG_BUFFER_HEXDUMP("Received Message:", data, sizeof(vent_status_cmd_t), ESP_LOG_INFO);
        postTemperatureForMac((uint16_t*)receivedData.vent_temp,receivedData.ble_addr);

            uart_flush_input(UART_NUM_X);
    }
    free(data);
}

    static void periodic_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Timer called... creating task.");
    xTaskCreate(getTemperatureForMac, "getTemperatureForMac", 8192, NULL, configMAX_PRIORITIES, NULL);
    

}

// static void wifi_config_task(void * parm);
// static void event_handler(void* arg, esp_event_base_t event_base,
//                                 int32_t event_id, void* event_data)
// {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         xTaskCreate(wifi_config_task, "wifi_config_task", 4096, NULL, 3, NULL);
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         esp_wifi_connect();
//         xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
//     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
//     } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
//         ESP_LOGI(TAG, "WiFi scan done");
//     } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
//         ESP_LOGI(TAG, "Found channel");
//     } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
//         ESP_LOGI(TAG, "Got SSID and password");

//         smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
//         wifi_config_t wifi_config;
//         uint8_t ssid[33] = { 0 };
//         uint8_t password[65] = { 0 };
//         bzero(&wifi_config, sizeof(wifi_config_t));
//         memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
//         memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
//         wifi_config.sta.bssid_set = evt->bssid_set;
//         if (wifi_config.sta.bssid_set == true) {
//             memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
//         } 

//         memcpy(ssid, evt->ssid, sizeof(evt->ssid));
//         memcpy(password, evt->password, sizeof(evt->password));
//         ESP_LOGI(TAG, "Connected to SSID:%s", ssid);
//         ESP_LOGI(TAG, "Network password:%s", password);


//         ESP_ERROR_CHECK( esp_wifi_disconnect() );
//         ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
//         ESP_ERROR_CHECK( esp_wifi_connect() );

    
//     } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
//         xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
//     }
// }

// esp_err_t clientEventHandler(esp_http_client_event_t *evt) 
// {
// switch(evt->event_id)
// {
//     case HTTP_EVENT_ON_DATA:
//     printf("%.*s\n",evt->data_len, (char *)evt->data);
//     default:
//     break;
// }
// return ESP_OK;
// }

void configure_http_server() 
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
}


// static void initialise_wifi(void)
// {
//     tcpip_adapter_init();
//     s_wifi_event_group = xEventGroupCreate();
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

//     ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
//     ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
//     ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

//     ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
//     ESP_ERROR_CHECK( esp_wifi_start() );

//      ESP_LOGI(TAG, "Initialization Done");

   

// }

// static void wifi_config_task(void * parm)
// {
//      ESP_LOGI(TAG, "Do we even get here");
//     EventBits_t uxBits;
//     ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
//      ESP_LOGI(TAG, "How about here");
//     smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
//     ESP_LOGI(TAG, "here");
//     ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
//     ESP_LOGI(TAG, "just before while");
//     while (1) {
//         ESP_LOGI(TAG, "while");
//         uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
//         if(uxBits & CONNECTED_BIT) {
//             ESP_LOGI(TAG, "Device is connected to the access point.");
//         }
//         if(uxBits & ESPTOUCH_DONE_BIT) {
//             ESP_LOGI(TAG, "WiFi configuration task has ended.");
//             esp_smartconfig_stop();
//              mWifiSet = true;
//             vTaskDelete(NULL);
            
             
//         }
//         else {
//             ESP_LOGI(TAG, "Some Error");
//         }
//     }
// }

static void timer_init()
{
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "GET TEMP TIMER"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, GET_TEMP_REQUEST_INTERVAL_MICRO_S));
}

void app_main()
{
ESP_LOGI(TAG, "Initializing NVS flash:");
    esp_err_t ret = nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ESP_LOGI(TAG, "NVS flash was erased.");
      ret = nvs_flash_init();
      ESP_LOGI(TAG, "NVS flash was re-initalized.");
    }
    ESP_ERROR_CHECK(ret);
    // ESP_LOGI(TAG, "Initalizing WiFi for Smart Config Pairing");
    // initialise_wifi();
    // while(mWifiSet != true);
    ESP_LOGI(TAG, "Initializing UART...");
    uartInit();

     gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
 

    ESP_LOGI(TAG, "Configuring Network...");
    configure_http_server();
    ESP_LOGI(TAG, "Setting up Timer...");
    timer_init();
    ESP_LOGI(TAG, "Begin Receiving from UART");
    xTaskCreate(rx_task, "uart_rx_task", 8192, NULL, configMAX_PRIORITIES, NULL);

}

