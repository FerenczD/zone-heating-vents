/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"
#include "uart_esp.h"

    
vent_cmd_t mVentCmd;                                    /* Instance of vent command structure */
uint8_t* mVentCmdPtr = (uint8_t*)&mVentCmd;             /* Pointer that stores vent command */

vent_status_cmd_t mVentStatus;                          /* Instance of vent status response structure */
uint8_t* mVentStatusPtr = (uint8_t*)&mVentStatus;       /* Pointer that stores vent status */

static void tx_task()
{

    uint8_t testUUID[CMD_UUID_LEN] = { 0x12 , 0x34 };
    uint8_t testBleAddr[BLE_ADDR_LEN] = { 0xAA , 0x17, 0x07, 0x58, 0xCD, 0xF4 };
    uint8_t testCmd = 0x1;

    memcpy( &mVentCmd.cmd_uuid, &testUUID, CMD_UUID_LEN);
    memcpy(&mVentCmd.ble_addr, &testBleAddr, BLE_ADDR_LEN);
    mVentCmd.cmd = testCmd;

    memcpy(mVentCmdPtr, &mVentCmd, sizeof(vent_cmd_t));

    ESP_LOG_BUFFER_HEXDUMP("TEST", mVentCmdPtr, sizeof(vent_cmd_t), ESP_LOG_INFO);

    while (1) {
        uartSendData((char*)mVentCmdPtr);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task()
{

    uint8_t* data = (uint8_t*)calloc(sizeof(mVentCmd), sizeof(uint8_t*));

    while (1) {
        const int rxBytes = uartReadData(data, (portTickType)portMAX_DELAY); /* Blocking function add timeout */

        if (rxBytes > 0) {
            data[rxBytes] = 0;
            uart_flush_input(UART_NUM_X);
        }
    }
    free(data);
}

void app_main()
{
    uartInit();
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}
