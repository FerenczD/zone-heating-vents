#ifndef UART_BLE_H
#define UART_BLE_H

/* Structures and functions to communicate nRF and ESP through UART */

/* Libraries */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nordic_common.h"
#include "app_uart.h"
#include "app_error.h"
#include "app_util.h"
#include "bsp_btn_ble.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/* Constants */
#define UART_TX_BUF_SIZE        256                                     /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE        256                                     /**< UART RX buffer size. */

#define UART_RX_PIN             15
#define UART_TX_PIN             14

#define BLE_ADDR_LEN            6
#define CMD_UUID_LEN            2

#define NORMAL_MODE             0x01
#define PAIRING_MODE            0x02

/* Message sent from ESP to nRF to send command to vent */
typedef struct {
    //uint8_t cmd_uuid[CMD_UUID_LEN];     /* UUID for message */
    uint8_t ble_addr[BLE_ADDR_LEN];     /* BLE address of vent */
    uint8_t cmd;                        /* Command to open or close vent */
    uint8_t mode;                       /* Mode of operation */
}vent_cmd_t;

/* Message send from nRF to ESP to send vent status information */
typedef struct {
    uint8_t ble_addr[BLE_ADDR_LEN];     /* BLE address of vent */
    uint8_t vent_status;                /* Vent status code (or struct) */
    uint8_t vent_temp[2];               /* Vent temperature */
    //bool is_cmd;
    //uint8_t cmd_uuid[CMD_UUID_LEN];     /* UUID for message */
}vent_status_cmd_t;

typedef void (* uart_ble_event_handler)(app_uart_evt_t * p_event);

/* Initialize UART with a given event handler */
void uart_init(uart_ble_event_handler uart_event_handle);

/* Send a stream of bytes through UART */
void uart_send(uint8_t * p_data, int data_len);

/* Abstraction function of app_uart_get */
void uart_get(uint8_t * p_byte);

#endif