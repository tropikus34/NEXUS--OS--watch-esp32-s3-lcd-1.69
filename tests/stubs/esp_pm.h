#pragma once
#define ESP_OK 0
struct esp_pm_config_t{int max_freq_mhz,min_freq_mhz;bool light_sleep_enable;};
inline int fakePmError=0,fakePmCalls=0;inline esp_pm_config_t fakePmConfig={};
inline int esp_pm_configure(const esp_pm_config_t* config){++fakePmCalls;if(!fakePmError)fakePmConfig=*config;return fakePmError;}
