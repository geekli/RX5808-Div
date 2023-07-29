#ifndef __NVS_RW_H
#define __NVS_RW_H
#include <nvs.h>
#include <nvs_flash.h>

void nvs_init();
void nvs_get_configs(uint16_t *configs, uint16_t len);
void nvs_set_configs(uint16_t *configs, uint16_t len);
bool nvs_set_config(uint16_t key, uint16_t val);
#endif