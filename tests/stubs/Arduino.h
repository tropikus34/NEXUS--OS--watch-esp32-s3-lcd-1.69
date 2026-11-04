#pragma once
#include <cstdint>
#include <cassert>
#include <deque>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <cstdarg>
using std::size_t;
using String=std::string;
inline uint32_t fakeCpu=240;inline bool setCpuFrequencyMhz(uint32_t mhz){fakeCpu=mhz;return true;}inline uint32_t getCpuFrequencyMhz(){return fakeCpu;}
inline uint32_t fakeMs=0;
inline uint32_t millis(){return fakeMs;}
inline void delay(int ms){fakeMs+=ms;}
inline int fakeButton=1;
inline int digitalRead(int){return fakeButton;}
inline std::map<int,int> fakeDigitalValues;inline void digitalWrite(int pin,int value){fakeDigitalValues[pin]=value;}
inline void pinMode(int,int){}
inline int fakeBacklight=-1;inline void analogWrite(int,int value){fakeBacklight=value;}
inline int analogReadMilliVolts(int){return 1267;}
inline void analogReadResolution(int){}
inline void analogSetPinAttenuation(int,int){}
inline void tone(int,int,int){}
inline void noTone(int){}
inline float temperatureRead(){return 30;}
template<class T,class L,class H> T constrain(T x,L l,H h){return x<l?l:x>h?h:x;}
#define OUTPUT 1
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0
#define ADC_11db 0
struct SerialT {bool attached=false;explicit operator bool()const{return attached;}std::deque<uint8_t> input;std::string output;void begin(int){}int available(){return input.size();}int read(){if(input.empty())return -1;int n=input.front();input.pop_front();return n;}void println(const char*s){output+=s;output+="\n";}void printf(const char*f,...){char b[512];va_list a;va_start(a,f);vsnprintf(b,sizeof(b),f,a);va_end(a);output+=b;} }; inline SerialT Serial;
struct Esp {void restart(){} unsigned getFreeHeap(){return 100000;}};inline Esp ESP;
