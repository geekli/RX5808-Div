#include "nvs_rw.h"
#include "esp_log.h"

static const char* TAG = "NVS";
nvs_handle_t nvs_config_handle;

void nvs_init()
{
     // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    err = nvs_open("storage", NVS_READWRITE, &nvs_config_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 

}

void nvs_get_configs(uint16_t *configs, uint16_t len)
{
    esp_err_t err;
    char  keyname[10];
    for(uint16_t i=0; i<len; i++) {
        sprintf(keyname, "cnf%d", i);
        uint16_t val = 0;
        err = nvs_get_u16(nvs_config_handle, keyname, &val);
        switch (err) {
            case ESP_OK:
                ESP_LOGI(TAG, "key:%s %u",keyname, val);
                configs[i] = val;
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGI(TAG, "The value is not initialized yet!");
                configs[i] =0;
                break;
            default :
                ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
                configs[i] =0;
        } 
    }
}

void nvs_set_configs(uint16_t *configs,uint16_t len)
{
    ESP_LOGI(TAG, "save config");
    esp_err_t err;
    char  keyname[10];
    for(uint16_t i=0; i<len; i++) {
        sprintf(keyname, "cnf%u", i);
        err = nvs_set_u16(nvs_config_handle, keyname, configs[i]);
        if(err == ESP_OK) {
            ESP_LOGI(TAG, "save key:%s %u",keyname, configs[i]);
        }
        else {
            ESP_LOGE(TAG, "save (%s) !", esp_err_to_name(err));
        }
    }
}
