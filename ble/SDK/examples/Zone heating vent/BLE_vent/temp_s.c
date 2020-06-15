#include "temp_s.h"

static const nrfx_twim_t m_twim = NRFX_TWIM_INSTANCE(0);  /* I2C instance */

static volatile bool m_xfer_done = false;                 /* Indicates if operation on TWI has ended. */

static uint8_t data_buf[2];                               /* Slave data buffer */


void twim_handler(nrfx_twim_evt_t const * p_event, void * p_context){
    switch (p_event->type)
    {
        case NRFX_TWIM_EVT_DONE:
            if (p_event->xfer_desc.type == NRFX_TWIM_XFER_RX)
            {
                float* temp_ptr = (float*)p_context;
                int16_t raw_temp = ((data_buf[0] << 8) | data_buf[1]);

                *temp_ptr = raw_temp * 0.0078125;
            }
            m_xfer_done = true;
            break;
        default:
            break;
    }
}

void read_sensor_data(){

    ret_code_t err_code; 

    uint8_t reg = TEMP_REG;

    m_xfer_done = false;
    err_code = nrfx_twim_tx(&m_twim, TMP116_ADDR, &reg, 1, false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);

    m_xfer_done = false;
    err_code = nrfx_twim_rx(&m_twim, TMP116_ADDR, data_buf, sizeof(data_buf));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);

    /* Read 1 byte from the specified address - skip 3 bits dedicated for fractional part of temperature. */
}

void set_one_shot_temp(){
    ret_code_t err_code; 

    /* Setting up configuration register following datasheet and system parameters */
    uint8_t reg[3] = {CONFIG_REG, (ONE_SHOT_CONFIG >> 8) , ONE_SHOT_CONFIG};  /* 1. Desired register 2. MSB 3. LSB of the config*/ 

    m_xfer_done = false;
    err_code = nrfx_twim_tx(&m_twim, TMP116_ADDR, reg, sizeof(reg), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);

//    /* Writing address of temperature reg for future readings */
//    reg[0] = TEMP_REG;
//    m_xfer_done = false;
//    err_code = nrfx_twim_tx(&m_twim, TMP116_ADDR, reg, 1, false);
//    APP_ERROR_CHECK(err_code);
//    while (m_xfer_done == false);

}

void setup_temp_sensor(){
   ret_code_t err_code; 

    /* Setting up configuration register following datasheet and system parameters */
    uint8_t reg[3] = {CONFIG_REG, (ONE_SHOT_CONFIG >> 8) , ONE_SHOT_CONFIG};  /* 1. Desired register 2. MSB 3. LSB of the config*/ 

    m_xfer_done = false;
    err_code = nrfx_twim_tx(&m_twim, TMP116_ADDR, reg, sizeof(reg), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);

    /* Writing address of temperature reg for future readings */
    reg[0] = TEMP_REG;
    m_xfer_done = false;
    err_code = nrfx_twim_tx(&m_twim, TMP116_ADDR, reg, 1, false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
    
    nrf_delay_ms(500);

    m_xfer_done = false;
    err_code = nrfx_twim_rx(&m_twim, TMP116_ADDR, &data_buf, sizeof(data_buf));
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);



}

void twim_init(float* temp_ptr){

    ret_code_t err_code;

    nrfx_twim_config_t twim_config;     
    
    twim_config.scl                  = SCL_PIN;
    twim_config.sda                  = SDA_PIN;
    twim_config.frequency            = NRF_TWIM_FREQ_100K;
    twim_config.interrupt_priority   = APP_IRQ_PRIORITY_HIGH;
    twim_config.hold_bus_uninit        = false;

    err_code = nrfx_twim_init(&m_twim, &twim_config, twim_handler, (void*)temp_ptr);
    APP_ERROR_CHECK(err_code);

    nrfx_twim_enable(&m_twim);
}

void temp_ready_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action){

  NRF_LOG_INFO("Here");

}

void temp_sensor_init(float* temp_ptr){
    twim_init(temp_ptr);
    setup_temp_sensor();
}

