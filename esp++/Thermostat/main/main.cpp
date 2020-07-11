
/* *********** Standard Libraries *********** */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

/* *********** Free RTOS Libraries *********** */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

/* *********** ESP Libraries *********** */
#include "esp_tls.h"
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
#include "uart_esp.h"
#include "esp_tls.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "esp_smartconfig.h"
#include "driver/gpio.h"

/* *********** Components *********** */
#include "uart_esp.h"
#include "HttpServer.h"
#include "HttpPost.h"
// #include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"
#include "SmartConfig.h"
#include "vent_class.h"

#define GPIO_INPUT_IO_0 4
#define GPIO_INPUT_IO_1 5
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

/* Globals */
static char tag[]="Venti";

/* Data structures */
EspUart uartIntance_g;
std::vector<Vent*> myHomeVents;  /* Vents paired and known by the sytem. Note: This hould be sinchronized with erver at some point and store it locally in flash */
                                 /* Various tasks will have access so do semaphores in the future */
/* Flags */
int wifiIsConnected = 0;
static int pairingEnabled = 0;

/* Handles */
static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle pairingEvtQueue = NULL;

/* Prototypes */
static void smartConfig();
esp_err_t _http_event_handler(esp_http_client_event_t *evt);

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

extern "C"{
    void app_main();
}

static void tx_task(void *pvParameter){
    uint8_t testBleAddr[BLE_ADDR_LEN] = { 0xAA , 0x17, 0x07, 0x58, 0xCD, 0xF4 };
    uint8_t testCmd = 0x1;

    uartIntance_g.uartSetWriteData(testCmd, testBleAddr);

    while(1){
        uartIntance_g.uartSendData();
        vTaskDelay(2000 / portTICK_PERIOD_MS); 
    }
}
static void update_data_task(void *pvParameters){

    ventStatus_t receivedData = *(ventStatus_t*)pvParameters;

    /* Update local data structure */
    Vent* localVent = findVentByMacBytes(receivedData.bleAddr, myHomeVents);

    localVent->setStatus(receivedData.ventStatus);
    localVent->setCurrentTemperature((uint16_t*)receivedData.ventTemp);

    /* Update server */
    char query[255] = {0};
    sprintf(query, "action=updateExistingVent&mac=%s&temp=%d&status=%d", localVent->getMacAddrStr().c_str(), 
                                                                         *(uint16_t*)receivedData.ventTemp,
                                                                         receivedData.ventStatus);

    ESP_LOGI("UPDATE", "%s", query);

    //char* response = request(_http_event_handler, query);

    /* Check http response for errors */

    /* Run the algorithm task. Note: Important to do everytime you receive new data from vents */


    free(response);
    vTaskDelete(NULL);
}

static void rx_task(void *pvParameter)
{
    while(1){
        int rxBytes = uartIntance_g.uartRead((portTickType)portMAX_DELAY);

        if(rxBytes > 0){
            ventStatus_t receivedData = uartIntance_g.uartGetStatus(); 
            ESP_LOG_BUFFER_HEXDUMP(UART_READ_LOG_NAME, &receivedData, rxBytes, ESP_LOG_INFO);

            if(pairingEnabled == 1){
                uint32_t pairingCompleted = 1;

                xQueueSend(pairingEvtQueue, &pairingCompleted, ( TickType_t ) 0);  /* Dont block if queue is already full */
            }else{
                /* This should be a normal operation read */
                ventStatus_t receivedDataCpy = receivedData;
                xTaskCreate(update_data_task, "update_data_task", 8192, &receivedDataCpy, configMAX_PRIORITIES - 1, NULL);
                
            }
        }    
    }

    vTaskDelete(NULL);
}

static void get_server_data_task(void* pvParameters){
    const TickType_t callFrequency = 10000 / portTICK_PERIOD_MS;    /* 10 seconds */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    char query[255] = {0};
    sprintf(query, "action=getVentData");  /* Get all data */

    for(;;){
        char* response = request(_http_event_handler, query);

        /* Parse response with cJSON and get ids and setTemperatures */
        float* setTempArr;
        int* idArr;

        /* Update local data structure with new setTemperature */
        updateHomeVents(myHomeVents, idArr, setTempArr);

        free(response);
        vTaskDelayUntil(&xLastWakeTime, callFrequency);
        xLastWakeTime = xTaskGetTickCount();
    }

    vTaskDelete(NULL);
}

static void uart_pairing_task(void *pvParameter){

    uint32_t pairingCompleted = 0;

    ESP_LOGI("PAIRING", "Pairing task entered");

    pairingEnabled = 1;

    uartIntance_g.uartSendPairingModeRequest(); 

    xQueueReceive(pairingEvtQueue, &pairingCompleted, portMAX_DELAY);

    if(pairingCompleted == 1){
        
        ESP_LOGI("PAIRING", "Pairing done. This is the MAC address of vent");

        ventStatus_t receivedData = uartIntance_g.uartGetStatus(); 
        ESP_LOG_BUFFER_HEXDUMP(UART_READ_LOG_NAME, &receivedData.bleAddr, BLE_ADDR_LEN, ESP_LOG_INFO);

        /* Here send the information to other task waiting for pairing information or do post request directly */
        char query[255] = {0};
        sprintf(query, "action=addNewVent&mac=%02x%02x%02x%02x%02x%02x", receivedData.bleAddr[0],
                                                                         receivedData.bleAddr[1],
                                                                         receivedData.bleAddr[2],
                                                                         receivedData.bleAddr[3],
                                                                         receivedData.bleAddr[4],
                                                                         receivedData.bleAddr[5]);

        ESP_LOG_BUFFER_HEXDUMP("PAIRING_TASK", query, strlen(query), ESP_LOG_INFO);

        char* response = request(_http_event_handler, query);           /********** This response should give you the vent of the id *************/

        /* Check if response is ok before storing in data structure */
            /* if ok Create a new intance of the vent class store available data and store in vector */
            // Vent* newVent;
            // newVent->setCurrentTemperature((uint16_t*)receivedData.ventTemp);
            // newVent->setStatus(receivedData.ventStatus);
            // newVent->setMacAddr(receivedData.bleAddr);
            //newVent.setId(response);

            //myHomeVents.push_back(newVent);

            /* Else error? */
        free(response);
        
    }else{
        ESP_LOGI("PAIRING", "Error pairing. PairingComplete = 0");
    }

    pairingEnabled = 0;
    ESP_LOGI("PAIRING", "Pairing task completed. Killing task");

    vTaskDelete(NULL);

}

static void flag_lookup_task(void *pvParameter){
    const TickType_t callFrequency = 10000 / portTICK_PERIOD_MS;    /* 10 seconds */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;){

        char * response = request(_http_event_handler, "action=checkIfVentsNeedPairing");
        ESP_LOGI("FLAG_TASK", "Server responed with: %s",response);
 
        if(*response - '0' == 1){
            ESP_LOGI("FLAG_TASK", "Pairing required starting pairing task");
            
            pairingEnabled = 1;
            xTaskCreate(uart_pairing_task, "uart_pairing_task", 8192, NULL, configMAX_PRIORITIES-1, NULL);
            while(pairingEnabled == 1);


        }else{
            ESP_LOGI("FLAG_TASK", "Pairing not required yet");
        }
        free(response);
        vTaskDelayUntil(&xLastWakeTime, callFrequency);

        xLastWakeTime = xTaskGetTickCount();
    }


}

static void smartConfig() {
    uint32_t io_num;

    smartConfigEnabled = 1;

    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
    printf("testsetset");
        printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level((gpio_num_t) io_num));
        initialise_wifi();
    }
    while(smartConfigEnabled != 0) {
        
    }
    // smartConfigEnabled = 0;
    wifiIsConnected = 1;

    // ESP_ERROR_CHECK(httpServerConnect());
    char * response = request(_http_event_handler, "action=confirmSmartConfigCompleted");
    ESP_LOGD(TAG, "Server responed with: %s",response);
    free(response);
    vTaskDelete(NULL);
}


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(POST_TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(POST_TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(POST_TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(POST_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(POST_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(POST_TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(POST_TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t) evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(POST_TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(POST_TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}

/* Only to test functions. Called on main */
static void tester(){

    uint8_t temp[2] = {0xAB, 0x09};
    uint8_t status = 0x01;
    uint8_t mac[6] = {0xaa, 0x11, 0x22, 0x33,0x44,0x55};

    Vent* newVent = new Vent();
    newVent->setCurrentTemperature((uint16_t*)temp);
    newVent->setStatus(status);
    newVent->setMacAddr(mac);   
    newVent->setId(1);
    myHomeVents.push_back(newVent);

    uint8_t temp2[2] = {0xc9, 0x0A};
    uint8_t status2 = 0x02;
    uint8_t mac2[6] = {0xcc, 0x77, 0x88, 0x99,0xaa,0xbb}; 
    Vent* newVent2 = new Vent();;
    newVent2->setCurrentTemperature((uint16_t*)temp2);
    newVent2->setStatus(status2);
    newVent2->setMacAddr(mac2);  
    newVent2->setId(2);
    myHomeVents.push_back(newVent2);

    /* Iterating through the vector */
    for(auto it = std::begin(myHomeVents); it != std::end(myHomeVents); ++it) {
        Vent* test = *it;
        ESP_LOGI("TEST", "Status: %d", test->getStatus());
        ESP_LOGI("TEST", "Temp: %.2f", test->getCurrentTemperature());
        ESP_LOGI("TEST", "mac: %s", test->getMacAddrStr().c_str());
    }

    /* Searching */
    Vent* found = findVentById(1, myHomeVents);
    ESP_LOGI("TEST", "Temp: %.2f", found->getCurrentTemperature());
    Vent* found2 = findVentByMacBytes(mac, myHomeVents);

    /*Edit values */
    uint8_t temp3 [2] = {0x11, 0x07};
    // found->setCurrentTemperature((uint16_t*)temp3);
    // for(auto it = std::begin(myHomeVents); it != std::end(myHomeVents); ++it) {
    //     Vent* test = *it;
    //     ESP_LOGI("TEST", "Status: %d", test->getStatus());
    //     ESP_LOGI("TEST", "Temp: %.2f", test->getCurrentTemperature());
    //     ESP_LOGI("TEST", "mac: %s", test->getMacAddrStr().c_str());
    // }

    ventStatus_t receivedData;
    receivedData.ventStatus = 0x01;
    memcpy(&receivedData.bleAddr,mac,6);
    memcpy(&receivedData.ventTemp , &temp3,2);

    xTaskCreate(update_data_task, "update_data_task", 8192, &receivedData, configMAX_PRIORITIES - 1, NULL);
    for(;;);
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    // tcpip_adapter_init();
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    // ESP_ERROR_CHECK(httpServerConnect());
    
    gpio_config_t io_conf;
    //  //interrupt of rising edge
    io_conf.intr_type = (gpio_int_type_t) GPIO_PIN_INTR_POSEDGE;
    // //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    // //enable pull-up mode
    io_conf.pull_down_en = (gpio_pulldown_t) 1;
    gpio_config(&io_conf);

    // //change gpio intrrupt type for one pin
    gpio_set_intr_type((gpio_num_t) GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    // //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Create Queue to handle pairing completed information
    pairingEvtQueue = xQueueCreate(3, sizeof(uint32_t));
    // //start gpio task
    
    xTaskCreate((TaskFunction_t ) smartConfig, "smartConfig", 16383, NULL, 10, NULL);

    if(wifiIsConnected == 1) {
        ESP_ERROR_CHECK(httpServerConnect());
    }
    // // xTaskCreate((TaskFunction_t ) smartConfig, "smartConfig", 8192, NULL, configMAX_PRIORITIES, NULL);
    // //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // //hook isr handler for specific gpio pin
    gpio_isr_handler_add((gpio_num_t) GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    // // //hook isr handler for specific gpio pin
    gpio_isr_handler_add((gpio_num_t) GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    while(wifiIsConnected != 1);

    // //remove isr handler for gpio number.
    gpio_isr_handler_remove((gpio_num_t) GPIO_INPUT_IO_0);
    // //hook isr handler for specific gpio pin again
    gpio_isr_handler_add((gpio_num_t) GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

    uartIntance_g.uartInit();

    xTaskCreate(rx_task, "uart_rx_task", 8192, NULL, configMAX_PRIORITIES, NULL);
    //xTaskCreate(tx_task, "uart_tx_task", 8192, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(flag_lookup_task, "flag_lookup_task", 8192, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(get_server_data_task, "get_server_data_task", 8192, NULL, configMAX_PRIORITIES, NULL);

    //tester();
    for(;;);
}

