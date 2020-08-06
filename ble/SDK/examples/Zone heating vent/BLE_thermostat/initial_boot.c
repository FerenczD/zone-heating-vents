#include "initial_boot.h"


static void fds_evt_handler(fds_evt_t const * p_evt)
{
    switch (p_evt->id)
    {
        case FDS_EVT_INIT:
            if (p_evt->result == NRF_SUCCESS)
            {
                m_fds_initialized = true;
            }
            break;

        case FDS_EVT_WRITE:
        {
            if (p_evt->result == NRF_SUCCESS)
            {
                NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->write.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->write.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
            }
        } break;

//        case FDS_EVT_DEL_RECORD:
//        {
//            if (p_evt->result == NRF_SUCCESS)
//            {
//                NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->del.record_id);
//                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->del.file_id);
//                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
//            }
//            m_delete_all.pending = false;
//        } break;

        default:
            break;
    }
}

/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
    while (!m_fds_initialized);
}

void factory_reset(){
    ret_code_t err_code;

    /* Register first to receive an event when initialization is complete. */
    (void) fds_register(fds_evt_handler);

    err_code = fds_init();
    APP_ERROR_CHECK(err_code);

    wait_for_fds_ready();

    fds_stat_t stat = {0};

    err_code = fds_stat(&stat);
    APP_ERROR_CHECK(err_code);

    fds_record_desc_t desc = {0};
    fds_find_token_t tok   = {0};

    if (fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok) == NRF_SUCCESS)
    {
        ret_code_t rc = fds_record_delete(&desc);
        if (rc != NRF_SUCCESS)
        {
            NRF_LOG_INFO("FDS DELETE ERROR");
        }else{
            NRF_LOG_INFO("Reseted to factory");
        }

    }else{
        NRF_LOG_INFO("Record doesnt exist");
    }
}




int initial_boot(){

    ret_code_t err_code;
    int ret = 1;  // Initial boot by default

    if(!m_fds_initialized){
        /* Register first to receive an event when initialization is complete. */
        (void) fds_register(fds_evt_handler);

        err_code = fds_init();
        APP_ERROR_CHECK(err_code);

        wait_for_fds_ready();
    }

    fds_record_desc_t desc = {0};
    fds_find_token_t  tok  = {0};

    err_code = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok);
  
    if(err_code == NRF_SUCCESS){  /* Record found. No first boot */

        /* A config file is in flash. Let's update it. */
        fds_flash_record_t config = {0};
        /* Open the record and read its contents. */
        err_code = fds_record_open(&desc, &config);
        APP_ERROR_CHECK(err_code);

        memcpy(&m_initial_boot, config.p_data, sizeof(uint32_t));
        m_initial_boot = false;

        err_code = fds_record_close(&desc);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_INFO("Initial boot: %s", (m_initial_boot == false) ? "Set" : "Not set");
        
        ret = 0;

    }else{  /* Record not found. First boot */
        NRF_LOG_INFO("No boot record found. Creating one");

        m_initial_boot = true;

        fds_record_t record;

        record.file_id = CONFIG_FILE;
        record.key     = CONFIG_REC_KEY;
        record.data.p_data = &m_initial_boot;
        record.data.length_words = (sizeof(m_initial_boot)+3)/sizeof(uint32_t);   /* Size of record is in word units*/

        err_code = fds_record_write(&desc, &record);

        if ((err_code != NRF_SUCCESS) && (err_code == FDS_ERR_NO_SPACE_IN_FLASH))
        {
            NRF_LOG_INFO("No space in flash, delete some records to update the config file.");
        }
        
        ret = 1;  /* Is the first boot */
    }
    return ret;
}
