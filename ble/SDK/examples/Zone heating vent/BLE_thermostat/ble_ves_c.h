#ifndef BLE_VES_C_H
#define BLE_VES_C_H

/* Custom service for central device for capstone project VES = Vent Service */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_gatt.h"
#include "ble_gattc.h"

#include "ble_db_discovery.h"
#include "ble_srv_common.h"
#include "nrf_ble_gq.h"
#include "nrf_sdh_ble.h"

#include "sdk_config.h"
#include "sdk_common.h"
#include "app_error.h"
#include "nrf_log.h"
  
/* randomly generated UUID b24cc5da-5d08-11ea-bc55-0242ac130003 */
/* Inverted because SDK uses small-endian */
#define VENT_SERVICE_UUID_BASE         { 0x03, 0x00, 0x13, 0xac, 0x42, 0x02, \
                                         0x55, 0xbc, 0xea, 0x11, 0x08, 0x5d, \
                                         0xda, 0xc5, 0x4c, 0xb2 }

/* Values to be inserted into the base UUID */
#define VENT_SERVICE_UUID                 0x1400
#define DC_MOTOR_CHAR_UUID                0x1401
#define TEMP_CHAR_UUID                    0x1402
#define VENT_STATUS_UUID                  0x1403

/* Vent status codes */
#define STATUS_VENT_ERROR               0
#define STATUS_VENT_OK                  1                                       
#define STATUS_VENT_OPENING             2                                      
#define STATUS_VENT_CLOSING             3                                       
#define STATUS_VENT_OPENED              4                                       
#define STATUS_VENT_CLOSED              5      

/**@brief   Macro for defining a ble_nus_c instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */

#define BLE_VES_C_DEF(_name)                                       \
static ble_ves_c_t _name;                                          \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                \
                     BLE_HRS_BLE_OBSERVER_PRIO,                    \   
                     ble_ves_c_on_ble_evt, &_name)



/**@brief VES Client event type. */
typedef enum
{
    BLE_VES_C_EVT_DISCOVERY_COMPLETE,     /**< Event indicating that the VES service and its characteristics were found. */
    BLE_VES_C_EVT_VES_TEMP_EVT,           /**< Event indicating that the central received something from a peer. */
    BLE_VES_C_EVT_VES_STATUS_EVT,
    BLE_VES_C_EVT_DISCONNECTED            /**< Event indicating that the VES server disconnected. */
}ble_ves_c_evt_type_t;

/**@brief Handles on the connected peer device needed to interact with it. */
typedef struct
{
    uint16_t status_cccd_handle;      /* Handle for CCCD of vent status       */
    uint16_t temp_cccd_handle;        /* Handle for CCCD of temperature       */
    uint16_t dc_motor_handle;         /* Handle for DC motor chracteristic    */
    uint16_t temp_handle;             /* Handle for temperature chracteristic */
    uint16_t status_handle;           /* Handle for status chracteristic      */
} ble_ves_c_handles_t;

/**@brief Structure containing the VES event data received from the peer. */
typedef struct
{
    ble_ves_c_evt_type_t evt_type;
    uint16_t             conn_handle;
    uint16_t             data_len;
    uint8_t*             current_temp;
    uint8_t*             vent_status;
    ble_ves_c_handles_t  handles     /**< Handles on which the Vent service characteristics were discovered on the peer device.*/
}ble_ves_c_evt_t;

// Forward declaration of the ble_ves_t type.
typedef struct ble_ves_c_s ble_ves_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that is to be provided by the application
 *          of this module to receive events.
 */
typedef void (* ble_ves_c_evt_handler_t)(ble_ves_c_t * p_ble_ves_c, ble_ves_c_evt_t const * p_evt);

/**@brief VES Client structure. */
typedef struct ble_ves_c_s
{
    uint8_t                   uuid_type;      /**< UUID type. */
    uint16_t                  conn_handle;    /**< Handle of the current connection. Set with @ref ble_nus_c_handles_assign when connected. */
    ble_ves_c_handles_t       handles;        /**< Handles on the connected peer device needed to interact with it. */
    ble_ves_c_evt_handler_t   evt_handler;    /**< Application event handler to be called when there is an event related to the NUS. */
    ble_srv_error_handler_t   error_handler;  /**< Function to be called in case of an error. */
    nrf_ble_gq_t            * p_gatt_queue;   /**< Pointer to BLE GATT Queue instance. */
}ble_ves_c_t;

/**@brief VES Client initialization structure. */
typedef struct
{
    ble_ves_c_evt_handler_t   evt_handler;    /**< Application event handler to be called when there is an event related to the VES. */
    ble_srv_error_handler_t   error_handler;  /**< Function to be called in case of an error. */
    nrf_ble_gq_t            * p_gatt_queue;   /**< Pointer to BLE GATT Queue instance. */
} ble_ves_c_init_t;

/**@brief     Function for initializing the Vent service client module.
 *
 * @details   This function registers with the Database Discovery module
 *            for the VES. The Database Discovery module looks for the presence
 *            of a VES instance at the peer when a discovery is started.
 *            
 * @param[in] p_ble_ves_c      Pointer to the VES client structure.
 * @param[in] p_ble_ves_c_init Pointer to the VES initialization structure that contains the
 *                             initialization information.
 *
 * @retval    NRF_SUCCESS If the module was initialized successfully.
 * @retval    err_code    Otherwise, this function propagates the error code
 *                        returned by the Database Discovery module API
 *                        @ref ble_db_discovery_evt_register.
 */
uint32_t ble_ves_c_init(ble_ves_c_t * p_ble_ves_c, ble_ves_c_init_t * p_ble_ves_c_init);

/**@brief Function for handling events from the Database Discovery module.
 *
 * @details This function handles an event from the Database Discovery module, and determines
 *          whether it relates to the discovery of VES at the peer. If it does, the function
 *          calls the application's event handler to indicate that NUS was
 *          discovered at the peer. The function also populates the event with service-related
 *          information before providing it to the application.
 *
 * @param[in] p_ble_ves_c Pointer to the NUS client structure.
 * @param[in] p_evt       Pointer to the event received from the Database Discovery module.
 */
 void ble_ves_c_on_db_disc_evt(ble_ves_c_t * p_ble_ves_c, ble_db_discovery_evt_t * p_evt);

 /**@brief     Function for handling BLE events from the SoftDevice.
 *
 * @details   This function handles the BLE events received from the SoftDevice. If a BLE
 *            event is relevant to the VES module, the function uses the event's data to update
 *            internal variables and, if necessary, send events to the application.
 *
 * @param[in] p_ble_evt     Pointer to the BLE event.
 * @param[in] p_context     Pointer to the NUS client structure.
 */
void ble_ves_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

/**@brief   Function for requesting the peer to start sending notification of characteristics.
 *
 * @details This function enables notifications of the VES characteristic at the peer
 *          by writing to the CCCD of the characteristic.
 *
 * @param   p_ble_ves_c Pointer to the NUS client structure.
 *
 * @retval  NRF_SUCCESS If the operation was successful. 
 * @retval  err_code 	Otherwise, this API propagates the error code returned by function @ref nrf_ble_gq_item_add.
 */
uint32_t ble_ves_c_temp_notif_enable(ble_ves_c_t * p_ble_ves_c,  bool notification_enable);
uint32_t ble_ves_c_status_notif_enable(ble_ves_c_t * p_ble_ves_c, bool notification_enable);

/**@brief Function for sending a dc motor direction to the peripheral
 *
 * @details This function writes the Dc motor characteristic
 *
 * @param[in] p_ble_ves_c       Pointer to the VES client structure.
 * @param[in] motor_direction  0x01 - Open 0x02 - Close 
 *
 * @retval NRF_SUCCESS If the string was sent successfully. 
 * @retval err_code    Otherwise, this API propagates the error code returned by function @ref nrf_ble_gq_item_add.
 */
uint32_t ble_ves_c_dc_motor_send(ble_ves_c_t * p_ble_ves_c, uint8_t motor_direction);

/**@brief Function for assigning handles to this instance of ves_c.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module. The connection handle and attribute handles are
 *          provided from the discovery event @ref BLE_VES_C_EVT_DISCOVERY_COMPLETE.
 *
 * @param[in] p_ble_ves_c    Pointer to the VES client structure instance to associate with these
 *                           handles.
 * @param[in] conn_handle    Connection handle to associated with the given VES Instance.
 * @param[in] p_peer_handles Attribute handles on the NUS server that you want this VES client to
 *                           interact with.
 *
 * @retval    NRF_SUCCESS    If the operation was successful.
 * @retval    NRF_ERROR_NULL If a p_nus was a NULL pointer.
 * @retval    err_code       Otherwise, this API propagates the error code returned 
 *                           by function @ref nrf_ble_gq_item_add.
 */
uint32_t ble_ves_c_handles_assign(ble_ves_c_t *               p_ble_ves_c,
                                  uint16_t                    conn_handle,
                                  ble_ves_c_handles_t const * p_peer_handles);



#endif 