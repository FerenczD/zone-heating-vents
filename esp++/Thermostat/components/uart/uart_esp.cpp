#include "uart_esp.h"

int EspUart::uartSendData()
{
    const char* data = (char*)&this->ventCmd;
    const int len = sizeof(ventCmd_t);

    const int txBytes = uart_write_bytes(UART_NUM_X, data, len);

    ESP_LOGI(UART_SEND_LOG_NAME, "Wrote %d bytes", txBytes);
    ESP_LOG_BUFFER_HEXDUMP(UART_SEND_LOG_NAME, data, txBytes, ESP_LOG_INFO);

    return txBytes;
}

int EspUart::uartRead(portTickType timeout) {

    uint8_t* buffer = (uint8_t*)calloc(sizeof(ventStatus_t), sizeof(uint8_t));

    const int rxBytes = uart_read_bytes(UART_NUM_X, buffer, sizeof(ventStatus_t), timeout); /* Blocking function add timeout */

    memcpy(&this->ventStatus, buffer, sizeof(ventStatus_t));

    ESP_LOGI(UART_READ_LOG_NAME, "Read %d bytes: '%s'", rxBytes, buffer);
    ESP_LOG_BUFFER_HEXDUMP(UART_READ_LOG_NAME, buffer, rxBytes, ESP_LOG_INFO);

    uart_flush_input(UART_NUM_X);
    free(buffer);

    return rxBytes;
}

ventStatus_t EspUart::uartGetStatus(){
    return this->ventStatus;
}

void EspUart::uartSetWriteData(uint8_t cmd, uint8_t* addr){
    this->ventCmd.cmd = cmd;
    memcpy(&this->ventCmd.bleAddr, addr, BLE_ADDR_LEN);
    //memcpy(&this->ventCmd.cmd_uuid, uuid, CMD_UUID_LEN);
    this->ventCmd.mode = NORMAL_MODE;
}

int EspUart::uartSendPairingModeRequest(){
    this->ventCmd.cmd = 0x00;   /* Vent stops motor or doesnt do anything */
    memcpy(&this->ventCmd.bleAddr, &defaultBleAddr, BLE_ADDR_LEN);
    this->ventCmd.mode = PAIRING_MODE;

    return uartSendData();
}

void EspUart::uartInit() {
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
    uart_flush_input(UART_NUM_X);

}

uint8_t EspUart::uartCheckMessageReceived(portTickType timeout){
    
    uint8_t buffer = 0;

    const int rxBytes = uart_read_bytes(UART_NUM_X, &buffer, 1, timeout); /* Blocking function add timeout */

    if(buffer > 0x01){
        ESP_LOGI(UART_READ_LOG_NAME, "Uart message received by peer");
        buffer = 0x01;
    }else{
        ESP_LOGI(UART_READ_LOG_NAME, "Uart message error");
        buffer = 0x00;
    }

    uart_flush_input(UART_NUM_X);
    
    return buffer;
}

