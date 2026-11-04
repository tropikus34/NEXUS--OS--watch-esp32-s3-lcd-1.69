#pragma once
#include "Arduino.h"
#define pdPASS 1
#define pdMS_TO_TICKS(ms) (ms)
using TaskHandle_t=void*;
inline int xTaskCreate(void(*)(void*),const char*,uint32_t,void*,int,TaskHandle_t* h){*h=(void*)1;return pdPASS;}
inline void vTaskDelay(uint32_t ms){delay(ms);}
