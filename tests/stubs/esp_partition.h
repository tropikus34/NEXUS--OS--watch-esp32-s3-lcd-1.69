#pragma once
#include <cstdint>
#include <cstring>
struct esp_partition_t{uint32_t size=0x170000;};
inline bool fakeFsPartition=true;inline esp_partition_t fakePartition;
constexpr int ESP_PARTITION_TYPE_DATA=1,ESP_PARTITION_SUBTYPE_ANY=255;
inline const esp_partition_t* esp_partition_find_first(int,int,const char* label){return fakeFsPartition&&!strcmp(label,"spiffs")?&fakePartition:nullptr;}
