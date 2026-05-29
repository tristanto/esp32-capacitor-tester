#include "wifi_config_mgr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "NVS_MGR";
#define NVS_NAMESPACE "wifi_store"
#define NVS_KEY_CONFIG "wifi_cfg"

void init_nvs_storage(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

bool load_wifi_credentials(wifi_stored_config_t *config) {
    nvs_handle_t handle;
    memset(config, 0, sizeof(wifi_stored_config_t));
    
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }
    
    size_t required_size = sizeof(wifi_stored_config_t);
    esp_err_t err = nvs_get_blob(handle, NVS_KEY_CONFIG, config, &required_size);
    nvs_close(handle);
    
    return (err == ESP_OK && config->is_configured);
}

void save_wifi_credentials(const char *ssid, const char *password) {
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
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
void clear_wifi_credentials(void) {
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_key(handle, NVS_KEY_CONFIG);
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGW(TAG, "Wi-Fi credentials have been cleared from NVS.   ");
    }
}