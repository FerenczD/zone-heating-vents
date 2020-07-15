#ifndef UART_ESP_H
#define UART_ESP_H

#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TXD_PIN         GPIO_NUM_17
#define RXD_PIN         GPIO_NUM_16
#define UART_NUM_X      UART_NUM_2

#define UART_SEND_LOG_NAME      "UART_SEND_DATA()"
#define UART_READ_LOG_NAME      "UART_READ_DATA()"

#define BLE_ADDR_LEN    6
#define CMD_UUID_LEN    2

#define NORMAL_MODE             0x01
#define PAIRING_MODE            0x02

static const int RX_BUF_SIZE = 1024;
static const uint8_t defaultBleAddr[BLE_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* Message sent from ESP to nRF to send command to vent */
typedef struct {
    //uint8_t cmd_uuid[CMD_UUID_LEN];     /* UUID for message */
    uint8_t bleAddr[BLE_ADDR_LEN];     /* BLE address of vent */
    uint8_t cmd;                        /* Command to open or close vent */
    uint8_t mode;                       /* Mode of operation of the BLE. Pairing or Normal*/
}ventCmd_t;

/* Message send from nRF to ESP to send vent status information */
typedef struct {
    uint8_t bleAddr[BLE_ADDR_LEN];     /* BLE address of vent */
    uint8_t ventStatus;                /* Vent status code (or struct) */
    uint8_t ventTemp[2];               /* Vent temperature */
    //bool isCmd;
    //uint8_t cmd_uuid[CMD_UUID_LEN];     /* UUID for message */
}ventStatus_t;

class EspUart
{
    private:
        ventCmd_t       ventCmd;
        ventStatus_t    ventStatus;
    public:

        void uartInit();

        int uartSendData();

        int uartRead(portTickType timeout);     /* Read for X amount of time the UART buffer and store read value in this->ventStatus */

        ventStatus_t uartGetStatus();           /* Retrieve ventStatus */

        void uartSetWriteData(uint8_t cmd, uint8_t* addr);    /* Set vent cmd*/

        int uartSendPairingModeRequest();

        uint8_t uartCheckMessageReceived(portTickType timeout);

};



#endif 