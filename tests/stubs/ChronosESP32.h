#pragma once
#include "Arduino.h"
#include <ctime>
struct Notification {String app,title,message;};
enum Config{CF_TIME,CF_HR24,CF_FIND};
class ChronosESP32 {public:ChronosESP32(const char*){} template<class T>void setConnectionCallback(T){} template<class T>void setNotificationCallback(T){} template<class T>void setConfigurationCallback(T){} bool isSubscribed(){return true;} void setName(const char*){} void begin(){} void loop(){} void setBattery(int){} tm getTimeStruct(){return {};}};
