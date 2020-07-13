#ifndef HTTPPOST_H_
#define HTTPPOST_H_

#include <string.h>

#define WEB_URL "http://192.168.0.14"
#define SERVER_URL "http://192.168.0.14/venti/serverRequests.php"

static const char *TAG = "HttpPost";

 esp_http_client_config_t config = {
        .url = WEB_URL,
        .event_handler = HttpPost::_http_event_handler,
    };

class HttpPost
{
    private:
        ventCmd_t       ventCmd;
        ventStatus_t    ventStatus;
    public:
        int request(char * query);
        esp_err_t _http_event_handler(_http_event_handler(esp_http_client_event_t *evt);
};

#endif 