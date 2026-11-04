#pragma once
#include "Arduino.h"
#define BLE_HS_IO_NO_INPUT_OUTPUT 3
struct FakeAddress {std::string toString(){return "aa:bb:cc:dd:ee:ff";}};
struct NimBLEConnInfo { bool isBonded(){return fakeBonded;} uint16_t getConnHandle(){return 1;} FakeAddress getIdAddress(){return {};} static inline bool fakeBonded=false;};
struct FakeServer {int updates=0;uint16_t minInterval=0,maxInterval=0,latency=0,timeout=0;void updateConnParams(uint16_t,uint16_t a,uint16_t b,uint16_t c,uint16_t d){++updates;minInterval=a;maxInterval=b;latency=c;timeout=d;}int count=0;int getConnectedCount(){return count;}NimBLEConnInfo getPeerInfo(uint8_t){return {};}bool disconnect(uint16_t){count=0;return true;}};
struct FakeAdvertising {uint16_t minInterval=0,maxInterval=0;void setMinInterval(uint16_t v){minInterval=v;}void setMaxInterval(uint16_t v){maxInterval=v;}bool active=true;int starts=0;bool isAdvertising(){return active;}bool start(){active=true;++starts;return true;}bool stop(){active=false;return true;}};
struct NimBLEDevice {static inline FakeServer server;static inline FakeAdvertising adv;static inline int requests=0;static FakeServer*getServer(){return &server;}static FakeAdvertising*getAdvertising(){return &adv;}static void setSecurityAuth(bool,bool,bool){}static void setSecurityIOCap(int){}static bool startSecurity(uint16_t){++requests;return true;}static bool deleteAllBonds(){NimBLEConnInfo::fakeBonded=false;return true;}};
