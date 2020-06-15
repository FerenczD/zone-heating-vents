#ifndef TEMP_S_H
#define TEMP_S_H

/* Functions and definitions that deal directly with the i2c and temperature sensor */

/* Libraries */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrfx_twim.h"
#include "nrfx_gpiote.h"
#include "nrf_delay.h"


/* Constants */
#define SDA_PIN        03
#define SCL_PIN        04
#define ALERT_PIN      28
#define TMP116_ADDR    (0x90U >> 1)

#define TEMP_REG        0x00
#define CONFIG_REG      0x01

#define CONFIG_VALUE    0x062C

#define ONE_SHOT_CONFIG 0x0E24

/*Initialize I2C instance and inititalize sensor with set config  */ 
void temp_sensor_init(float* current_temp);

/* Init i2c. Param is for i2c handler context */
void twim_init(float* temp_ptr);

/*  Init sensor with set config */
void setup_temp_sensor();

/* Read temperature from sensor. */
void read_sensor_data();

void set_one_shot_temp();


#endif
