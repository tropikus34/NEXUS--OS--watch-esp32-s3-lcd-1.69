#include <cassert>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
int main(){setup();assert(batteryCapacityMah==600&&prefs.getBytesLength("bat_cap")==4);auto pins=fakeDigitalValues;auto tx=Wire.transactions;
 assert(batterySetCapacity(320));batteryCapacityMah=999;loadBatteryCapacity();assert(batteryCapacityMah==320);
 auto data=prefs.data["bat_cap"];prefs.data["bat_cap"]={};batteryCapacityMah=320;loadBatteryCapacity();assert(batteryCapacityMah==600);batteryCapacityMah=320;prefs.data["bat_cap"]=data;
 assert(!batterySetCapacity(0)&&!batterySetCapacity(10001)&&batteryCapacityMah==320);prefs.failWrites=true;assert(!batterySetCapacity(500)&&batteryCapacityMah==320);prefs.failWrites=false;
 auto command=[](const char* text){while(*text)Serial.input.push_back(*text++);while(Serial.available())themeSerialService();};
 Serial.output.clear();command("NXCAPACITY 320\nNXBATTERY\n");assert(Serial.output.find("OK CAPACITY")!=std::string::npos&&Serial.output.find("CAP_MAH=320")!=std::string::npos&&Serial.output.find("CAP_SOURCE=USER_CONFIG")!=std::string::npos&&Serial.output.find("CHARGING=UNKNOWN FULL=UNKNOWN CHARGE_CONTROL=HARDWARE")!=std::string::npos);
 Serial.output.clear();command("NXCAPACITY 0\nNXCAPACITY 320 garbage\n");assert(Serial.output.find("ERR CAPACITY")!=std::string::npos&&batteryCapacityMah==320);
 assert(fakeDigitalValues==pins&&Wire.transactions==tx); // capacity configuration never controls charger or I2C.
 bool valid=batPercent>=0;batPercent=-1;Serial.output.clear();printBatteryStatus();assert(Serial.output.find("VOLTAGE_VALID=0")!=std::string::npos);batPercent=valid?85:-1;
 currentScreen=SC_MENU;drawMenuScreen();gfx->save("/tmp/battery_capacity592.ppm");
 puts("PASS: configured 320mAh capacity, NVS roundtrip/corruption/write failure, USB validation, honest unknown charge/full state, no charger GPIO/I2C operations, battery UI");
}
