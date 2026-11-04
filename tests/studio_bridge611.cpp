#include <iostream>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
int main(int argc,char** argv){if(argc>1){if(!strcmp(argv[1],"--unformatted"))LittleFS.ready=false;else if(!strcmp(argv[1],"--no-partition"))fakeFsPartition=false;else if(!strcmp(argv[1],"--low-space"))LittleFS.capacity=100000;}setup();Serial.output.clear();char c;while(std::cin.get(c)){Serial.input.push_back((uint8_t)c);themeSerialService();if(!Serial.output.empty()){std::cout<<Serial.output<<std::flush;Serial.output.clear();fakeMs+=5;}}}
