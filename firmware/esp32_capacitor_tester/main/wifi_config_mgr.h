#ifndef WIFI_CONFIG_MGR_H
#define WIFI_CONFIG_MGR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

typedef struct {
    char ssid[WIFI_SSID_MAX_LEN];
    char password[WIFI_PASS_MAX_LEN];
    bool is_configured;
} wifi_stored_config_t;

void init_nvs_storage(void);
bool load_wifi_credentials(wifi_stored_config_t *config);
void save_wifi_credentials(const char *ssid, const char *password);
void clear_wifi_credentials(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_CONFIG_MGR_H
