#include "util.h"

void status_update(int status){

    ret_code_t err_code;
    NRF_LOG_INFO("status code: %d", status_notifications_enable);
    if (status_notifications_enable == true){
      err_code = ble_ves_status_update(&m_ves, status);
      APP_ERROR_CHECK(err_code);
    }else{
      NRF_LOG_INFO("Status notifications are not enable. Enable them first");  /* NOTE: Look for a way to enable notifications by default. It can al be done from central device within its process */
    }

}