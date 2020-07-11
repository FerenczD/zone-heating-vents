#include "HttpPost.h"

 char * request(esp_err_t(* eventHandler)(esp_http_client_event_t *), char * query) 
{
     esp_http_client_config_t config = {
        .url = WEB_URL,
        .event_handler = eventHandler,
    };
    ESP_LOGI(POST_TAG, "Post request payload: %s",query);
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_set_url(client, SERVER_URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, query, strlen(query));
    err = esp_http_client_perform(client);
    char * buffer = (char *) malloc(esp_http_client_get_content_length(client));

    if (err != ESP_OK) {
        ESP_LOGE(POST_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    } else {
        esp_http_client_read(client,buffer,esp_http_client_get_content_length(client));
        ESP_LOGI(POST_TAG, "Server responded with: %s",buffer);
     }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    return buffer;

}


