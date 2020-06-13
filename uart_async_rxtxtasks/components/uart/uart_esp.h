#ifndef UART_ESP_H
#define UART_ESP_H

#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TXD_PIN         GPIO_NUM_17
#define RXD_PIN         GPIO_NUM_16
#define UART_NUM_X      UART_NUM_2

#define UART_SEND_LOG_NAME      "UART_SEND_DATA()"
#define UART_READ_LOG_NAME      "UART_READ_DATA()"

#define BLE_ADDR_LEN    6
#define CMD_UUID_LEN    2

static const int RX_BUF_SIZE = 1024;

/* Message sent from ESP to nRF to send command to vent */
typedef struct {
    uint8_t cmd_uuid[CMD_UUID_LEN];     /* UUID for message */
    uint8_t ble_addr[BLE_ADDR_LEN];     /* BLE address of vent */
    uint8_t cmd;              /* Command to open or close vent */
}vent_cmd_t;

/* Message send from nRF to ESP to send vent status information */
typedef struct {
    uint8_t ble_addr[BLE_ADDR_LEN];     /* BLE address of vent */
    uint8_t vent_status;      /* Vent status code (or struct) */
    uint8_t vent_temp[2];
    bool isCmd;
    uint8_t cmd_uuid[CMD_UUID_LEN];     /* UUID for message */
}vent_status_cmd_t;

int uartSendData(const char* data);

int uartReadData(uint8_t* data, portTickType timeout);

void uartInit();




#endif 