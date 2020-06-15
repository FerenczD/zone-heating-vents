/* Custom service for Capstone application */

#include "ble_ves.h"

uint32_t ble_ves_init(ble_ves_t * p_ves, const ble_ves_init_t * p_ves_init)
{
    if (p_ves == NULL || p_ves_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_ves->evt_handler               = p_ves_init->evt_handler;
    p_ves->conn_handle               = BLE_CONN_HANDLE_INVALID;

    // Add Vent Service UUID
    ble_uuid128_t base_uuid = {VENT_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_ves->uuid_type);   /* Add UUID to the BLE stack */
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_ves->uuid_type;
    ble_uuid.uuid = VENT_SERVICE_UUID;

    // Add the Vent Service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_ves->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    /* Add custom characteristics */
    ves_char_setting_t char_settings;

    /* Add motor direction characteristic */
    memset(&char_settings, 0, sizeof(char_settings));
    char_settings.uuid = DC_MOTOR_CHAR_UUID;
    char_settings.write_enable = OPTION_ENABLE;
    char_settings.read_enable = OPTION_DISABLE;
    char_settings.notify_enable = OPTION_DISABLE;
    char_settings.data_size = sizeof(uint8_t);

    err_code = custom_value_char_add(p_ves, p_ves_init, char_settings, &p_ves->dc_motor_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    /* Add temperature reading characteristic */
    memset(&char_settings, 0, sizeof(char_settings));
    char_settings.uuid = TEMP_CHAR_UUID;
    char_settings.write_enable = OPTION_DISABLE;
    char_settings.read_enable = OPTION_ENABLE;
    char_settings.notify_enable = OPTION_ENABLE;
    char_settings.data_size = sizeof(uint16_t);

    err_code = custom_value_char_add(p_ves, p_ves_init, char_settings, &p_ves->temp_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    /* Add vent status characteristic. Open, closed, error, etc */
    memset(&char_settings, 0, sizeof(char_settings));
    char_settings.uuid = VENT_STATUS_UUID;
    char_settings.write_enable = OPTION_DISABLE;
    char_settings.read_enable = OPTION_ENABLE;
    char_settings.notify_enable = OPTION_ENABLE;
    char_settings.data_size = sizeof(uint8_t);


    err_code = custom_value_char_add(p_ves, p_ves_init, char_settings, &p_ves->status_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    VERIFY_SUCCESS(err_code);
    
    return err_code;
}

static uint32_t custom_value_char_add(ble_ves_t * p_ves, const ble_ves_init_t * p_ves_init, ves_char_setting_t char_settings, ble_gatts_char_handles_t* char_handle)
{
    uint32_t            err_code;
    ble_gatts_char_md_t char_md;          /* Properties to be displayed to Central during discovery. This is a structure holding the value properties of the characteristic value */
    ble_gatts_attr_md_t cccd_md;          /* Client Characteristic Configuration Descriptor. Controls what kind of packet GATT server can send to client. CCCD is a writable descriptor that allows the client, i.e. your nRF Connect for Desktop or phone, to enable or disable notification or indication, on your kit.*/
    ble_gatts_attr_t    attr_char_value;  /* Set the UUID, points to the attribute metadata and the size of the characteristic. This structure holds the actual value of the characteristic  */
    ble_uuid_t          ble_uuid;         /* Hold UUID information */
    ble_gatts_attr_md_t attr_md;          /* his is a structure holding permissions and authorization levels required by characteristic value attributes */

    /* Inititalize characteristic properties */
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read   = char_settings.read_enable;
    char_md.char_props.write  = char_settings.write_enable;
    char_md.char_props.notify = char_settings.notify_enable;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL; 
    char_md.p_sccd_md         = NULL;

    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.read_perm  = p_ves_init->custom_value_char_attr_md.read_perm;
    attr_md.write_perm = p_ves_init->custom_value_char_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;                              /* Store in SoftDevice RAM, not Application RAM */
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = p_ves->uuid_type;
    ble_uuid.uuid = char_settings.uuid;

    memset(&attr_char_value, 0, sizeof(attr_char_value));
    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = char_settings.data_size;  // NOTE: Initial lenght to be 1 byte. Can change
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = char_settings.data_size;  // NOTE:1 byte length as of right now. Can change

    //  Read  operation on Cccd should be possible without authentication.
    memset(&cccd_md, 0, sizeof(cccd_md));
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;            /* CCCD is stored in Softdevice memory, not application */
    char_md.p_cccd_md  = &cccd_md; 
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    /* Add characteristic */
    err_code = sd_ble_gatts_characteristic_add(p_ves->service_handle, &char_md,
                                               &attr_char_value,
                                               char_handle);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

void ble_ves_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_ves_t * p_ves = (ble_ves_t *) p_context;
    
    if (p_ves == NULL || p_ble_evt == NULL)
    {
        return;
    }

    /* Handle events */
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_ves, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_ves, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_ves, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

static void on_connect(ble_ves_t * p_ves, ble_evt_t const * p_ble_evt)
{
    p_ves->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    ble_ves_evt_t evt;
    evt.evt_type = BLE_VES_EVT_CONNECTED;
    p_ves->evt_handler(p_ves, &evt);
}

static void on_disconnect(ble_ves_t * p_ves, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_ves->conn_handle = BLE_CONN_HANDLE_INVALID;

    ble_ves_evt_t evt;
    evt.evt_type = BLE_VES_EVT_DISCONNECTED;
    p_ves->evt_handler(p_ves, &evt);
}

static void on_write(ble_ves_t * p_ves, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    ble_ves_evt_t evt;

    // Check if the handle passed with the event matches the Custom Characteristic handle.
    if (p_evt_write->handle == p_ves->dc_motor_handle.value_handle)
    {
        
        if(p_evt_write->data != NULL){
          evt.evt_type = BLE_VES_EVT_DC;

           evt.dc_direction = p_evt_write->data[0];

           p_ves->evt_handler(p_ves, &evt);
        }
    }

    //To enable notifications. Check if the Custom CCCD is written to and that the value is the appropriate length, i.e 2 bytes. 
    else if ((p_evt_write->handle == p_ves->temp_handle.cccd_handle)
              && (p_evt_write->len == 2))
    {
        // CCCD written, call application event handler
        if (p_ves->evt_handler != NULL)
        {
            ble_ves_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_VES_EVT_TEMP_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_VES_EVT_TEMP_NOTIFICATION_DISABLED;
            }
            // Call the application event handler.
            p_ves->evt_handler(p_ves, &evt);
        }

    }

    else if ((p_evt_write->handle == p_ves->status_handle.cccd_handle)
              && (p_evt_write->len == 2))
    {
        // CCCD written, call application event handler
        if (p_ves->evt_handler != NULL)
        {
            ble_ves_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_VES_EVT_STATUS_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_VES_EVT_STATUS_NOTIFICATION_DISABLED;
            }

            // Call the application event handler.
            p_ves->evt_handler(p_ves, &evt);
        }

    }
}

uint32_t ble_ves_temperature_update(ble_ves_t * p_ves, uint16_t temperature){
    
    //NRF_LOG_INFO("In ble_ves_temperature_update. Temp: %d", temperature); 

    if (p_ves == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(temperature);
    gatts_value.offset  = 0;
    gatts_value.p_value = &temperature;

    // Update database.
    err_code = sd_ble_gatts_value_set(p_ves->conn_handle,
                                        p_ves->temp_handle.value_handle,
                                        &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((p_ves->conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_ves->temp_handle.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(p_ves->conn_handle, &hvx_params);
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}

uint32_t ble_ves_status_update(ble_ves_t * p_ves, int32_t status){
    
    NRF_LOG_INFO("In ble_ves_status_update. Status code: %d", status); 

    if (p_ves == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = &status;

    // Update database.
    err_code = sd_ble_gatts_value_set(p_ves->conn_handle,
                                        p_ves->status_handle.value_handle,
                                        &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((p_ves->conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_ves->status_handle.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(p_ves->conn_handle, &hvx_params);
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}