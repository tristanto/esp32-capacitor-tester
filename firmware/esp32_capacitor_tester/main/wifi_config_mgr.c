#include "wifi_config_mgr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include "wifi_config_mgr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define NVS_NAMESPACE "wifi_store"
#define NVS_KEY_CONFIG "wifi_cfg"

#define AP_SSID "CAP_TEST_Config"
#define AP_PASS "12345678"
#define MAX_SCAN_AP 15

#define BOOT_BUTTON_PIN GPIO_NUM_0
#define DEBOUNCE_DELAY_MS 3000

bool wifi_is_connected;
char wifi_ip_address[16] = "0.0.0.0";

static const char *TAG = "WiFi_Config";

/* ====================================================================
   1. STORAGE MANAGEMENT(NVS)
   ==================================================================== */

void init_nvs_storage(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

bool load_wifi_credentials(wifi_stored_config_t *config)
{
    nvs_handle_t handle;
    memset(config, 0, sizeof(wifi_stored_config_t));

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    size_t required_size = sizeof(wifi_stored_config_t);
    esp_err_t err = nvs_get_blob(handle, NVS_KEY_CONFIG, config, &required_size);
    nvs_close(handle);

    return (err == ESP_OK && config->is_configured);
}

void save_wifi_credentials(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK)
    {
        wifi_stored_config_t config;
        memset(&config, 0, sizeof(wifi_stored_config_t));

        strncpy(config.ssid, ssid, WIFI_SSID_MAX_LEN - 1);
        strncpy(config.password, password, WIFI_PASS_MAX_LEN - 1);
        config.is_configured = true;

        nvs_set_blob(handle, NVS_KEY_CONFIG, &config, sizeof(wifi_stored_config_t));
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGI(TAG, "The credentials for Wi-Fi SSID '%s' have been saved to NVS.", ssid);
    }
}
void clear_wifi_credentials(void)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK)
    {
        nvs_erase_key(handle, NVS_KEY_CONFIG);
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGW(TAG, "Wi-Fi credentials have been cleared from NVS.");
    }
}

/* ====================================================================
   2. HTTP WEB SERVER HANDLERS
   ==================================================================== */
static esp_err_t wifi_scan_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received request for Wi-Fi configuration page.");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "close");

    wifi_scan_config_t scan_config = {.ssid = 0, .bssid = 0, .channel = 0, .show_hidden = false};
    esp_wifi_scan_start(&scan_config, true);

    uint16_t ap_count = MAX_SCAN_AP;
    wifi_ap_record_t ap_records[MAX_SCAN_AP];
    ESP_LOGI(TAG, "Scanning for Wi-Fi networks...");
    esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    ESP_LOGI(TAG, "Found %d access points.", ap_count);

    char buf[128];
    httpd_resp_send_chunk(req, "<html><body><h2>Konfigurasi Wi-Fi ESP32</h2>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<form action='/save' method='POST'>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<label>Pilih Wi-Fi:</label><br><select name='ssid'>", HTTPD_RESP_USE_STRLEN);

    for (int i = 0; i < ap_count; i++)
    {
        snprintf(buf, sizeof(buf), "<option value='%s'>%s (RSSI: %d)</option>",
                 (char *)ap_records[i].ssid, (char *)ap_records[i].ssid, ap_records[i].rssi);
        httpd_resp_send_chunk(req, buf, HTTPD_RESP_USE_STRLEN);
    }

    httpd_resp_send_chunk(req, "</select><br><br>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<label>Password:</label><br><input type='password' name='pass'><br><br>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<input type='submit' value='Simpan dan Hubungkan'></form></body></html>", HTTPD_RESP_USE_STRLEN);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t wifi_save_post_handler(httpd_req_t *req)
{
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Gagal menerima data");
        return ESP_FAIL;
    }
    content[ret] = '\0';

    char ssid[WIFI_SSID_MAX_LEN] = {0};
    char pass[WIFI_PASS_MAX_LEN] = {0};
    char *ssid_ptr = strstr(content, "ssid=");
    char *pass_ptr = strstr(content, "pass=");

    if (ssid_ptr && pass_ptr)
    {
        sscanf(ssid_ptr, "ssid=%[^&]", ssid);
        sscanf(pass_ptr, "pass=%s", pass);

        save_wifi_credentials(ssid, pass);

        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_send_chunk(req, "<h3>Konfigurasi disimpan! ESP32 memuat ulang...</h3>", HTTPD_RESP_USE_STRLEN);
        httpd_resp_send_chunk(req, NULL, 0);

        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    }
    return ESP_OK;
}

static void start_config_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = wifi_scan_get_handler};
        httpd_register_uri_handler(server, &index_uri);
        httpd_uri_t save_uri = {.uri = "/save", .method = HTTP_POST, .handler = wifi_save_post_handler};
        httpd_register_uri_handler(server, &save_uri);
        ESP_LOGI(TAG, "Web Server aktif di http://192.168.4.1");
    }
}

/* ====================================================================
   3. WI-FI DRIVER LOGIC (AP & STA)
   ==================================================================== */
static void start_access_point_mode(void)
{
    ESP_LOGI(TAG, "Starting access point mode...");
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK},
    };
    if (strlen(AP_PASS) == 0)
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    start_config_webserver();
}

// Handler untuk memantau status koneksi fisik Wi-Fi
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "Koneksi Wi-Fi terputus atau gagal terhubung!");

        // Atur ke false karena perangkat tidak lagi memiliki koneksi aktif
        wifi_is_connected = false;
        strcpy(wifi_ip_address, "0.0.0.0");

        // Auto-reconnect: Coba menghubungkan kembali secara otomatis ke router
        esp_wifi_connect();
    }
}

// Handler untuk menangkap saat router SELESAI memberikan Alamat IP via DHCP
static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        // 1. Ambil alamat IP biner dan ubah menjadi teks string ("192.168.1.XX")
        esp_ip4addr_ntoa(&event->ip_info.ip, wifi_ip_address, sizeof(wifi_ip_address));

        ESP_LOGI(TAG, "--- KONEKSI SUKSES ---");
        ESP_LOGI(TAG, "Mendapatkan Alamat IP: %s", wifi_ip_address);

        // 2. DI SINI tempat yang benar untuk mengubah variabel menjadi true!
        wifi_is_connected = true;
    }
}

static void start_station_mode(wifi_stored_config_t *stored_cfg)
{
    ESP_LOGI(TAG, "Starting client mode...");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, stored_cfg->ssid);
    strcpy((char *)wifi_config.sta.password, stored_cfg->password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to %s...", stored_cfg->ssid);

    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Gagal memerintahkan driver Wi-Fi untuk terhubung: %d", ret);
    }
}

/* ====================================================================
   4. LOGIKA TOMBOL & MANAGEMEN UTAMA
   ==================================================================== */
static bool check_force_factory_reset(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    if (gpio_get_level(BOOT_BUTTON_PIN) == 0)
    {
        ESP_LOGW(TAG, "Reset button detected! Hold for 3 seconds...");
        int elapsed_ms = 0;
        while (elapsed_ms < DEBOUNCE_DELAY_MS)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (gpio_get_level(BOOT_BUTTON_PIN) != 0)
            {
                ESP_LOGI(TAG, "Reset cancelled!");
                return false;
            }
            elapsed_ms += 100;
        }
        clear_wifi_credentials();
        return true;
    }
    return false;
}

// Fungsi pintu masuk utama yang dipanggil dari main.c
void wifi_manager_init(void)
{
    init_nvs_storage();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // WAJIB: Daftarkan kedua Event Handler di atas agar sistem mendengarkan status Wi-Fi & IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL, NULL));

    bool force_ap = check_force_factory_reset();
    wifi_stored_config_t stored_wifi;
    bool has_config = load_wifi_credentials(&stored_wifi);

    if (force_ap || !has_config)
    {
        ESP_LOGI(TAG, "No valid Wi-Fi configuration found. Starting in AP mode for setup.");
        start_access_point_mode();
    }
    else
    {
        ESP_LOGI(TAG, "Valid Wi-Fi configuration found. Starting in Station mode.");
        start_station_mode(&stored_wifi);
    }
}
