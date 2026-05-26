#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "ws_stream";

// WebSocket URI handler
esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake successful, client connected");
        return ESP_OK;
    }
    
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    
    // Query packet details first
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to frame query: %d", ret);
        return ret;
    }
    
    if (ws_pkt.len > 0) {
        uint8_t *buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        
        // Fetch actual data payload
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Received packet: %s", ws_pkt.payload);
        free(buf);
    }
    return ESP_OK;
}

// Global configuration wrapper for registration
httpd_uri_t ws_uri = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_handler,
    .user_ctx   = NULL,
    .is_websocket = true
};
