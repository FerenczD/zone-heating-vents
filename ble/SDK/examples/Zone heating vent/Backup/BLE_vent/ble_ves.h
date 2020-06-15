#ifndef BLE_VES_H
#define BLE_VES_H

/* Custom service for Capstone application. VES = VEnt Service*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ble.h"
#include "ble_srv_common.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "sdk_common.h"

#include "ble_srv_common.h"
#include "boards.h"

#include "nrf_log.h"
//#include "nrf_log_ctrl.h"
//#include "nrf_log_default_backends.h"

/* randomly generated UUID b24cc5da-5d08-11ea-bc55-0242ac130003 */
/* Inverted because SDK uses small-endian */
#define VENT_SERVICE_UUID_BASE	{0x03, 0x00, 0x13, 0xac, 0x42, 0x02, \
                                         0x55, 0xbc, 0xea, 0x11, 0x08, 0x5d, \
                                         0xda, 0xc5, 0x4c, 0xb2 }
					
/* Values to be inserted into the base UUID */
#define VENT_SERVICE_UUID                 0x1400
#define DC_MOTOR_CHAR_UUID                0x1401
#define TEMP_CHAR_UUID                    0x1402
#define VENT_STATUS_UUID                  0x1403


/* Common definitions */
#define OPTION_ENABLE                     1
#define OPTION_DISABLE                    0

/**@brief   Macro for defining a BLE_VES instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_VES_DEF(_name)                                              \
          ble_ves_t _name;                                              \   
          NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \ 
                               BLE_HRS_BLE_OBSERVER_PRIO,               \
                               ble_ves_on_ble_evt, &_name)


/*@brief Specific event types to custom service */
typedef enum
{
    BLE_VES_EVT_DISCONNECTED,
    BLE_VES_EVT_CONNECTED,
    BLE_VES_EVT_TEMP_NOTIFICATION_ENABLED,                             
    BLE_VES_EVT_TEMP_NOTIFICATION_DISABLED,     
    BLE_VES_EVT_STATUS_NOTIFICATION_ENABLED,                             
    BLE_VES_EVT_STATUS_NOTIFICATION_DISABLED,
    BLE_VES_EVT_DC,
    BLE_VES_EVT_TEMP
  
} ble_ves_evt_type_t;

/**@brief Vent Service event. */
typedef struct
{
    ble_ves_evt_type_t evt_type;                                  /**< Type of event. */
    uint8_t dc_direction;                                         /* Direction of the DC motor */
    uint16_t current_temp;
    uint8_t vent_status
} ble_ves_evt_t;

typedef struct ble_ves_s ble_ves_t;  

/**@brief Vent Service event handler type. */
typedef void (*ble_ves_evt_handler_t) (ble_ves_t * p_ves, ble_ves_evt_t * p_evt);


/**@brief Vent Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    ble_ves_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Vent Service. */
    uint8_t                       initial_custom_value;           /* Initial custom value */
    ble_srv_cccd_security_mode_t  custom_value_char_attr_md;      /* Initial security level for Custom characteristics attribute */
} ble_ves_init_t;

/**@brief Vent Service structure. This contains various status information for the service. */
typedef struct ble_ves_s
{
    ble_ves_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Vent Service. */
    uint16_t                      service_handle;                 /* Handle of Vent Service (as provided by the BLE stack). */
    uint16_t                      conn_handle;                    /* Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                       uuid_type;
    ble_gatts_char_handles_t      dc_motor_handle;                /* Handle for DC motor chracteristic */
    ble_gatts_char_handles_t      temp_handle;                    /* Handle for temperature chracteristic */
    ble_gatts_char_handles_t      status_handle;                  /* Handle for status chracteristic */
}ble_ves_t;       

/* @brief Structure to set up various characteristics with different settings each */
typedef struct ves_char_setting_s 
{
  uint8_t write_enable;
  uint8_t read_enable;
  uint8_t notify_enable;
  uint16_t uuid;
  ble_gatts_char_handles_t char_handle;
  size_t data_size;
}ves_char_setting_t;

/**@brief Function for initializing the Vent Service.
 *
 * @param[out]  p_ves       Vent Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_ves_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_ves_init(ble_ves_t * p_ves, const ble_ves_init_t * p_ves_init);

/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_ves        Vent Service structure.
 * @param[in]   p_ves_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t custom_value_char_add(ble_ves_t * p_ves, const ble_ves_init_t * p_ves_init, ves_char_setting_t char_settings, ble_gatts_char_handles_t* char_handle);


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Battery Service.
 *
 * @note 
 *
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 * @param[in]   p_context  Vent Service structure.
 */
void ble_ves_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_ves       Vent Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_ves_t * p_ves, ble_evt_t const * p_ble_evt);

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_ves       Vent Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_ves_t * p_ves, ble_evt_t const * p_ble_evt);


/**@brief Function for handling the Write event.
 *
 * @param[in]   p_ves       Vent Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_ves_t * p_ves, ble_evt_t const * p_ble_evt);

/**@brief Function for updating the temperature value
 *
 * @details The application calls this function when the cutom value should be updated. If
 *          notification has been enabled, the custom value characteristic is sent to the client.
 *
 * @note 
 *       
 * @param[in]   p_ves          Vent Service structure.
 * @param[in]   Custom value 
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_ves_temperature_update(ble_ves_t * p_ves, uint16_t temperature);

uint32_t ble_ves_status_update(ble_ves_t * p_ves, int32_t status);

#endif
