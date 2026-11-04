#pragma once
#include "Arduino.h"
#include <memory>
struct File {
 std::shared_ptr<std::vector<uint8_t>> bytes;size_t pos=0;bool opened=false;
 File()=default;File(std::shared_ptr<std::vector<uint8_t>>b):bytes(b),opened(true){}
 explicit operator bool()const{return opened&&bool(bytes);}size_t size()const{return bytes?bytes->size():0;}
 void close(){opened=false;}void flush(){}bool seek(size_t p){pos=p;return bool(*this);}
 size_t write(const uint8_t*p,size_t n){if(!*this)return 0;if(bytes->size()<pos+n)bytes->resize(pos+n);memcpy(bytes->data()+pos,p,n);pos+=n;return n;}
 size_t read(uint8_t*p,size_t n){if(!*this||pos>=bytes->size())return 0;n=std::min(n,bytes->size()-pos);memcpy(p,bytes->data()+pos,n);pos+=n;return n;}
};
struct LittleFSTest {bool ready=true;int formatCalls=0;std::map<std::string,std::shared_ptr<std::vector<uint8_t>>> files;
 size_t capacity=0x170000;size_t totalBytes(){return capacity;}size_t usedBytes(){size_t n=0;for(const auto& f:files)n+=f.second->size();return n;}
 bool begin(bool format){if(format){++formatCalls;files.clear();ready=true;}return ready;}bool exists(const char*p){return files.count(p);}
 bool remove(const char*p){return files.erase(p)>0;}
 bool rename(const char*a,const char*b){if(!files.count(a)||files.count(b))return false;files[b]=files[a];files.erase(a);return true;}
 File open(const char*p,const char*mode){if(!ready)return {};if(mode[0]=='w')files[p]=std::make_shared<std::vector<uint8_t>>();if(!files.count(p))return {};return File(files[p]);}
};inline LittleFSTest LittleFS;
