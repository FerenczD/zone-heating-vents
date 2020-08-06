#ifndef DC_MOTOR_H
#define DC_MOTOR_H

/* Functions and definitions that deal directly with the mechanism that opens and closes the air vents */

/* Libraries */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "bsp_btn_ble.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "ble_ves.h"
#include "util.h"

/* Definitions */
//#define DC_ENABLE_PIN                   31                                      /* Enable DC motor output */
//#define DC_DIRECTION_A_PIN              30                                      /* Set direction A */
//#define DC_DIRECTION_B_PIN              29                                      /* Set direction B */

#define DC_ENABLE_PIN                   31                                      /* Enable DC motor output */
#define DC_DIRECTION_A_PIN              10                                      /* Set direction A */
#define DC_DIRECTION_B_PIN              9                                       /* Set direction B */

#define LIMIT_SWITCH_OPEN               BSP_BUTTON_2                            /* Stop motor when limit switch is hit and vent is fully open */
#define LIMIT_SWITCH_CLOSE              BSP_BUTTON_3                            /* Stop motor when limit switch is hit and vent is fully closed */

#define CURRENT_SENSE_ENABLE_PIN        11

#define DC_MOTOR_OPEN                   0x01
#define DC_MOTOR_CLOSE                  0x02

/* Functions */

void dc_init();

void limit_switch_init(nrfx_gpiote_in_config_t* in_config );

/**@brief Function that opens the vent
 *
 * @details This function uses a DC motor H bridge and GPIOs pins predefined
   @ret: True: Vent opening
         False: Vent already open
 */
void open_vent();

/**@brief Function that opens the vent
 *
 * @details This function uses a DC motor H bridge and GPIOs pins predefined
 */
void close_vent();

/**@brief Function that opens the vent
 *
 * @details This function uses a DC motor H bridge and GPIOs pins predefined
 */
void stop_vent();

/**@brief Function that opens the vent
 *
 * @details This function uses a DC motor H bridge and GPIOs pins predefined
 */
uint8_t get_vent_direction();

/**@brief Function that opens the vent
 *
 * @details This function uses a DC motor H bridge and GPIOs pins predefined
 */
void limit_switch_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);


#endif