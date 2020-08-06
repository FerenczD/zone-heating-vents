#include "ble_ves_c.h"

/**@brief Function for intercepting the errors of GATTC and the BLE GATT Queue.
 *
 * @param[in] nrf_error   Error code.
 * @param[in] p_ctx       Parameter from the event handler.
 * @param[in] conn_handle Connection handle.
 */
static void gatt_error_handler(uint32_t   nrf_error,
                               void     * p_ctx,
                               uint16_t   conn_handle)
{
    ble_ves_c_t * p_ble_ves_c = (ble_ves_c_t *)p_ctx;

    NRF_LOG_DEBUG("A GATT Client error has occurred on conn_handle: 0X%X", conn_handle);

    if (p_ble_ves_c->error_handler != NULL)
    {
        p_ble_ves_c->error_handler(nrf_error);
    }
}

void ble_ves_c_on_db_disc_evt(ble_ves_c_t * p_ble_ves_c, ble_db_discovery_evt_t * p_evt)
{
    ble_ves_c_evt_t ves_c_evt;
    memset(&ves_c_evt,0,sizeof(ble_ves_c_evt_t));

    ble_gatt_db_char_t * p_chars = p_evt->params.discovered_db.charateristics;

    // Check if the VES was discovered.
    if (    (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE)
        &&  (p_evt->params.discovered_db.srv_uuid.uuid == VENT_SERVICE_UUID)
        &&  (p_evt->params.discovered_db.srv_uuid.type == p_ble_ves_c->uuid_type))
    {
        /* Store service characteristics */
        for (uint32_t i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            switch (p_chars[i].characteristic.uuid.uuid)
            {
                case DC_MOTOR_CHAR_UUID:
                    ves_c_evt.handles.dc_motor_handle = p_chars[i].characteristic.handle_value;
                    break;

                case TEMP_CHAR_UUID:
                    ves_c_evt.handles.temp_handle = p_chars[i].characteristic.handle_value;
                    ves_c_evt.handles.temp_cccd_handle = p_chars[i].cccd_handle;
                    break;

                case VENT_STATUS_UUID:
                    ves_c_evt.handles.status_handle = p_chars[i].characteristic.handle_value;
                    ves_c_evt.handles.status_cccd_handle = p_chars[i].cccd_handle;
                    break;

                default:
                    break;
            }
        }
        if (p_ble_ves_c->evt_handler != NULL)
        {
            ves_c_evt.conn_handle = p_evt->conn_handle;
            ves_c_evt.evt_type    = BLE_VES_C_EVT_DISCOVERY_COMPLETE;
            p_ble_ves_c->evt_handler(p_ble_ves_c, &ves_c_evt);
        }
    }
}

/**@brief     Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details   This function uses the Handle Value Notification received from the SoftDevice
 *            and checks if it is a notification of the VES characteristics from the peer.
 *            If it is, this function decodes the data and sends it to the application.
 *            
 * @param[in] p_ble_ves_c Pointer to the VES Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_hvx(ble_ves_c_t * p_ble_ves_c, ble_evt_t const * p_ble_evt)
{
    /* HVX can only occur when a notification from peripheral occurs */
    ble_ves_c_evt_t ble_ves_c_evt;

    if (p_ble_ves_c->evt_handler != NULL){

      /* Check wich characteristic notified */
      if((p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_ves_c->handles.temp_handle) && (p_ble_ves_c->handles.temp_handle != BLE_GATT_HANDLE_INVALID)){
        ble_ves_c_evt.evt_type        = BLE_VES_C_EVT_VES_TEMP_EVT;
        ble_ves_c_evt.current_temp    = (uint8_t*)p_ble_evt->evt.gattc_evt.params.hvx.data; /* It must be uint8_t* thus always keep data length*/
        ble_ves_c_evt.data_len        = p_ble_evt->evt.gattc_evt.params.hvx.len;
        NRF_LOG_DEBUG("Client sending current temperature.");

      }else if((p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_ves_c->handles.status_handle) && (p_ble_ves_c->handles.status_handle != BLE_GATT_HANDLE_INVALID)){
        ble_ves_c_evt.evt_type        = BLE_VES_C_EVT_VES_STATUS_EVT;
        ble_ves_c_evt.vent_status     = (uint8_t*)p_ble_evt->evt.gattc_evt.params.hvx.data; /* It must be uint8_t* thus always keep data length*/
        ble_ves_c_evt.data_len        = p_ble_evt->evt.gattc_evt.params.hvx.len;
        NRF_LOG_DEBUG("Client sending current vent status.");
      }

      p_ble_ves_c->evt_handler(p_ble_ves_c, &ble_ves_c_evt);

    }

}

uint32_t ble_ves_c_init(ble_ves_c_t * p_ble_ves_c, ble_ves_c_init_t * p_ble_ves_c_init){
    uint32_t      err_code;
    ble_uuid_t    vent_uuid;
    ble_uuid128_t ves_base_uuid = VENT_SERVICE_UUID_BASE;

    VERIFY_PARAM_NOT_NULL(p_ble_ves_c);
    VERIFY_PARAM_NOT_NULL(p_ble_ves_c_init);
    VERIFY_PARAM_NOT_NULL(p_ble_ves_c_init->p_gatt_queue);

    err_code = sd_ble_uuid_vs_add(&ves_base_uuid, &p_ble_ves_c->uuid_type);
    VERIFY_SUCCESS(err_code);

    vent_uuid.type = p_ble_ves_c->uuid_type;
    vent_uuid.uuid = VENT_SERVICE_UUID;

    p_ble_ves_c->conn_handle             = BLE_CONN_HANDLE_INVALID;
    p_ble_ves_c->evt_handler             = p_ble_ves_c_init->evt_handler;
    p_ble_ves_c->error_handler           = p_ble_ves_c_init->error_handler;
    p_ble_ves_c->handles.temp_handle     = BLE_GATT_HANDLE_INVALID;
    p_ble_ves_c->handles.status_handle   = BLE_GATT_HANDLE_INVALID;
    p_ble_ves_c->handles.dc_motor_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_ves_c->p_gatt_queue            = p_ble_ves_c_init->p_gatt_queue;

    return ble_db_discovery_evt_register(&vent_uuid);
}

void ble_ves_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context){
    ble_ves_c_t * p_ble_ves_c = (ble_ves_c_t *)p_context;

    if ((p_ble_ves_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    if ( (p_ble_ves_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(p_ble_ves_c->conn_handle != p_ble_evt->evt.gap_evt.conn_handle)
       )
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_ble_ves_c, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            if (p_ble_evt->evt.gap_evt.conn_handle == p_ble_ves_c->conn_handle
                    && p_ble_ves_c->evt_handler != NULL)
            {
                ble_ves_c_evt_t ves_c_evt;

                ves_c_evt.evt_type = BLE_VES_C_EVT_DISCONNECTED;

                p_ble_ves_c->conn_handle = BLE_CONN_HANDLE_INVALID;
                p_ble_ves_c->evt_handler(p_ble_ves_c, &ves_c_evt);
            }
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for creating a message for writing to the CCCD. */
static uint32_t cccd_configure(ble_ves_c_t * p_ble_ves_c, uint16_t handle ,bool notification_enable)
{
    nrf_ble_gq_req_t cccd_req;
    uint8_t          cccd[BLE_CCCD_VALUE_LEN];
    uint16_t         cccd_val = notification_enable ? BLE_GATT_HVX_NOTIFICATION : 0;

    memset(&cccd_req, 0, sizeof(nrf_ble_gq_req_t));

    cccd[0] = LSB_16(cccd_val);
    cccd[1] = MSB_16(cccd_val);

    cccd_req.type                        = NRF_BLE_GQ_REQ_GATTC_WRITE;
    cccd_req.error_handler.cb            = gatt_error_handler;
    cccd_req.error_handler.p_ctx         = p_ble_ves_c;
    cccd_req.params.gattc_write.handle   = handle; 
    cccd_req.params.gattc_write.len      = BLE_CCCD_VALUE_LEN;
    cccd_req.params.gattc_write.offset   = 0;
    cccd_req.params.gattc_write.p_value  = cccd;
    cccd_req.params.gattc_write.write_op = BLE_GATT_OP_WRITE_REQ;
    cccd_req.params.gattc_write.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;

    return nrf_ble_gq_item_add(p_ble_ves_c->p_gatt_queue, &cccd_req, p_ble_ves_c->conn_handle);
}

uint32_t ble_ves_c_temp_notif_enable(ble_ves_c_t * p_ble_ves_c, bool notification_enable)
{
    VERIFY_PARAM_NOT_NULL(p_ble_ves_c);

    if ( (p_ble_ves_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(p_ble_ves_c->handles.temp_cccd_handle == BLE_GATT_HANDLE_INVALID)
       )
    {
        return NRF_ERROR_INVALID_STATE;
    }
    return cccd_configure(p_ble_ves_c, p_ble_ves_c->handles.temp_cccd_handle, notification_enable); /* tambien manda que tipo de handler es e.g temp or status */
}

uint32_t ble_ves_c_status_notif_enable(ble_ves_c_t * p_ble_ves_c, bool notification_enable )
{
    VERIFY_PARAM_NOT_NULL(p_ble_ves_c);

    if ( (p_ble_ves_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(p_ble_ves_c->handles.status_cccd_handle == BLE_GATT_HANDLE_INVALID)
       )
    {
        return NRF_ERROR_INVALID_STATE;
    }
    return cccd_configure(p_ble_ves_c, p_ble_ves_c->handles.status_cccd_handle, notification_enable); /* tambien manda que tipo de handler es e.g temp or status */
}

uint32_t ble_ves_c_dc_motor_send(ble_ves_c_t * p_ble_ves_c, uint8_t motor_direction) {
    VERIFY_PARAM_NOT_NULL(p_ble_ves_c);

    nrf_ble_gq_req_t write_req;
    memset(&write_req, 0, sizeof(nrf_ble_gq_req_t));

    if (p_ble_ves_c->conn_handle == BLE_CONN_HANDLE_INVALID){
        NRF_LOG_WARNING("Connection handle invalid.");
        return NRF_ERROR_INVALID_STATE;
    }

    write_req.type                        = NRF_BLE_GQ_REQ_GATTC_WRITE;
    write_req.error_handler.cb            = gatt_error_handler;
    write_req.error_handler.p_ctx         = p_ble_ves_c;
    write_req.params.gattc_write.handle   = p_ble_ves_c->handles.dc_motor_handle;
    write_req.params.gattc_write.len      = sizeof(uint8_t);
    write_req.params.gattc_write.offset   = 0;
    write_req.params.gattc_write.p_value  = &motor_direction;
    write_req.params.gattc_write.write_op = BLE_GATT_OP_WRITE_CMD;
    write_req.params.gattc_write.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;

    return nrf_ble_gq_item_add(p_ble_ves_c->p_gatt_queue, &write_req, p_ble_ves_c->conn_handle);
}

uint32_t ble_ves_c_handles_assign(ble_ves_c_t               * p_ble_ves,
                                  uint16_t                    conn_handle,
                                  ble_ves_c_handles_t const * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_ves);

    p_ble_ves->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_ves->handles.temp_cccd_handle     = p_peer_handles->temp_cccd_handle;
        p_ble_ves->handles.temp_handle          = p_peer_handles->temp_handle;
        p_ble_ves->handles.status_cccd_handle   = p_peer_handles->status_cccd_handle;
        p_ble_ves->handles.status_handle        = p_peer_handles->status_handle;
        p_ble_ves->handles.dc_motor_handle      = p_peer_handles->dc_motor_handle;
    }
    return nrf_ble_gq_conn_handle_register(p_ble_ves->p_gatt_queue, conn_handle);
}


