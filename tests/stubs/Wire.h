#pragma once
#include "Arduino.h"
#include <array>
struct WireT {
 bool emulate=false,fail=false;uint8_t address=0,reg=0;std::vector<uint8_t>tx,rx;size_t pos=0;uint32_t transactions=0;
 std::map<uint8_t,std::array<uint8_t,256>>devices;
 void begin(int,int){}void setClock(int){}void setTimeOut(int){}
 void beginTransmission(int a){address=a;tx.clear();++transactions;}
 void write(int n){tx.push_back(n);}
 int endTransmission(bool=true){if(!emulate||fail||!devices.count(address))return 1;if(tx.size()){reg=tx[0];for(size_t i=1;i<tx.size();++i)devices[address][reg+i-1]=tx[i];}return 0;}
 int requestFrom(uint8_t a,uint8_t count){rx.clear();pos=0;if(!emulate||fail||!devices.count(a))return 0;for(int i=0;i<count;++i)rx.push_back(devices[a][reg+i]);if(reg==0x35&&count==6)devices[a][0x2e]&=~1;return count;}
 int read(){return pos<rx.size()?rx[pos++]:0;}
};inline WireT Wire;
