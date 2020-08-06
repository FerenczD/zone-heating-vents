/**
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

 /* Libraries */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "app_error.h"

#include "ble_db_discovery.h"
#include "app_timer.h"
#include "app_util.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_ble_gatt.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_scan.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "fds.h"
#include "nrf_fstorage.h"
#include "ble_conn_state.h"
#include "nrf_ble_lesc.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"

#include "ble_ves_c.h"
#include "uart_ble.h"
#include "cmd_queue.h"
#include "initial_boot.h"

/* Constants */
#define APP_BLE_CONN_CFG_TAG        1                                   /**< Tag that refers to the BLE stack configuration set with @ref sd_ble_cfg_set. The default tag is @ref BLE_CONN_CFG_TAG_DEFAULT. */
#define APP_BLE_OBSERVER_PRIO       3                                   /**< BLE observer priority of the application. There is no need to modify this value. */
#define APP_SOC_OBSERVER_PRIO       1                                   /**< Applications' SoC observer priority. You shouldn't need to modify this value. */

#define TES_TIMER               APP_TIMER_TICKS(3000)                   /* Notify error status timeout */

#define SEC_PARAM_BOND              1                                   /**< Perform bonding. */
#define SEC_PARAM_MITM              0                                   /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC              1                                   /**< LE Secure Connections enabled. */
#define SEC_PARAM_KEYPRESS          0                                   /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES   BLE_GAP_IO_CAPS_NONE                /**< No I/O capabilities. */
#define SEC_PARAM_OOB               0                                   /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE      7                                   /**< Minimum encryption key size in octets. */
#define SEC_PARAM_MAX_KEY_SIZE      16                                  /**< Maximum encryption key size in octets. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)          /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)          /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                    0                                        /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(5000, UNIT_10_MS)           /**< Connection supervisory timeout (4 seconds). */

#define DC_MOTOR_OPEN               0x01
#define DC_MOTOR_CLOSE              0x02

#define FACTORY_RESET                0
#define DELETE_BONDS                 1

#define CONNECTION_ERROR_TIMEOUT        APP_TIMER_TICKS(20000)

/* Modules instances */
NRF_BLE_GATT_DEF(m_gatt);                                               /**< GATT module instance. */
BLE_DB_DISCOVERY_DEF(m_db_disc);                                        /**< Database discovery module instance. */
NRF_BLE_SCAN_DEF(m_scan);                                               /**< Scanning Module instance. */
NRF_BLE_GQ_DEF(m_ble_gatt_queue,                                        /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_CENTRAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);

BLE_VES_C_DEF(m_ves_c);                                                 /* Vent service module instance */

APP_TIMER_DEF(m_toggler_timer);                                         /* Just for testing */
APP_TIMER_DEF(m_connection_error_timer_id);


/* Globals and flags */
vent_status_cmd_t m_vent_status;                                        /* Instance of vent status response structure */
uint8_t* m_vent_status_ptr = (uint8_t*)&m_vent_status;                  /* Pointer that stores vent status */

static bool     m_whitelist_disabled = false;                                   /**< True if whitelist has been temporarily disabled. */
static bool     m_memory_access_in_progress = false;                    /**< Flag to keep track of ongoing operations on persistent memory. */
static bool     m_pairing_enabled = false;                              /* True if pairing mode is enabled */
static bool     m_device_connected = false;

/* Configuration parameters */
/**< Scan parameters requested for scanning and connection with whitelist. */
static ble_gap_scan_params_t const m_scan_param =
{
    .active        = 0x01,
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,
    .filter_policy = BLE_GAP_SCAN_FP_WHITELIST,
    .timeout       = BLE_GAP_SCAN_TIMEOUT_UNLIMITED,
    .scan_phys     = BLE_GAP_PHY_1MBPS,
};

/* Connection parameters required to call nrf_ble_gap_connect() same value as vent */
static ble_gap_conn_params_t const m_conn_param = {
    .min_conn_interval = MIN_CONN_INTERVAL,
    .max_conn_interval = MAX_CONN_INTERVAL,
    .slave_latency     = SLAVE_LATENCY,
    .conn_sup_timeout  = CONN_SUP_TIMEOUT,
};



/**@brief NUS UUID. */
static ble_uuid_t const m_ves_uuid =
{
    .uuid = VENT_SERVICE_UUID,
    .type = BLE_UUID_TYPE_VENDOR_BEGIN
};


/**@brief Function for handling asserts in the SoftDevice.
 *
 * @details This function is called in case of an assert in the SoftDevice.
 *
 * @warning This handler is only an example and is not meant for the final product. You need to analyze
 *          how your product is supposed to react in case of assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing assert call.
 * @param[in] p_file_name  File name of the failing assert call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Function for handling the Vent Service client errors 
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void ves_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

//void uart_error_handle(app_uart_evt_t * p_event)
//{
//    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
//    {
//        APP_ERROR_HANDLER(p_event->data.error_communication);
//    }
//    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
//    {
//        APP_ERROR_HANDLER(p_event->data.error_code);
//    }
//}

/**@brief Function to start scanning. */
static void scan_start(void)
{
    ret_code_t ret;

    if (nrf_fstorage_is_busy(NULL))
    {
        m_memory_access_in_progress = true;
        return;
    }

    if (!m_whitelist_disabled){
        ret = nrf_ble_scan_params_set(&m_scan, &m_scan_param);
        APP_ERROR_CHECK(ret);
    }else{
        ret = nrf_ble_scan_params_set(&m_scan, NULL); /* Dont use whitelist. Only when pairing mode */
        APP_ERROR_CHECK(ret);
    }

    ret = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(ret);

    ret = bsp_indication_set(BSP_INDICATE_SCANNING);
    APP_ERROR_CHECK(ret);

    NRF_LOG_INFO("Starting BLE scan for service UUID b24cc5da-5d08-11ea-bc55-0242ac130003");
}

/**@brief Function for disabling the use of whitelist for scanning.
 */
static void whitelist_disable(void)
{
    if (!m_whitelist_disabled)
    {
        NRF_LOG_INFO("Whitelist temporarily disabled.");
        m_whitelist_disabled = true;
        nrf_ble_scan_stop();
        scan_start();
    }
}

/**@brief Function for disabling the use of whitelist for scanning.
 */
static void whitelist_enable(void)
{
    if (m_whitelist_disabled)
    {
        NRF_LOG_INFO("Whitelist enabled.");
        m_whitelist_disabled = false;
        nrf_ble_scan_stop();
        scan_start();
    }
}

/**@brief Retrieve a list of peer manager peer IDs.
 *
 * @param[inout] p_peers   The buffer where to store the list of peer IDs.
 * @param[inout] p_size    In: The size of the @p p_peers buffer.
 *                         Out: The number of peers copied in the buffer.
 */
static void peer_list_get(pm_peer_id_t * p_peers, uint32_t * p_size)
{
    pm_peer_id_t peer_id;
    uint32_t     peers_to_copy;

    peers_to_copy = (*p_size < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) ?
                     *p_size : BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    *p_size = 0;

    while ((peer_id != PM_PEER_ID_INVALID) && (peers_to_copy--))
    {
        p_peers[(*p_size)++] = peer_id;
        peer_id = pm_next_peer_id_get(peer_id);
    }
}

static void whitelist_load()
{
    ret_code_t   ret;
    pm_peer_id_t peers[8];
    uint32_t     peer_cnt;

    memset(peers, PM_PEER_ID_INVALID, sizeof(peers));
    peer_cnt = (sizeof(peers) / sizeof(pm_peer_id_t));

    // Load all peers from flash and whitelist them.
    peer_list_get(peers, &peer_cnt);

    ret = pm_whitelist_set(peers, peer_cnt);
    APP_ERROR_CHECK(ret);

    // Setup the device identies list.
    // Some SoftDevices do not support this feature.
    ret = pm_device_identities_list_set(peers, peer_cnt);
    if (ret != NRF_ERROR_NOT_SUPPORTED)
    {
        APP_ERROR_CHECK(ret);
    }
}

static int whitelist_comp(ble_gap_addr_t scanned_addr){

    ret_code_t err_code;
    int found = 0;

    // Whitelist buffers.
    ble_gap_addr_t whitelist_addrs[8];
    ble_gap_irk_t  whitelist_irks[8];

    memset(whitelist_addrs, 0x00, sizeof(whitelist_addrs));
    memset(whitelist_irks,  0x00, sizeof(whitelist_irks));

    uint32_t addr_cnt = (sizeof(whitelist_addrs) / sizeof(ble_gap_addr_t));
    uint32_t irk_cnt  = (sizeof(whitelist_irks)  / sizeof(ble_gap_irk_t));

    /* Get whitelisted devices */
    err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                                whitelist_irks,  &irk_cnt);

    /* Compare address to list */
    for (int i = 0; i < addr_cnt; i++){
        if (memcmp(&whitelist_addrs[i], &scanned_addr, BLE_ADDR_LEN) == 0){
          found = 1;
        }
    }
    
    return found;
}


static void on_whitelist_req(void)
{
    ret_code_t err_code;

    // Whitelist buffers.
    ble_gap_addr_t whitelist_addrs[8];
    ble_gap_irk_t  whitelist_irks[8];

    memset(whitelist_addrs, 0x00, sizeof(whitelist_addrs));
    memset(whitelist_irks,  0x00, sizeof(whitelist_irks));

    uint32_t addr_cnt = (sizeof(whitelist_addrs) / sizeof(ble_gap_addr_t));
    uint32_t irk_cnt  = (sizeof(whitelist_irks)  / sizeof(ble_gap_irk_t));

    // Reload the whitelist and whitelist all peers.
    whitelist_load();

    // Get the whitelist previously set using pm_whitelist_set().
    err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                                whitelist_irks,  &irk_cnt);

    if (((addr_cnt == 0) && (irk_cnt == 0)) ||
        (m_whitelist_disabled))
    {
        // Don't use whitelist if its empty. It breaks the scan otherwise
        NRF_LOG_INFO("NORMAL SCAN");
        err_code = nrf_ble_scan_params_set(&m_scan, NULL);
        APP_ERROR_CHECK(err_code);
    }
}



/**@brief Function for handling Scanning Module events.
 */
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
    ret_code_t err_code;

    switch(p_scan_evt->scan_evt_id)
    {
         case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
         {
              err_code = p_scan_evt->params.connecting_err.err_code;
              NRF_LOG_INFO("NRF_BLE_SCAN_EVT_CONNECTING_ERROR");
              APP_ERROR_CHECK(err_code);
         } break;

         case NRF_BLE_SCAN_EVT_CONNECTED:
         {
              ble_gap_evt_connected_t const * p_connected =
                               p_scan_evt->params.connected.p_connected;
             // Scan is automatically stopped by the connection.
             NRF_LOG_INFO("Connecting to target %02x%02x%02x%02x%02x%02x",
                      p_connected->peer_addr.addr[0],
                      p_connected->peer_addr.addr[1],
                      p_connected->peer_addr.addr[2],
                      p_connected->peer_addr.addr[3],
                      p_connected->peer_addr.addr[4],
                      p_connected->peer_addr.addr[5]
                      );

             /* Store vent ble addr on sturcture for later use */
             memcpy(&m_vent_status.ble_addr, &p_connected->peer_addr.addr, BLE_ADDR_LEN);
             

         } break;

         case NRF_BLE_SCAN_EVT_FILTER_MATCH:
         {
            NRF_LOG_INFO("Filter matched, checking if device is bonded...");
            ble_gap_evt_adv_report_t const* p_adv_report = p_scan_evt->params.filter_match.p_adv_report;

            /* When in paring mode */
            /* If peer address is not in whitelist then add it and connect to it */
            if(whitelist_comp(p_adv_report->peer_addr) != 1){
                NRF_LOG_INFO("Device not-bonded. Connecting.");
                
                err_code = sd_ble_gap_connect(&p_adv_report->peer_addr, p_scan_evt->p_scan_params, &m_conn_param, APP_BLE_CONN_CFG_TAG);
                APP_ERROR_CHECK(err_code);
            }else{
                NRF_LOG_INFO("Device bonded already. Skipping it");
            }

         } break;
         case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
         {
             NRF_LOG_INFO("Scan timed out.");
             scan_start();
         } break;

         case NRF_BLE_SCAN_EVT_WHITELIST_REQUEST:
          {
              NRF_LOG_INFO("Whitelist request ");

              on_whitelist_req();
              m_whitelist_disabled = false;
          } break;

         case NRF_BLE_SCAN_EVT_WHITELIST_ADV_REPORT:
              NRF_LOG_INFO("Whitelisted peripheral found");
              ble_gap_evt_adv_report_t const* p_whitelist_adv = p_scan_evt->params.p_whitelist_adv_report;

              err_code = sd_ble_gap_connect(&p_whitelist_adv->peer_addr, p_scan_evt->p_scan_params, &m_conn_param, APP_BLE_CONN_CFG_TAG);
              APP_ERROR_CHECK(err_code);

              break;

         default:
              break;
    }
}


/**@brief Function for initializing the scanning and setting the filters.
 */
static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;

    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.p_scan_param     = &m_scan_param;
    init_scan.connect_if_match = false; 
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_UUID_FILTER, &m_ves_uuid); /* Just scan for Vent service UUID */
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_ALL_FILTER, false);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling database discovery events.
 *
 * @details This function is a callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function forwards the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    ble_ves_c_on_db_disc_evt(&m_ves_c, p_evt);
}

/**@brief   Function for handling app_uart events.
 *
 * @details This function receives a single character from the app_uart module and appends it to
 *          a string. The string is sent over BLE when the last character received is a
 *          'new line' '\n' (hex 0x0A) or if the string reaches the maximum data length.
 */
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint16_t index = 0;
    uint32_t ret_val;

    static vent_cmd_t m_vent_cmd = {0};                                  /* Instance of vent command structure */
    static uint8_t* m_vent_cmd_ptr = (uint8_t*)&m_vent_cmd;        /* Pointer that stores vent command */


    switch (p_event->evt_type)
    {
        /**@snippet [Handling data from UART] */
        case APP_UART_DATA_READY:
            uart_get(&m_vent_cmd_ptr[index]);
            index++;

            nrf_gpio_pin_set(BSP_LED_3);
            if (index >= sizeof(vent_cmd_t)){

                NRF_LOG_INFO("UART data received: "); 
                NRF_LOG_HEXDUMP_INFO(m_vent_cmd_ptr, sizeof(vent_cmd_t));
                
                /* Store command into hash table if its not pairing mode */
                vent_cmd_t cmd_store;
                memcpy(&cmd_store, m_vent_cmd_ptr, sizeof(vent_cmd_t));
                
                if(cmd_store.mode == PAIRING_MODE){ /* Dont care whats the command if its the first time connecting */
                    
//                    uint8_t uartReceivedMsg = 0x01;
//                    uart_send(&uartReceivedMsg, 1);

                    /* Make sure device is not connected */
                    if(m_device_connected){
                          ret_val = sd_ble_gap_disconnect(m_ves_c.conn_handle,
                                                          BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);

                          while(m_device_connected);  /* Wait for device to finnish connection */
      
                    }


                    NRF_LOG_INFO("PAIRING MODE");
                    m_pairing_enabled = true;

                    whitelist_disable();
                }else{
                    NRF_LOG_INFO("ADDING COMMAND");

                    cmd_add(cmd_store.ble_addr, cmd_store);   

                    NRF_LOG_INFO("COMMAND ADDED");

                }

                index = 0;

                nrf_gpio_pin_clear(BSP_LED_3);

                /* Error check and tell ESP a message status i.e error or ok */

            }
            break;

        /**@snippet [Handling data from UART] */
        case APP_UART_COMMUNICATION_ERROR:
            NRF_LOG_ERROR("Communication error occurred while handling UART.");
//            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            NRF_LOG_ERROR("Error occurred in FIFO module used by UART.");
//            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}


/**@brief Callback handling Vent Service client events 
 *
 * @details This function is called to notify the application of a VES event
 *
 * @param[in]   p_ble_ves_c   VES client handle. This identifies the VES client.
 * @param[in]   p_ble_ves_evt Pointer to the VES client event.
 */

/**@snippet [Handling events from the ble_ves_c module] */
static void ble_ves_c_evt_handler(ble_ves_c_t * p_ble_ves_c, ble_ves_c_evt_t const * p_ble_ves_evt)
{
    ret_code_t err_code;

    switch (p_ble_ves_evt->evt_type)
    {
        case BLE_VES_C_EVT_DISCOVERY_COMPLETE:
            NRF_LOG_INFO("Discovery complete.");
            err_code = ble_ves_c_handles_assign(p_ble_ves_c, p_ble_ves_evt->conn_handle, &p_ble_ves_evt->handles);
            APP_ERROR_CHECK(err_code);

            // Initiate bonding.
            err_code = pm_conn_secure(p_ble_ves_c->conn_handle, false);
            APP_ERROR_CHECK(err_code);

            err_code = ble_ves_c_temp_notif_enable(p_ble_ves_c, true); 
            APP_ERROR_CHECK(err_code);

            err_code = ble_ves_c_status_notif_enable(p_ble_ves_c, true); 
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("Connected to device with Vent Service (VES)");

//            app_timer_start( m_toggler_timer,TES_TIMER, NULL);

  
            if(m_pairing_enabled){  /* At thi point vent is not opening. I think i because it cant until it bonds. ref BLE_GAP_EVT_AUTH_STATUS */
                NRF_LOG_INFO("Vent paired. Opening vent");
                ble_ves_c_dc_motor_send(&m_ves_c, DC_MOTOR_OPEN);  /* Open as initital position */
            }else{
                        /* Check if there is a command */
                nlist* stored_cmd = cmd_lookup(m_vent_status.ble_addr);

                if (stored_cmd == NULL){
                    //m_vent_status.is_cmd = false;             /* No command. Just receive temperature of vent */
                    ble_ves_c_dc_motor_send(&m_ves_c, 0x00);    /* Stop the motor just in case */
                }else{
                    //m_vent_status.is_cmd = true;
                    //memcpy(m_vent_status.cmd_uuid, stored_cmd->data.cmd_uuid, CMD_UUID_LEN);

                    NRF_LOG_INFO("Command to the vent : %s",  stored_cmd->data.cmd == 0x01 ? "OPEN" : "CLOSE" );

                    ble_ves_c_dc_motor_send(&m_ves_c, stored_cmd->data.cmd);  /* Open or close command give*/

                    //delete_cmd(stored_cmd);
                }
            
            }


            break;

        case BLE_VES_C_EVT_VES_TEMP_EVT:
            
            /* Do something with this temperature value i.e put it in a message structure to send through UART */
            memcpy(&m_vent_status.vent_temp, p_ble_ves_evt->current_temp, p_ble_ves_evt->data_len);
            NRF_LOG_HEXDUMP_INFO( m_vent_status.vent_temp,sizeof(m_vent_status.vent_temp));

            NRF_LOG_INFO("Current room temperature: %d", *(uint16_t*)m_vent_status.vent_temp );

            break;

        case BLE_VES_C_EVT_VES_STATUS_EVT:

            /* Do something with this status i.e put it in a message structure to send through UART */
            if ((*p_ble_ves_evt->vent_status == STATUS_VENT_ERROR)   ||
                (*p_ble_ves_evt->vent_status == STATUS_VENT_OPENED)  ||
                (*p_ble_ves_evt->vent_status == STATUS_VENT_CLOSED)) {

                    /* Make sure temperature is valid */
                    if(m_vent_status.vent_temp[0] != 0 && m_vent_status.vent_temp[1] != 0) {
                      
                        if(*p_ble_ves_evt->vent_status == 0x00){
                            NRF_LOG_INFO("Vent sent error status code 0x00");
                        }else{
                            NRF_LOG_INFO("Vent is now: %s", (*p_ble_ves_evt->vent_status == 0x01) ? "OPENED" : "CLOSED");
                        }

                        memcpy(&m_vent_status.vent_status, p_ble_ves_evt->vent_status, p_ble_ves_evt->data_len);

                        /* Prepare message pointer */
                        memcpy(m_vent_status_ptr, &m_vent_status, sizeof(m_vent_status));
                        NRF_LOG_HEXDUMP_INFO(m_vent_status_ptr, sizeof(m_vent_status));

                        /* Send vent status to ESP */
                        nrf_gpio_pin_toggle(BSP_LED_2);
                        uart_send(m_vent_status_ptr, sizeof(m_vent_status));
                        nrf_delay_ms(150);
                        nrf_gpio_pin_toggle(BSP_LED_2);

//                        app_timer_stop(m_toggler_timer);  /* Teting timer */

                        /* Disconnect vent */
                        NRF_LOG_INFO("Disconnecting vent");
                        err_code = sd_ble_gap_disconnect(m_ves_c.conn_handle,
                                                          BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                        /* Clear structure */
                        memset(&m_vent_status, 0, sizeof(m_vent_status));
                   
                    }
                }

            break;

        case BLE_VES_C_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected. Re-starting scan ");
            
//            if(m_pairing_enabled){
//              m_pairing_enabled = false;
//              whitelist_enable();
//            }else{
//              scan_start(); /* Implied whitelisting scan */
//            }

            break;
    }
}

/**@snippet [Handling events from the ble_ves_c module] */


/**
 * @brief Function for handling shutdown events.
 *
 * @param[in]   event       Shutdown type.
 */
static bool shutdown_handler(nrf_pwr_mgmt_evt_t event)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    switch (event)
    {
        case NRF_PWR_MGMT_EVT_PREPARE_WAKEUP:
            // Prepare wakeup buttons.
//            err_code = bsp_btn_ble_sleep_mode_prepare();
//            APP_ERROR_CHECK(err_code);

            /* Don't need it to go to sleep */
            break;

        default:
            break;
    }

    return true;
}

NRF_PWR_MGMT_HANDLER_REGISTER(shutdown_handler, APP_SHUTDOWN_HANDLER_PRIORITY);


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t            err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:

            m_device_connected = true;
            app_timer_start( m_connection_error_timer_id, CONNECTION_ERROR_TIMEOUT, NULL);

            err_code = ble_ves_c_handles_assign(&m_ves_c, p_ble_evt->evt.gap_evt.conn_handle, NULL);
            APP_ERROR_CHECK(err_code);

            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);

            // start discovery of services. The VES Client waits for a discovery result
            err_code = ble_db_discovery_start(&m_db_disc, p_ble_evt->evt.gap_evt.conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:

            NRF_LOG_INFO("Disconnected. conn_handle: 0x%x, reason: 0x%x",
                         p_gap_evt->conn_handle,
                         p_gap_evt->params.disconnected.reason);

            app_timer_stop(m_connection_error_timer_id);
            if(m_pairing_enabled){
              m_pairing_enabled = false;
              whitelist_enable();
            }else{
              scan_start(); /* Implied whitelisting scan */
            }

            m_device_connected = false;
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                NRF_LOG_INFO("Connection Request timed out.");
            }
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
            // Accepting parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                                    &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_INFO("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_KEY_REQUEST");
            break;

        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_LESC_DHKEY_REQUEST");
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d kdist_own:0x%x kdist_peer:0x%x",
                          p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
                          p_ble_evt->evt.gap_evt.params.auth_status.bonded,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));

            /************* Vent has to be open here when the authentication is completed ******************/
            if(m_pairing_enabled){
                NRF_LOG_INFO("Vent paired. Opening vent");
                ble_ves_c_dc_motor_send(&m_ves_c, DC_MOTOR_OPEN);  /* Open as initital position */
            }

            break;

        default:
            break;
    }
}

/** @brief Clear bonding information from persistent storage
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            // Bonds are deleted. Start scanning.
            //scan_start();
            break;
        case PM_EVT_CONN_SEC_CONFIG_REQ: ;

            pm_conn_sec_config_t config;
            pm_conn_sec_config_reply(p_evt->conn_handle, &config);

            break;
        default:
            break;
    }
}

/**@brief SoftDevice SoC event handler.
 *
 * @param[in]   evt_id      SoC event.
 * @param[in]   p_context   Context.
 */
static void soc_evt_handler(uint32_t evt_id, void * p_context)
{
    switch (evt_id)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            /* fall through */
        case NRF_EVT_FLASH_OPERATION_ERROR:

            if (m_memory_access_in_progress)
            {
                m_memory_access_in_progress = false;
                scan_start();
            }
            break;

        default:
            // No implementation needed.
            break;
    } 
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events and memory events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
    NRF_SDH_SOC_OBSERVER(m_soc_observer, APP_SOC_OBSERVER_PRIO, soc_evt_handler, NULL);

}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    switch (p_evt->evt_id)
    {
        case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED:
        {
            NRF_LOG_INFO("GATT ATT MTU on connection 0x%x changed to %d.",
                         p_evt->conn_handle,
                         p_evt->params.att_mtu_effective);
        } break;

        case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED:
        {
            NRF_LOG_INFO("Data length for connection 0x%x updated to %d.",
                         p_evt->conn_handle,
                         p_evt->params.data_length);
        } break;

        default:
            break;
    }
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

//    err_code = nrf_ble_gatt_att_mtu_central_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
//    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in] event  Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
        case BSP_EVENT_SLEEP:
            NRF_LOG_INFO("Shutdown mode called");
            nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_ves_c.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        default:
            break;
    }
}


/**@brief Function for initializing the Vent service client. */
static void ves_c_init(void)
{
    ret_code_t       err_code;
    ble_ves_c_init_t init;

    init.evt_handler   = ble_ves_c_evt_handler;
    init.error_handler = ves_error_handler;
    init.p_gatt_queue  = &m_ble_gatt_queue;

    err_code = ble_ves_c_init(&m_ves_c, &init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing buttons and leds. */
static void buttons_leds_init(void)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LEDS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);
}

static void toggle_timeout_handler(void * p_context){

    static bool flag = true;

    UNUSED_PARAMETER(p_context);
    ret_code_t err_code;

    if (flag == true){
        ble_ves_c_dc_motor_send(&m_ves_c, 0x01);
        flag = false;
    }else{
        ble_ves_c_dc_motor_send(&m_ves_c, 0x02);
        flag = true;
    }
}

static void connection_error_timeout_handler(void* p_context){
    UNUSED_PARAMETER(p_context);
    ret_code_t err_code;

    NRF_LOG_INFO("Error stablishing connection with vent. Disconnecting");
    err_code = sd_ble_gap_disconnect(m_ves_c.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);    
    //APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the timer. */
static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_toggler_timer, APP_TIMER_MODE_REPEATED, toggle_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_connection_error_timer_id, APP_TIMER_MODE_SINGLE_SHOT, connection_error_timeout_handler);
    APP_ERROR_CHECK(err_code);

}


/**@brief Function for initializing the nrf log module. */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/** @brief Function for initializing the database discovery module. */
static void db_discovery_init(void)
{
    ble_db_discovery_init_t db_init;

    memset(&db_init, 0, sizeof(ble_db_discovery_init_t));

    db_init.evt_handler  = db_disc_handler;
    db_init.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_db_discovery_init(&db_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details Handles any pending log operations, then sleeps until the next event occurs.
 */
static void idle_state_handle(void)
{
    ret_code_t err_code;
    
    err_code = nrf_ble_lesc_request_handler();
    APP_ERROR_CHECK(err_code);
    
    NRF_LOG_FLUSH();
    nrf_pwr_mgmt_run();
}

int main(void)
{
   
    // Initialize.
    log_init();
    timer_init();
    uart_init(uart_event_handle);

    buttons_leds_init();
    nrf_gpio_pin_clear(BSP_LED_2);
    nrf_gpio_pin_clear(BSP_LED_1);

    peer_manager_init();
    db_discovery_init();
    power_management_init();
    ble_stack_init();
    gatt_init();
    ves_c_init();
    scan_init();

    NRF_LOG_INFO("Venti thermostat program intitalized correctly");
    NRF_LOG_FLUSH();

    while(!m_pairing_enabled){
        nrf_pwr_mgmt_run();
    }

    if(DELETE_BONDS == 1){
        delete_bonds();
        whitelist_disable();
    }else{
        scan_start();
    }



 /************ Test **************/
//    if(FACTORY_RESET == 1){
//        factory_reset();
//        delete_bonds();
//    }
//
//    /* At first boot up wait for pairing signal. This flag should be stored in flash */
//    if(initial_boot() == 1){
//        NRF_LOG_INFO("First boot of the program. Waiting for pairing ignal from ESP");
//
//        while(!m_pairing_enabled){
//            nrf_pwr_mgmt_run();
//        }
//
//        whitelist_disable();
//    }else{
//        /* Then start execution */
//        while(!m_pairing_enabled){
//            nrf_pwr_mgmt_run();
//        }
//
//        whitelist_disable();
//        //scan_start();
//    }

/***********************************/

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }

}
