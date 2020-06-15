#include "uart_ble.h"

/**@brief Function for initializing the UART. */
void uart_init(uart_ble_event_handler uart_event_handle)
{
    ret_code_t err_code;

    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = UART_RX_PIN,
        .tx_pin_no    = UART_TX_PIN,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud115200
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);

    APP_ERROR_CHECK(err_code);
}

/**@brief Function for sending data through UART
 *
 * @details 
 */
void uart_send(uint8_t * p_data)
{    
      ret_code_t ret_val;
      printf(p_data);     /* printf or app_uart_put() both work */

//      for (int i = 0; i < data_len; i++){
//        do
//          {
//              ret_val = app_uart_put(p_data[i]);
//              
//              if ((ret_val != NRF_SUCCESS) && (ret_val != NRF_ERROR_BUSY))
//              {
//                  NRF_LOG_INFO("app_uart_put failed for index 0x%04x.", i);
//                  APP_ERROR_CHECK(ret_val);
//              }
//          } while (ret_val == NRF_ERROR_BUSY);
//      }


}

void uart_get(uint8_t * buffer_byte){
    ret_code_t          err_code;

    err_code = app_uart_get(buffer_byte);
}