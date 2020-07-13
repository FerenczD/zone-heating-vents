#include "uart_esp.h"

int uartSendData(const char* data)
{

    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_X, data, len);
    ESP_LOGI(UART_SEND_LOG_NAME, "Wrote %d bytes", txBytes);
    ESP_LOG_BUFFER_HEXDUMP(UART_SEND_LOG_NAME, data, txBytes, ESP_LOG_INFO);

    return txBytes;
}

int uartReadData(uint8_t* data, portTickType timeout) {

    const int rxBytes = uart_read_bytes(UART_NUM_X, data, sizeof(vent_status_cmd_t), timeout); /* Blocking function add timeout */

    ESP_LOGI(UART_READ_LOG_NAME, "Read %d bytes: '%s'", rxBytes, data);
    ESP_LOG_BUFFER_HEXDUMP(UART_READ_LOG_NAME, data, rxBytes, ESP_LOG_INFO);

    return rxBytes;
}


void uartInit() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_X, &uart_config);
    uart_set_pin(UART_NUM_X, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_X, RX_BUF_SIZE * 2, 0, 0, NULL, 0);

    esp_log_level_set(UART_SEND_LOG_NAME, ESP_LOG_INFO);
    esp_log_level_set(UART_READ_LOG_NAME, ESP_LOG_INFO);
}

