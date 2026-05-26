idf_component_register(SRCS "web_based_display.c" "main.c"                    
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_adc esp_driver_mcpwm esp_driver_gpio esp_driver_uart esp_http_server esp_wifi
                       REQUIRES driver)
