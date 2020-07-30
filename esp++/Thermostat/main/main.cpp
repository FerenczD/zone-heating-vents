
/* *********** Standard Libraries *********** */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>        

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
#include "cJSON.h"


/* *********** Components *********** */
#include "uart_esp.h"
#include "HttpServer.h"
#include "HttpPost.h"
#include "SmartConfig.h"
#include "vent_class.h"

/* Tensorflow micro libraries */
#include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "venti_model.h"


#define GPIO_INPUT_IO_0 4
#define GPIO_INPUT_IO_1 39
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

#define FAN_GPIO        23
#define AC_GPIO         5
#define FURNACE_GPIO    18
#define AC_LED          25
#define FAN_LED         26

/* Globals */
static char tag[]="Venti";

/* Data structures */
EspUart uartIntance_g;
std::vector<Vent*> myHomeVents;  /* Vents paired and known by the sytem. Note: This hould be sinchronized with erver at some point and store it locally in flash */
                                 /* Various tasks will have access so do semaphores in the future */
/* Flags */
int wifiIsConnected = 0;
static int pairingEnabled = 0;
static int runGetDataTask = 0;

/* Handles */
static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle pairingEvtQueue = NULL;
static TaskHandle_t getServerDataTaskHandle;
static TaskHandle_t flagLookupTaskHandle;
static TaskHandle_t  rxTaskHandle;

/* Prototypes */
static void smartConfig();
esp_err_t _http_event_handler(esp_http_client_event_t *evt);

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void JSON_ParseAndStore(const cJSON * const jsonArray, std::vector<float>* setTempArr, std::vector<int>* idArr, std::vector<std::string>* nameArr){
    int id = cJSON_GetObjectItem(jsonArray,"id")->valueint;
    // int battery = cJSON_GetObjectItem(array, "battery")->valueint;
    char* ventName = cJSON_GetObjectItem(jsonArray,"name")->valuestring;
    float setTemp = (float)cJSON_GetObjectItem(jsonArray,"setTemp")->valuedouble;
    ESP_LOGI("JSON", "id=%d",id);
    ESP_LOGI("JSON", "name=%s",ventName);
    ESP_LOGI("JSON", "setTemp=%.2f",setTemp);

    setTempArr->push_back(setTemp);
    idArr->push_back(id);

    std::string ventNameString(ventName);
    nameArr->push_back(ventNameString);
}


extern "C"{
    void app_main();
}

static void toggle_fan(int status){
    if(status == 1){     /* Turn on Fan only */
        gpio_set_level((gpio_num_t)FAN_GPIO, status);
        gpio_set_level((gpio_num_t)FAN_LED, status);


    }else if(status == 0){
        gpio_set_level((gpio_num_t)FAN_GPIO, status);
        gpio_set_level((gpio_num_t)FAN_LED, status);
    }
}

static void toggle_ac(int status){
    if(status == 1){     /* Turn on AC and Fan*/
        gpio_set_level((gpio_num_t)FAN_GPIO, status);
        gpio_set_level((gpio_num_t)FAN_LED, status);
        vTaskDelay(12000 / portTICK_PERIOD_MS);
        gpio_set_level((gpio_num_t)AC_GPIO, status);
        gpio_set_level((gpio_num_t)AC_LED, status);

    }else if(status == 0){
        gpio_set_level((gpio_num_t)FAN_GPIO, status);
        gpio_set_level((gpio_num_t)FAN_LED, status);
        vTaskDelay(12000 / portTICK_PERIOD_MS);
        gpio_set_level((gpio_num_t)AC_GPIO, status);
        gpio_set_level((gpio_num_t)AC_LED, status);

    }
}

static void tx_task(void *pvParameter){
    
    for(auto it = std::begin(myHomeVents); it != std::end(myHomeVents); ++it) {
        Vent* ventInstance = *it;
        
        uartIntance_g.uartSetWriteData(ventInstance->getLatestVentCommand(), ventInstance->getMacAddrBytes());

        ESP_LOGI("TX", "Command: %d", ventInstance->getLatestVentCommand());
        ESP_LOGI("TX", "Mac: %s", ventInstance->getMacAddrStr().c_str());

        uartIntance_g.uartSendData();

        vTaskDelay(100 / portTICK_PERIOD_MS);  /* Enough time for BLE to process data  */
    }
    
    vTaskDelete(NULL);
}

static void the_algorithm_task(void* pvParameters){

    const int deltaThreshold = 1;
    const float fanThreshold = 0.5;
    int fan = 0;
    int ac = 0;
    float lowestDeltaT = 0;
    float highestDeltaT = 0;

    /* Determine fan and ac */
    for(auto it = std::begin(myHomeVents); it != std::end(myHomeVents); ++it) {
        Vent* ventInstance = *it;

        float deltaT = (ventInstance->getSetTemperature() - ventInstance->getCurrentTemperature());

        if(abs(deltaT) >= fanThreshold && deltaT < lowestDeltaT){
            lowestDeltaT = deltaT;
        }else if(abs(deltaT) >= fanThreshold && deltaT > highestDeltaT){
                highestDeltaT = deltaT;
        }
    }

    if(abs(highestDeltaT + lowestDeltaT) <= deltaThreshold){
        fan = 1;
    }

    if(fan == 0 && lowestDeltaT <= -deltaThreshold){
        ac = 1;
    }
    
    /* Determine vent command */
    for(auto it = std::begin(myHomeVents); it != std::end(myHomeVents); ++it) {
        Vent* ventInstance = *it;

        float deltaT = (ventInstance->getSetTemperature() - ventInstance->getCurrentTemperature());
            
            if(ac == 1){
                if(deltaT < -deltaThreshold){
                    ventInstance->setLatestVentCommand(0x01);   /* Open vent */
                }else{
                    ventInstance->setLatestVentCommand(0x02);   /* Close vent */
                }
            }else{
                ventInstance->setLatestVentCommand(0x01);   /* Open for pressure protection */
            }
    }
    
    /* Turn on or off the HVAC as required */
    char* response;

    if(ac == 1){
        toggle_ac(1);
        ESP_LOGI("ALGORITHM", "AC On");
        do{
            response = request(_http_event_handler, "action=updateHVAC&hvac=Cool");
        }while(response == NULL);
        free(response);
        do{
            response = request(_http_event_handler, "action=updateFan&fan=Auto");
        }while(response == NULL);
        free(response);

    }else if (fan == 1){
        toggle_ac(0);   /* Make sure fan is off first */
        do{
            response = request(_http_event_handler, "action=updateHVAC&hvac=Off");
        }while(response == NULL);
        free(response);

        toggle_fan(1);  /* Toggle only fan */
        ESP_LOGI("ALGORITHM", "Fan On");
        do{
            response = request(_http_event_handler, "action=updateFan&fan=On");
        }while(response == NULL);
        free(response);

    }else{
        toggle_ac(0);
        ESP_LOGI("ALGORITHM", "HVAC Off");
        do{
            response = request(_http_event_handler, "action=updateHVAC&hvac=Off");
        }while(response == NULL);
        free(response);

        do{
            response = request(_http_event_handler, "action=updateFan&fan=Auto");
        }while(response == NULL);
        free(response);

    }

    /*********** TEST ***************/
    /* Determine */
    // bool acFlag = 0;
    // char* response;
    // for(auto it = std::begin(myHomeVents); it != std::end(myHomeVents); ++it) {
    //     Vent* ventInstance = *it;

    //     int deltaT = (ventInstance->getSetTemperature() - ventInstance->getCurrentTemperature());

    //     if(deltaT >= 1){
    //         ventInstance->setLatestVentCommand(0x01);   /* Close */
    //     }else if(deltaT <= -1){
    //         ventInstance->setLatestVentCommand(0x02);   /* Open */
    //         acFlag = 1;
    //     }else{
    //         ventInstance->setLatestVentCommand(0x00);   /* Nothing */     
    //     }
    // }
    
    // if(acFlag == 1){
    //     toggle_ac(1);
    //     ESP_LOGI("ALGORITHM", "AC On");
    //     do{
    //         response = request(_http_event_handler, "action=updateHVAC&hvac=Cool");
    //     }while(response == NULL);
    //     do{
    //         response = request(_http_event_handler, "action=updateFan&fan=Auto");
    //     }while(response == NULL);
    // }else{
    //     toggle_ac(0);
    //     ESP_LOGI("ALGORITHM", "AC Off");

    //     do{
    //         response = request(_http_event_handler, "action=updateHVAC&hvac=Off");
    //     }while(response == NULL);
    //     do{
    //         response = request(_http_event_handler, "action=updateFan&fan=Auto");
    //     }while(response == NULL);

    // }


    xTaskCreate(tx_task, "tx_task", 8192, NULL, configMAX_PRIORITIES - 1, NULL);

    vTaskDelete(NULL);
}
/***************************** Tenorflow stuff ***************************************/
// Globals, used for compatibility with Arduino-style sketches.
namespace {
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    int inference_count = 0;

    // Create an area of memory to use for input, output, and intermediate arrays.
    // Finding the minimum value for your model may require some trial and error.
    constexpr int kTensorArenaSize = 2 * 2048;
    uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

static void setup(){
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(venti_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        error_reporter->Report(
            "Model provided is schema version %d not equal "
            "to supported version %d.",
            model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    // This pulls in all the operation implementations we need.
    // NOLINTNEXTLINE(runtime-global-variables)
    static tflite::ops::micro::AllOpsResolver resolver;

    // Build an interpreter to run the model with.
    static tflite::MicroInterpreter static_interpreter(
            model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    // Obtain pointers to the model's input and output tensors.
    input = interpreter->input(0);

    ESP_LOGI("TENSOR SETUP", "input size = %d", input->dims->size);
    ESP_LOGI("TENSOR SETUP", "input size in bytes = %d", input->bytes);
    ESP_LOGI("TENSOR SETUP", "Is input float32? = %s", (input->type == kTfLiteFloat32) ? "true" : "false");
    ESP_LOGI("TENSOR SETUP", "Input data dimentions = %d",input->dims->data[1]);

    output = interpreter->output(0);

    ESP_LOGI("TENSOR SETUP", "output size = %d", output->dims->size);
    ESP_LOGI("TENSOR SETUP", "output size in bytes = %d", output->bytes);
    ESP_LOGI("TENSOR SETUP", "Is input float32? = %s", (output->type == kTfLiteFloat32) ? "true" : "false");
    ESP_LOGI("TENSOR SETUP", "Output data dimentions = %d",output->dims->data[1]);



}

static bool setupDone = true;

static void the_ai_algorithm_task(){

    /* First time task is init setup the ai model */
    if(setupDone == false){
        setup();
        setupDone = true;
    }

    /* Load the input data i.e deltaT1 and deltaT2 */
    //int i = 0;

    float val1 = 0.0;
    float val2 = 0.4;

    input->data.f[0] = val1;    
    input->data.f[1] = val2;


    /* Run model */
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        error_reporter->Report("Invoke failed");
        // return;
    }

    /* Retrieve outputs Fan , AC , Vent 1 , Vent 2 */
    double fan = (double)output->data.f[0];
    double ac =  (double)output->data.f[1];
    double vent1 =  (double)output->data.f[2];
    double vent2 =  (double)output->data.f[3];


    ESP_LOGI("TENSOR SETUP", "fan = %f", fan);
    ESP_LOGI("TENSOR SETUP", "ac = %f", ac);
    ESP_LOGI("TENSOR SETUP", "vent1 = %f", vent1);
    ESP_LOGI("TENSOR SETUP", "vent2 = %f", vent2);
    
}
/************************** end of tensorflow stuff ************************************/

static void update_data_task(void *pvParameters){

    ventStatus_t receivedData = *(ventStatus_t*)pvParameters;

    /* Update local data structure */
    Vent* localVent = findVentByMacBytes(receivedData.bleAddr, myHomeVents);

    localVent->setStatus(receivedData.ventStatus);
    // localVent->setCurrentTemperature((uint16_t*)receivedData.ventTemp);

    /* Update server */
    char query[255] = {0};
    sprintf(query, "action=updateExistingVent&id=%d&temp=%d&status=%d", localVent->getId(), 
                                                                        *(uint16_t*)receivedData.ventTemp,
                                                                        receivedData.ventStatus);

    char* response;
    do{
        response = request(_http_event_handler, query);
    }while(response == NULL);
    free(response);

    /* Check http response for errors */

    /* Run the algorithm task. Note: Important to do everytime you receive new data from vents */
    xTaskCreate(the_algorithm_task, "the_algortihm_task", 8192, NULL, configMAX_PRIORITIES - 1, NULL);

    vTaskDelete(NULL);
}

static void rx_task(void *pvParameter)
{
    while(1){
        int rxBytes = uartIntance_g.uartRead((portTickType)portMAX_DELAY );

        if(rxBytes > 0){
            ventStatus_t receivedData = uartIntance_g.uartGetStatus(); 
            ESP_LOG_BUFFER_HEXDUMP(UART_READ_LOG_NAME, &receivedData, rxBytes, ESP_LOG_INFO);

            if(pairingEnabled == 1){
                uint32_t pairingCompleted = 1;

                ESP_LOGI("RX", "Queue send");
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
    const TickType_t callFrequency = 7000 / portTICK_PERIOD_MS;    /* 10 seconds */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    char query[255] = {0};
    sprintf(query, "action=getVentData");  /* Get all data */

    char* response;

    for(;;){

        // while(runGetDataTask == 0);

        do{
            response = request(_http_event_handler, query);
        }while(response == NULL);

        if((*response - '0' != 0) && (strlen(response) > 2)){       /**************************/
            /* Parse response with cJSON and get ids and setTemperatures */
            cJSON* root = cJSON_Parse(response);
            int rootArraySize = cJSON_GetArraySize(root);

            std::vector<float> setTempArr;
            std::vector<int> idArr;
            std::vector<std::string> nameArr;

            for(int i=0; i < rootArraySize; i++){
                cJSON *jsonArr = cJSON_GetArrayItem(root, i);
                JSON_ParseAndStore(jsonArr, &setTempArr, &idArr, &nameArr);
            }

            /* Update local data structure with new setTemperature */
            updateHomeVents(myHomeVents, idArr, setTempArr, nameArr); 

        }else{
            ESP_LOGI("GET_DATA", "No vent data in database");
        }

        free(response);
        vTaskDelayUntil(&xLastWakeTime, callFrequency);
        xLastWakeTime = xTaskGetTickCount();
    }

    vTaskDelete(NULL);
}

static void uart_pairing_task(void *pvParameter){

    /* Suspend tasks temporarily */
    vTaskSuspend(getServerDataTaskHandle);
    vTaskSuspend(flagLookupTaskHandle);

    pairingEnabled = 1;
    uint32_t pairingCompleted = 0;

    char* response;
    uint8_t uartReceivedByPeer = 0;

    ESP_LOGI("PAIRING", "Pairing task entered");

    /* Send pairing mode to BLE */
    uartIntance_g.uartSendPairingModeRequest(); 

    ESP_LOGI("PAIRING", "before queue received");

    /* Wait for BLE to send MAC information */
    xQueueReceive(pairingEvtQueue, &pairingCompleted, portMAX_DELAY);
    ESP_LOGI("PAIRING", "after queue received");

    /* Making sure flag was actually toggled */
    if(pairingCompleted == 1){
        
        /* Avoid receiving any new information while in pairing mode */
        vTaskSuspend(rxTaskHandle);

        ESP_LOGI("PAIRING", "Pairing done. This is the MAC address of vent");

        /* Retrieve information from vent */
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

        /* Check vent doesnt exist already */
        bool stopPairingProcess = false;

        if(myHomeVents.size() > 0){
            Vent* ventInstance = findVentByMacBytes(receivedData.bleAddr, myHomeVents);

            if(ventInstance != NULL){
                if(memcmp(ventInstance->getMacAddrBytes(), receivedData.bleAddr, BLE_ADDR_LEN) == 0){

                    /* Vent already exists. Cancel pairing */

                    ESP_LOGI("PAIRING", "EXISTS EXISTS EXISTS EXISTS");

                    stopPairingProcess = true;
                    // vTaskResume(rxTaskHandle);                         
                }
            }

        }
        
        if(stopPairingProcess != true) {

            /* Add vent to server */
            do{
                response = request(_http_event_handler, query);        
            }while(response == NULL);

            free(response);

            int pairingDoneFlag = 0;                                        /* VentCompletedPairing */
            const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

            /* Wait for pairing to be completed and retrieve ID of new vent */
            while(pairingDoneFlag == 0){

                do{
                    response = request(_http_event_handler, "action=checkIfVentCompletedPairing");
                }while(response == NULL);

                if (*response - '0' == 1){

                    free(response);

                    sprintf(query, "action=getIdForMac&mac=%02x%02x%02x%02x%02x%02x", receivedData.bleAddr[0],
                                                                                    receivedData.bleAddr[1],
                                                                                    receivedData.bleAddr[2],
                                                                                    receivedData.bleAddr[3],
                                                                                    receivedData.bleAddr[4],
                                                                                    receivedData.bleAddr[5]);
                    do{
                        response = request(_http_event_handler, query);
                    }while(response == NULL);

                    /* Store all vent data on local variable */
                    Vent* newVent = new Vent;
                    newVent->setCurrentTemperature(receivedData.ventTemp);
                    newVent->setStatus(receivedData.ventStatus);
                    newVent->setMacAddr(receivedData.bleAddr);
                    newVent->setId(atoi(response));

                    myHomeVents.push_back(newVent);

                    free(response);

                    pairingDoneFlag = 1;
                    // vTaskResume(rxTaskHandle);

                }else{
                    free(response);  
                }

                vTaskDelay( xDelay );
            }   
        }
    }else{
        ESP_LOGI("PAIRING", "Error pairing. PairingComplete = 0");
    }

    pairingEnabled = 0;
    ESP_LOGI("PAIRING", "Pairing task completed. Killing task");

    /* Resume tasks */
    vTaskResume(rxTaskHandle);
    vTaskResume(getServerDataTaskHandle);
    vTaskResume(flagLookupTaskHandle);

    vTaskDelete(NULL);

}

static void flag_lookup_task(void *pvParameter){
    const TickType_t callFrequency = 1000 / portTICK_PERIOD_MS;    /* 1 seconds */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    char * response;

    for(;;){
        do{
            response = request(_http_event_handler, "action=checkIfVentsNeedPairing");
        }while(response == NULL);
 
        if(*response - '0' == 1){
            ESP_LOGI("FLAG_TASK", "Pairing required starting pairing task");
            
            pairingEnabled = 1;

            xTaskCreate(uart_pairing_task, "uart_pairing_task", 16384, NULL, configMAX_PRIORITIES - 1, NULL);

            while(pairingEnabled == 1){
                vTaskDelay(100 / portTICK_PERIOD_MS);   /* Just error checking. It hould exit while loop instantly */
            }; /* Wait until pairing is done. In theory no other vent should be sending data */

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

    // ESP_ERROR_CHECK(httpServerConnect());
    char * response;

    do{
        response = request(_http_event_handler, "action=confirmSmartConfigCompleted");
    }while(response == NULL);

    wifiIsConnected = 1;

    free(response);
    vTaskDelete(NULL);
}


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(POST_TAG, "HTTP_EVENT_ERROR");
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
    setup();
    the_ai_algorithm_task();
    for(;;){}

}

void app_main()
{
    // ESP_ERROR_CHECK( nvs_flash_init() );
    // // tcpip_adapter_init();
    // // ESP_ERROR_CHECK(esp_event_loop_create_default());
    // // ESP_ERROR_CHECK(httpServerConnect());
    
    // gpio_config_t io_conf;

    // /* HVAC gpios */
    // //disable interrupt
    // io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    // //set as output mode
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // //bit mask of the pins that you want to set,e.g.GPIO18/19
    // io_conf.pin_bit_mask = ((1ULL<<AC_GPIO) | (1ULL<<FAN_GPIO) | (1ULL<<FURNACE_GPIO) | (1ULL<<AC_LED) | (1ULL<<FAN_LED));
    // //disable pull-down mode
    // io_conf.pull_down_en = (gpio_pulldown_t)0;
    // //disable pull-up mode
    // io_conf.pull_up_en = (gpio_pullup_t)0;
    // //configure GPIO with the given settings
    // gpio_config(&io_conf);

    // /* Interrupt gpio */
    //  //interrupt of rising edge
    // io_conf.intr_type = (gpio_int_type_t) GPIO_PIN_INTR_POSEDGE;
    // // //bit mask of the pins, use GPIO4/5 here
    // io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // // //set as input mode    
    // io_conf.mode = GPIO_MODE_INPUT;
    // // //enable pull-up mode
    // io_conf.pull_down_en = (gpio_pulldown_t) 0;
    // io_conf.pull_up_en = (gpio_pullup_t)1;

    // gpio_config(&io_conf);

    // // // //change gpio intrrupt type for one pin
    // gpio_set_intr_type((gpio_num_t) GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    // // //create a queue to handle gpio event from isr
    // gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // // // Create Queue to handle pairing completed information
    // pairingEvtQueue = xQueueCreate(3, sizeof(uint32_t));
    // // //start gpio task
    
    // xTaskCreate((TaskFunction_t ) smartConfig, "smartConfig", 16383, NULL, 10, NULL);

    // if(wifiIsConnected == 1) {
    //     ESP_ERROR_CHECK(httpServerConnect());
    // }
    // // xTaskCreate((TaskFunction_t ) smartConfig, "smartConfig", 8192, NULL, configMAX_PRIORITIES, NULL);
    // //install gpio isr service
    // gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // // //hook isr handler for specific gpio pin
    // gpio_isr_handler_add((gpio_num_t) GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    // // // //hook isr handler for specific gpio pin
    // gpio_isr_handler_add((gpio_num_t) GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    // while(wifiIsConnected != 1);

    // // //remove isr handler for gpio number.
    // gpio_isr_handler_remove((gpio_num_t) GPIO_INPUT_IO_0);
    // // //hook isr handler for specific gpio pin again
    // gpio_isr_handler_add((gpio_num_t) GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

    // uartIntance_g.uartInit();

    // xTaskCreate(rx_task, "uart_rx_task", 8192, NULL, configMAX_PRIORITIES - 1, &rxTaskHandle);
    // // //xTaskCreate(tx_task, "uart_tx_task", 8192, NULL, configMAX_PRIORITIES-1, NULL);
    // xTaskCreate(flag_lookup_task, "flag_lookup_task", 16383, NULL, configMAX_PRIORITIES -1 , &flagLookupTaskHandle);
    // xTaskCreate(get_server_data_task, "get_server_data_task", 8192, NULL, configMAX_PRIORITIES - 1, &getServerDataTaskHandle);

    tester();
    for(;;);
}

