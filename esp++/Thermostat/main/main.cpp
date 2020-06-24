/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "uart_esp.h"

#include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"

static char tag[]="cpp_helloworld";

EspUart uartIntance_g;

extern "C"{
    void app_main();
}

static void tx_task(void *pvParameter){
    uint8_t testUUID[CMD_UUID_LEN] = { 0x12 , 0x34 };
    uint8_t testBleAddr[BLE_ADDR_LEN] = { 0xAA , 0x17, 0x07, 0x58, 0xCD, 0xF4 };
    uint8_t testCmd = 0x1;

    uartIntance_g.uartSetWriteData(testCmd, testUUID, testBleAddr);

    while(1){
        uartIntance_g.uartSendData();
        vTaskDelay(2000 / portTICK_PERIOD_MS); 
    }
}

static void rx_task(void *pvParameter)
{
    while(1){
        int rxBytes = uartIntance_g.uartRead((portTickType)portMAX_DELAY);

        if(rxBytes > 0){

            ventStatus_t receivedData = uartIntance_g.uartGetStatus(); 

            ESP_LOG_BUFFER_HEXDUMP(UART_READ_LOG_NAME, &receivedData, rxBytes, ESP_LOG_INFO);

        }    
    }
}

void app_main()
{
    
    uartIntance_g.uartInit();

    xTaskCreate(rx_task, "uart_rx_task", 8192, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 8192, NULL, configMAX_PRIORITIES-1, NULL);

    for(;;);
}
