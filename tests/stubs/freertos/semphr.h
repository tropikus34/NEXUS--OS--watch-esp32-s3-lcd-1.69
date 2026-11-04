#pragma once
#include "Arduino.h"
#include "task.h"
using SemaphoreHandle_t=bool*;
inline SemaphoreHandle_t xSemaphoreCreateMutex(){return new bool(false);}
inline int xSemaphoreTake(SemaphoreHandle_t s,uint32_t){if(*s)return 0;*s=true;return 1;}
inline int xSemaphoreGive(SemaphoreHandle_t s){*s=false;return 1;}
