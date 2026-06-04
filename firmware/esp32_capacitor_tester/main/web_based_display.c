#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_http_server.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "Web_Display";

// Fungsi eksternal dari modul hardware kapasitor Anda
extern uint32_t get_capacitor_measurement(void); 

// --- HTML, CSS & JAVASCRIPT TO BE SENT TO THE BROWSER ---
static const char* html_page = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"    <meta charset='UTF-8'>"
"    <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"    <title>ESP32 Capacitor Tester</title>"
"    <style>"
"        body { font-family: 'Segoe UI', Arial, sans-serif; background-color: #f0f2f5; margin: 0; padding: 20px; display: flex; justify-content: center; align-items: center; min-height: 100vh; }"
"        .card { background: white; padding: 30px; border-radius: 16px; box-shadow: 0 4px 20px rgba(0,0,0,0.08); text-align: center; max-width: 400px; width: 100%; }"
"        h1 { color: #1a73e8; margin-bottom: 5px; font-size: 24px; }"
"        .subtitle { color: #5f6368; font-size: 14px; margin-bottom: 25px; }"
"        .display-container { background: #f8f9fa; border: 2px solid #e8eaed; border-radius: 12px; padding: 25px; margin-bottom: 20px; }"
"        .value { font-size: 48px; font-weight: bold; color: #202124; margin: 0; transition: color 0.3s; }"
"        .unit { font-size: 20px; color: #5f6368; font-weight: normal; margin-left: 5px; }"
"        .status { font-size: 12px; font-weight: bold; padding: 6px 12px; border-radius: 20px; display: inline-block; }"
"        .status.connected { background-color: #e6f4ea; color: #137333; }"
"        .status.disconnected { background-color: #fce8e6; color: #c5221f; }"
"    </style>"
"</head>"
"<body>"
"    <div class='card'>"
"        <h1>Capacitor Tester</h1>"
"        <div class='subtitle'>ESP32 Web Real-Time Data Stream</div>"
"        "
"        <div class='display-container'>"
"            <p class='value' id='cap-value'>0<span class='unit'>pF</span></p>"
"        </div>"
"        "
"        <div id='status-indicator' class='status disconnected'>Connecting...</div>"
"    </div>"
""
"    <script>"
"        // Fungsi mendengarkan data langsung (HTTP Chunked Stream) secara asinkron"
"        async function startStream() {"
"            const statusEl = document.getElementById('status-indicator');"
"            const valueEl = document.getElementById('cap-value');"
"            "
"            try {"
"                // Membuka koneksi data streaming ke URL /data"
"                const response = await fetch('/data');"
"                const reader = response.body.getReader();"
"                const decoder = new TextDecoder();"
"                "
"                statusEl.innerText = 'CONNECTED';"
"                statusEl.className = 'status connected';"
"                "
"                let buffer = '';"
"                while (true) {"
"                    const { value, done } = await reader.read();"
"                    if (done) break;"
"                    "
"                    buffer += decoder.decode(value, { stream: true });"
"                    const lines = buffer.split('\\n');"
"                    "
"                    // Ambil baris terakhir yang lengkap"
"                    for (let i = 0; i < lines.length - 1; i++) {"
"                        const val = lines[i].trim();"
"                        if (val) {"
"                            valueEl.innerHTML = val + '<span class=\"unit\">pF</span>';"
"                        }"
"                    }"
"                    buffer = lines[lines.length - 1];"
"                }"
"            } catch (error) {"
"                console.error(error);"
"                statusEl.innerText = 'DISCONNECTED';"
"                statusEl.className = 'status disconnected';"
"                // Retry to connect after 2 minutes"
"                setTimeout(startStream, 2000); "
"            }"
"        }"
"        "
"        // Start streaming after page loaded successfully"
"        window.onload = startStream;"
"    </script>"
"</body>"
"</html>";

// 1. Main handler for dashboard view(HTML)
static esp_err_t index_html_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
}

// 2. Maesurement value stream handler (HTTP Chunked Data Stream)
static esp_err_t data_stream_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/plain");
    
    char chunk_buffer[32];
    ESP_LOGI(TAG, "Stream data aktif untuk satu klien.");

    while (true) {
        uint32_t measurement = get_capacitor_measurement();
        
        // Hanya mengirim angka mentahnya saja agar mudah diolah JavaScript frontend
        int len = snprintf(chunk_buffer, sizeof(chunk_buffer), "%lu\n", measurement);
        
        if (len < 0 || len >= sizeof(chunk_buffer)) {
            continue; 
        }

        // Kirim data ke background page, putuskan jika browser di-refresh/ditutup
        esp_err_t ret = httpd_resp_send_chunk(req, chunk_buffer, len);
        if (ret != ESP_OK) {
            ESP_LOGI(TAG, "Klien menutup halaman dashboard.");
            break; 
        }
        
        vTaskDelay(pdMS_TO_TICKS(300)); // Refresh data di layar setiap 300ms
    }
   
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// --- REGISTRASI URL PATH ---

// Mengarah ke halaman utama root (http://IP_ESP32/)
static const httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_html_handler,
    .user_ctx  = NULL
};

// Mengarah ke penyuplai data latar belakang (http://IP_ESP32/data)
static const httpd_uri_t data_uri = {
    .uri       = "/data",
    .method    = HTTP_GET,
    .handler   = data_stream_handler,
    .user_ctx  = NULL
};

// 3. Fungsi Memulai Server Web
httpd_handle_t start_web_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Menjalankan server modern pada port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Daftarkan kedua URL agar saling bekerja sama
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &data_uri);
        return server;
    }

    ESP_LOGE(TAG, "Gagal mengaktifkan komponen server!");
    return NULL;
}
