#pragma once
#include "Arduino.h"
#include <deque>
struct FakeQueue {size_t cap,size;std::deque<std::vector<uint8_t>> q;};
using QueueHandle_t=FakeQueue*;
inline QueueHandle_t xQueueCreate(size_t c,size_t s){return new FakeQueue{c,s,{}};}
inline int xQueueSend(QueueHandle_t q,const void*p,int){if(q->q.size()>=q->cap)return 0;q->q.emplace_back((const uint8_t*)p,(const uint8_t*)p+q->size);return 1;}
inline int xQueueReceive(QueueHandle_t q,void*p,int){if(q->q.empty())return 0;memcpy(p,q->q.front().data(),q->size);q->q.pop_front();return 1;}
