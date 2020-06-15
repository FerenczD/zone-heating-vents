#include "dc_motor.h"


void open_vent(){
   int limit_switch_active = nrf_gpio_pin_read(LIMIT_SWITCH_OPEN);

   if (limit_switch_active != 0){
      nrf_gpio_pin_set(DC_ENABLE_PIN);
      nrf_gpio_pin_set(DC_DIRECTION_A_PIN);
      nrf_gpio_pin_clear(DC_DIRECTION_B_PIN);
   }

}

void close_vent(){
   int limit_switch_active = nrf_gpio_pin_read(LIMIT_SWITCH_CLOSE);

   if (limit_switch_active != 0){
     nrf_gpio_pin_set(DC_ENABLE_PIN);
     nrf_gpio_pin_clear(DC_DIRECTION_A_PIN);
     nrf_gpio_pin_set(DC_DIRECTION_B_PIN);
   }
}

void stop_vent(){
   nrf_gpio_pin_clear(DC_ENABLE_PIN);
   nrf_gpio_pin_clear(DC_DIRECTION_A_PIN);
   nrf_gpio_pin_clear(DC_DIRECTION_B_PIN);
}

uint8_t get_vent_direction(){
  
  /* Read current DC motor pin values */
  uint32_t enable_pin = nrf_gpio_pin_out_read(DC_ENABLE_PIN);
  uint32_t direction_a_pin = nrf_gpio_pin_out_read(DC_DIRECTION_A_PIN);
  uint32_t direction_b_pin = nrf_gpio_pin_out_read(DC_DIRECTION_B_PIN);

  /* If enable button is off then motor is off already */
  if (enable_pin != 1){
    NRF_LOG_INFO("Enable off already. Returning error");
    return 0;
  }
  
  if((direction_a_pin == 1) && (direction_b_pin != 1)){
    /* Vent is opening */
    NRF_LOG_INFO("Vent opening");
    return 1;
  }else if ((direction_a_pin != 1) && (direction_b_pin == 1)){
    /* Vent is closing */
    NRF_LOG_INFO("Vent closing");
    return 2; 
  }else {
    /* error state */
    return 0;
  }
  
}

void limit_switch_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action){

  ret_code_t err_code;
  uint8_t direction =  get_vent_direction();  /* Confirm that the limit switch activated is the same as the vent action. Avoid wrong limit switch triggering*/

  if(pin == LIMIT_SWITCH_OPEN && direction == 1){
    NRF_LOG_INFO("Limit Switch open pressed");
    stop_vent();
    status_update(STATUS_VENT_OPENED);

  }else if(pin == LIMIT_SWITCH_CLOSE && direction == 2){
    NRF_LOG_INFO("Limit Switch close pressed");
    stop_vent();
    status_update(STATUS_VENT_CLOSED);

  }else{
    NRF_LOG_INFO("Limit switch state error");

  }
}

void dc_init(){
    nrf_gpio_cfg_output(DC_DIRECTION_B_PIN);
    nrf_gpio_cfg_output(DC_DIRECTION_A_PIN);
    nrf_gpio_cfg_output(DC_ENABLE_PIN);

    nrf_gpio_pin_clear(DC_DIRECTION_B_PIN);
    nrf_gpio_pin_clear(DC_DIRECTION_A_PIN);
    nrf_gpio_pin_clear(DC_ENABLE_PIN);
 
}

void limit_switch_init(){
    ret_code_t err_code;
  
    //Initialize gpiote module
    err_code = nrfx_gpiote_init();
    APP_ERROR_CHECK(err_code);
    
    //Initialize output pin
    nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(false);                 //Configure output button

    //Configure sense input pin to enable wakeup and interrupt on button press.
    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);              //Configure to generate interrupt and wakeup on pin signal low. "false" means that gpiote will use the PORT event, which is low power, i.e. does not add any noticable current consumption (<<1uA). Setting this to "true" will make the gpiote module use GPIOTE->IN events which add ~8uA for nRF52 and ~1mA for nRF51.

    in_config.pull = NRF_GPIO_PIN_PULLUP;                                                     //Configure pullup for input pin to prevent it from floting. Pin is pulled down when button is pressed on nRF5x-DK boards, see figure two in http://infocenter.nordicsemi.com/topic/com.nordic.infocenter.nrf52/dita/nrf52/development/dev_kit_v1.1.0/hw_btns_leds.html?cp=2_0_0_1_4		

    err_code = nrfx_gpiote_in_init(LIMIT_SWITCH_OPEN, &in_config, limit_switch_handler);   //Initialize the pin with interrupt handler
    APP_ERROR_CHECK(err_code);                                                                //Check potential error

    err_code = nrfx_gpiote_in_init(LIMIT_SWITCH_CLOSE, &in_config, limit_switch_handler);  //Initialize the pin with interrupt handler
    APP_ERROR_CHECK(err_code);                                                                //Check potential error

    nrfx_gpiote_in_event_enable(LIMIT_SWITCH_OPEN, true);                                  //Enable event and interrupt for the wakeup pin
    nrfx_gpiote_in_event_enable(LIMIT_SWITCH_CLOSE, true);                                  //Enable event and interrupt for the wakeup pin

}
