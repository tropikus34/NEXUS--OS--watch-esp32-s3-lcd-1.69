#include <Arduino.h>
#include "NexusTypes.h"
#include "NexusLooks.h"
#include <algorithm>
#include "Arduino_GFX_Library.h"
#include <Wire.h>
#include <ChronosESP32.h>
#include <NimBLEDevice.h> // NimBLE-Arduino 2.x (same backend as current Chronos)
#include <time.h>
#include <utility>
#if defined(ESP32)
#include <esp_pm.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <esp_idf_version.h>
#endif
#include <Preferences.h>
#include <freertos/queue.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

/* ============================================================
   NEXUS OS 6.9.3 — WATCH UI
   ESP32-S3-LCD-1.69 V2 · Chronos + одна кнопка BOOT
   ============================================================ */

/* ---------- Пины ---------- */
#define LCD_WIDTH   240
#define LCD_HEIGHT  280
#define LCD_DC      4
#define LCD_CS      5
#define LCD_SCK     6
#define LCD_MOSI    7
#define LCD_RST     8
#define LCD_BL      15
#define I2C_SDA     11
#define I2C_SCL     10
#define BUZZER      42
#define BAT_ADC     1
#define BTN_PIN     0
#define RTC_ADDR    0x51
// Waveshare ESP32-S3-LCD-1.69 V2 only: battery power latch.
#define SYS_EN      41
// Loop task is woken by the button ISR instead of 50 Hz polling while the screen is off.
TaskHandle_t nexusButtonLoopTaskHandle=nullptr;
void IRAM_ATTR buttonIsr(){gpio_intr_disable((gpio_num_t)BTN_PIN);BaseType_t w=pdFALSE;if(nexusButtonLoopTaskHandle)vTaskNotifyGiveFromISR(nexusButtonLoopTaskHandle,&w);if(w)portYIELD_FROM_ISR();}

/* ---------- Дисплей ---------- */
Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
Arduino_GFX *lcd = new Arduino_ST7789(bus, LCD_RST, 0, true,
                                      LCD_WIDTH, LCD_HEIGHT, 0, 20, 0, 20);
#include "NexusFrame.h"
#include "NexusRender.h"

#include "NexusTypography.h"

/* ---------- Палитра ---------- */
#define COL_BG        0x0000
#define COL_ACCENT    themeAccent
#define COL_ACCENT2   themeSecondary
#define COL_TEXT      themeText
#define COL_DIM       themeDim
#define COL_WARN      (themeIndex==5 ? 0x9820:0xFD20)
#define COL_OK        0x07E0
#define COL_EDIT      0xFFE0
#define COL_BLE       0x001F

uint16_t customAccent=0x07FF,customSecondary=0xFFFF;
uint16_t customBg=0,customPanel=0x18C3,customText=0xFFFF,customMuted=0xAD55;
uint8_t faceVariant=0;bool paletteOverride=false,faceTop=true,faceDate=true;uint8_t cardRadius=22;
bool faceSeconds=true;uint8_t faceCompA=0,faceCompB=0,wallpaperMix=25;
uint16_t notificationToneHz=1500,notificationToneMs=180;
uint16_t themeAccent=0x07FF,themeSecondary=0xF81F,themeDim=0x8410;
uint8_t themeIndex=0;
char ownerName[9]="NEXUS", ownerDraft[9]="NEXUS";
char bleName[20]="NEXUS-NEXUS";
char savedPeer[18]={};
int profileCursor=0;
bool autoReconnect=true, peerBonded=false, securityRequested=false;
uint32_t bleCheckAt=0, pairWindowStart=0;
bool pairWindowActive=false, forgetFailed=false;
void drawProfile();
void bleService();
void forgetPhone();
void loadPersonalization();
bool setSpecialFromUsb(int index);
void applyTheme() {
 const WatchLook& l=(faceVariant==7?CLASSIC_LOOKS:WATCH_LOOKS)[themeIndex<7 ? themeIndex:0];
 themeBg=l.bg;themePanel=l.panel;themeText=l.text;themeMuted=l.muted;
 themeAccent=(themeIndex==7||paletteOverride) ? customAccent:l.accent;themeSecondary=(themeIndex==7||paletteOverride) ? customSecondary:l.secondary;
 if(themeIndex==7||paletteOverride){themeBg=customBg;themePanel=customPanel;themeText=customText;themeMuted=customMuted;}
 themeDim=themeMuted;uiFontBg=themeBg;
}
/* ---------- Chronos ---------- */
ChronosESP32 watch("NEXUS OS");

/* ---------- Экраны и режимы ---------- */
// Explicit prototype prevents Arduino from generating it before Screen is known.
void showScreen(Screen s);

Screen currentScreen = SC_CLOCK;
Mode   currentMode   = MODE_NAV;
const char* SCR_NAMES[SC_COUNT] = {"CLK", "ALM", "TMR", "MSG", "SET", "SYS", "DATE", "NAME", "TOOL", "STYLE", "ACT", "HEALTH", "SECTIONS"};

/* ---------- Время ---------- */
int hh = 12, mm = 0, ss = 0;
int wday = 4, day = 15, mon = 10, yr = 26;
uint32_t lastTick = 0;
bool timeValid = false;
bool rtcPresent = false;
bool colonOn = true;
bool rtcOK = false;

/* ---------- Настройки ---------- */
bool use24h     = true;
bool colonBlink = true;
uint8_t brightness = 128; // moderate default, existing user setting still loads

/* ---------- Будильник ---------- */
int  alarmH = 7;
int  alarmM = 30;
bool alarmEnabled = false;
bool alarmRinging = false;
unsigned long alarmStart = 0;
constexpr int ALARM_COUNT=5;
uint8_t alarmHours[ALARM_COUNT]={7,8,9,10,11};
uint8_t alarmMinutes[ALARM_COUNT]={30,0,0,0,0};
bool alarmEnabledBank[ALARM_COUNT]={};
uint32_t alarmDates[ALARM_COUNT]={};
int alarmSlot=0, timeTool=0, ringingAlarmSlot=-1;
void syncAlarmSlot() {
  alarmHours[alarmSlot]=alarmH; alarmMinutes[alarmSlot]=alarmM;
  alarmEnabledBank[alarmSlot]=alarmEnabled;
}

/* ---------- Курсоры ---------- */
int alarmCursor = 0;
int settingsCursor = 0;
int styleCursor=0;
#include "NexusNavigation.h"


// Divider retained from the supplied sketch. Calibrate against a multimeter.
constexpr float BAT_DIVIDER = 3.0f;
// User-specified rated capacity; not an automatically measured capacity.
uint16_t batteryCapacityMah=600;
int batPercent = -1;
bool chronosConnected = false;

/* ---------- Флаги отложенной обработки (важно для BLE) ---------- */
portMUX_TYPE eventMux = portMUX_INITIALIZER_UNLOCKED;
bool timeUpdatePending = false;
struct tm pendingTime = {};
bool connChangePending = false;
bool pendingConnState = false;
bool formatPending = false, pending24h = true;
bool findPending = false;

/* New features use bounded RAM; message history is not written to flash. */
Preferences prefs;
bool prefsOK = false, settingsDirty = false;
uint32_t settingsChangedAt = 0;
uint8_t screenTimeoutIndex = 0; // 15 s default for first boot
const uint16_t SCREEN_TIMEOUTS[] = {15, 30, 60, 120, 0};
bool screenAwake = true;
uint32_t lastActivity = 0;
bool uiDirty = false;
bool quietMode=false, privatePreview=false;
bool doNotDisturb=false,ecoMode=true,lcdSleeping=false,powerClockOK=true;
bool pendingPreview=false;
volatile uint32_t notificationRx=0,notificationLastRx=0;
void powerService();
void suspendDisplay();
bool injectTestNotification();
void printRuntimeStatus();
void loadRuntimeSettings();
float batteryVolts=0;
bool batteryLow=false;float batteryRawVolts=0;uint32_t batterySampleAt=0;
volatile uint32_t droppedMessages=0;
uint32_t lastRtcPoll=0;
int timeCursor=0;
int draftH=0,draftM=0,draftD=1,draftMo=1,draftY=26;
uint32_t restoredDeadline=0;
bool restoredTimerPending=false;
bool timerCheckpointDirty=false;
uint32_t timerCheckpointAt=0;
int unreadMessages=0;
void drawTimeSettings();
void checkpointTimer();
void restoreTimer();
uint32_t clockEpoch();
void commitDraftTime();


uint16_t timerPresetSeconds = 300;
uint32_t timerRemainingMs = 300000, timerLastUpdate = 0;
bool timerRunning = false, timerRinging = false;
uint32_t timerAlarmStart = 0;
int timerCursor = 0;

struct MessageRecord {
  char app[40];
  char title[96];
  char body[512];
  uint32_t receivedAt;
  bool unread;
  char stamp[6];
};
QueueHandle_t messageQueue = nullptr;
constexpr int MESSAGE_HISTORY = 5;
MessageRecord messages[MESSAGE_HISTORY] = {};
int messageCount = 0, messageIndex = 0, messagePage = 0;
bool previewActive = false;
uint32_t previewStart = 0;
MessageRecord previewMessage = {};

/* ---------- Дни недели ---------- */
const char* WD_NAMES[7]  = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
const char* MON_NAMES[12]= {"JAN","FEB","MAR","APR","MAY","JUN",
                            "JUL","AUG","SEP","OCT","NOV","DEC"};

/* ============================================================
   ПРОТОТИПЫ
   ============================================================ */
void redrawCurrentScreen();
void drawStatusRows();
void drawClockScreen();
void beep(int f, int ms);
void drawTimerScreen();
void drawMessagesScreen();
void drawAlertScreen();
void drawNotificationPreview();
void markSettingsDirty();
bool saveSettings();
void wakeDisplay();
void cancelPendingButtonAction();
void timerCheck();
void stopAlerts();
int daysInMonth(int month,int year);
bool activitySetProfile(unsigned height,unsigned weight,unsigned stride,unsigned goal,unsigned interval);
void printActivityStatus();
bool batterySetCapacity(unsigned capacity);
void printBatteryStatus();
void printPowerStatus();
void powerSerialActivity();
bool studioHandleCommand(const char* command);
// Explicit declaration keeps Arduino prototype generation away from defaults.
const char* messageLineEnd(const char* start,const char*& next,int maxWidth=216);

#include "NexusOsd.h"
#include "NexusTheme.h"
#include "NexusSpecial.h"
#include "NexusActivity.h"

/* ============================================================
   RTC PCF85063
   ============================================================ */
uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

int daysInMonth(int month, int year) {
  static const uint8_t lengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month < 1 || month > 12) return 0;
  const int fullYear = 2000 + year;
  const bool leap = (fullYear % 4 == 0 && (fullYear % 100 != 0 || fullYear % 400 == 0));
  return lengths[month - 1] + (month == 2 && leap ? 1 : 0);
}

bool validTime(int h, int m, int sec, int wd, int d, int mo, int y) {
  return h >= 0 && h < 24 && m >= 0 && m < 60 && sec >= 0 && sec < 60 &&
         wd >= 0 && wd < 7 && y >= 0 && y <= 99 && mo >= 1 && mo <= 12 &&
         d >= 1 && d <= daysInMonth(mo, y);
}

bool rtcReadTime() {
  ActivityBusLock lock;if(!lock)return false;
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(0x04);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)RTC_ADDR, (uint8_t)7) != 7) return false;
  uint8_t raw[7];
  for (int i = 0; i < 7; ++i) raw[i] = Wire.read();
  if (raw[0] & 0x80) return false; // oscillator-stop flag: time is untrusted
  const uint8_t masks[] = {0x7F,0x7F,0x3F,0x3F,0x07,0x1F,0xFF};
  for (int i = 0; i < 7; ++i) {
    uint8_t v = raw[i] & masks[i];
    if (i != 4 && ((v & 15) > 9 || (v >> 4) > 9)) return false;
  }
  int ns = bcd2dec(raw[0] & 0x7F), nm = bcd2dec(raw[1] & 0x7F);
  int nh = bcd2dec(raw[2] & 0x3F), nd = bcd2dec(raw[3] & 0x3F);
  int nw = raw[4] & 7, nmo = bcd2dec(raw[5] & 0x1F), ny = bcd2dec(raw[6]);
  if (!validTime(nh,nm,ns,nw,nd,nmo,ny)) return false;
  hh=nh; mm=nm; ss=ns; wday=nw; day=nd; mon=nmo; yr=ny;
  return true;
}

bool rtcWriteTime(int h,int m,int sec,int wd,int d,int mo,int y) {
  ActivityBusLock lock;if(!lock)return false;
  if (!validTime(h,m,sec,wd,d,mo,y)) return false;
  Wire.beginTransmission(RTC_ADDR); Wire.write(0x04);
  Wire.write(dec2bcd(sec)); Wire.write(dec2bcd(m)); Wire.write(dec2bcd(h));
  Wire.write(dec2bcd(d)); Wire.write(wd);
  Wire.write(dec2bcd(mo)); Wire.write(dec2bcd(y));
  return Wire.endTransmission() == 0;
}

void advanceSecond() {
  if (++ss < 60) return;
  ss = 0;
  if (++mm < 60) return;
  mm = 0;
  if (++hh < 24) return;
  hh = 0;
  wday = (wday + 1) % 7;
  if (++day <= daysInMonth(mon, yr)) return;
  day = 1;
  if (++mon <= 12) return;
  mon = 1; yr = (yr + 1) % 100;
}

void beep(int f, int ms) { if (!quietMode && !alarmRinging && !timerRinging) tone(BUZZER, f, ms); }

// Approximate open-circuit curve. USB/charging/load can bias this estimate.
int batteryCurve(float v) {
  const float volts[]={3.20f,3.40f,3.60f,3.70f,3.75f,3.80f,3.85f,3.90f,4.00f,4.10f,4.20f};
  const int pct[]={0,5,10,20,30,40,50,60,75,90,100};
  if(v<=volts[0]) return 0;
  for(int i=1;i<11;++i) if(v<=volts[i])
    return pct[i-1]+(int)((v-volts[i-1])*(pct[i]-pct[i-1])/(volts[i]-volts[i-1]));
  return 100;
}
int readBattery() {
  uint32_t total=0;
  for(int i=0;i<16;++i) total+=analogReadMilliVolts(BAT_ADC);
  float v=(total/16.0f)*BAT_DIVIDER/1000.0f;
  batteryRawVolts=v;batterySampleAt=millis();
  if(v<2.5f || v>4.5f) { batteryVolts=0; batteryLow=false; return -1; }
  batteryVolts=batteryVolts ? batteryVolts*0.75f+v*0.25f : v;
  batteryLow=batteryLow ? batteryVolts<3.60f : batteryVolts<3.45f;
  return batteryCurve(batteryVolts);
}

/* BLE callbacks only publish events. Display, I2C and buzzer run in loop(). */
void connectionCallback(bool state) {
  portENTER_CRITICAL(&eventMux);
  pendingConnState = state;
  connChangePending = true;
  portEXIT_CRITICAL(&eventMux);
}

// Class members are not rewritten into free-function prototypes by Arduino.
// The dependent return type names T, not a function parameter.
struct NotificationTitleReader {
  template <typename T>
  static decltype((void)std::declval<const T&>().title.c_str())
  copy(const T& notification, char* out, size_t size, int) {
    snprintf(out, size, "%s", notification.title.c_str());
  }

  // Chronos versions without Notification::title use an empty heading.
  template <typename T>
  static void copy(const T&, char* out, size_t, long) {
    out[0] = '\0';
  }
};

void notificationCallback(Notification notification) {
  portENTER_CRITICAL(&eventMux);++notificationRx;notificationLastRx=millis();portEXIT_CRITICAL(&eventMux);
  if (!messageQueue) {portENTER_CRITICAL(&eventMux);++droppedMessages;portEXIT_CRITICAL(&eventMux);return;}
  MessageRecord value = {};
  snprintf(value.app, sizeof(value.app), "%s", notification.app.c_str());
  NotificationTitleReader::copy(notification, value.title, sizeof(value.title), 0);
  snprintf(value.body, sizeof(value.body), "%s", notification.message.c_str());
  value.receivedAt = millis();
  // Keep the newest message when the bounded queue fills; never block BLE.
  if(xQueueSend(messageQueue,&value,0)!=pdTRUE) {
    MessageRecord discarded={};xQueueReceive(messageQueue,&discarded,0);
    portENTER_CRITICAL(&eventMux);++droppedMessages;portEXIT_CRITICAL(&eventMux);
    xQueueSend(messageQueue,&value,0);
  }
}

void configCallback(Config config, uint32_t a, uint32_t b) {
  (void)a;
  if (config == CF_TIME) {
    // One snapshot avoids mixing fields across a minute/day boundary.
    struct tm value = watch.getTimeStruct();
    portENTER_CRITICAL(&eventMux);
    pendingTime = value;
    timeUpdatePending = true;
    portEXIT_CRITICAL(&eventMux);
  } else {
    portENTER_CRITICAL(&eventMux);
    if (config == CF_HR24) { pending24h = (b == 1); formatPending = true; }
    if (config == CF_FIND) findPending = true;
    portEXIT_CRITICAL(&eventMux);
  }
}

/* ---------------- Settings: versioned NVS values with retry on failure ---------------- */
void markSettingsDirty() {
  settingsDirty = true;
  settingsChangedAt = millis();
}

void loadAlarmBank() {
  syncAlarmSlot(); // migrate the existing single alarm into slot 1
  uint8_t bank[16]={};
  if(prefs.getBytesLength("alarms")!=sizeof(bank) || prefs.getBytes("alarms",bank,sizeof(bank))!=sizeof(bank) || bank[0]!=1) return;
  for(int i=0;i<ALARM_COUNT;++i) if(bank[1+i*3]>23 || bank[2+i*3]>59 || bank[3+i*3]>1) return;
  for(int i=0;i<ALARM_COUNT;++i) { alarmHours[i]=bank[1+i*3]; alarmMinutes[i]=bank[2+i*3]; alarmEnabledBank[i]=bank[3+i*3]; }
  alarmH=alarmHours[0];alarmM=alarmMinutes[0];alarmEnabled=alarmEnabledBank[0];
}


void loadBatteryCapacity() {
 uint16_t value[2]={};batteryCapacityMah=600;
 if(prefs.getBytesLength("bat_cap")==sizeof(value) && prefs.getBytes("bat_cap",value,sizeof(value))==sizeof(value) && value[0]==1 && value[1]>=10 && value[1]<=10000)batteryCapacityMah=value[1];
 else if(!batterySetCapacity(600))Serial.println("Battery capacity profile save failed");
}
bool batterySetCapacity(unsigned capacity) {
 if(capacity<10 || capacity>10000 || !prefsOK)return false;
 uint16_t value[2]={1,(uint16_t)capacity};
 if(prefs.putBytes("bat_cap",value,sizeof(value))!=sizeof(value))return false;
 batteryCapacityMah=capacity;uiDirty=true;return true;
}
void printBatteryStatus() {
 Serial.printf("BATTERY CAP_MAH=%u CAP_SOURCE=USER_CONFIG V=%.3f SOC_APPROX=%d VOLTAGE_VALID=%d CHARGING=UNKNOWN FULL=UNKNOWN CHARGE_CONTROL=HARDWARE\n",batteryCapacityMah,batteryVolts,batPercent,batPercent>=0);
}

void loadSettings() {
  prefsOK = prefs.begin("nexus-os", false);
  if (!prefsOK) { Serial.println("NVS unavailable: settings remain in RAM"); return; }
  loadBatteryCapacity();
  quietMode=prefs.getBool("quiet",false);
  privatePreview=prefs.getBool("private",false);
  uint8_t data[12] = {};
  if (prefs.getBytesLength("config") != sizeof(data) ||
      prefs.getBytes("config", data, sizeof(data)) != sizeof(data)) return;
  const uint16_t preset = data[10] | ((uint16_t)data[11] << 8);
  if (data[0] != 1 || data[1] < 32 || data[2] > 1 || data[3] > 1 ||
      data[4] > 1 || data[5] >= 5 || data[6] > 23 || data[7] > 59 ||
      data[8] > 1 || preset < 1 || preset > 5999) return;
  brightness=data[1]; use24h=data[2]; colonBlink=data[3];
  screenTimeoutIndex=data[5]; alarmH=data[6]; alarmM=data[7]; alarmEnabled=data[8];
  timerPresetSeconds=preset; timerRemainingMs=(uint32_t)preset*1000;
  // Retain the old NVS layout, but clear its obsolete auto-cycle bit.
  if (data[4]) markSettingsDirty();
}

bool saveSettings() {
  if (!settingsDirty) return true;
  if (!prefsOK) return false;
  uint8_t data[12] = {1, brightness, (uint8_t)use24h, (uint8_t)colonBlink,
    0, screenTimeoutIndex, (uint8_t)alarmH, (uint8_t)alarmM,
    (uint8_t)alarmEnabled, 0, (uint8_t)timerPresetSeconds,
    (uint8_t)(timerPresetSeconds >> 8)};
  if (prefs.putBytes("config", data, sizeof(data)) != sizeof(data)) {
    Serial.println("NVS save failed; will retry");
    settingsChangedAt = millis();
    return false;
  }
  if(prefs.putBool("quiet",quietMode)!=1 || prefs.putBool("private",privatePreview)!=1) {
    settingsChangedAt=millis(); return false;
  }
  uint8_t personal[12]={2,themeIndex,(uint8_t)autoReconnect};
  memcpy(personal+3,ownerName,9);
  if(prefs.putBytes("personal",personal,sizeof(personal))!=sizeof(personal)) {settingsChangedAt=millis();return false;}
  uint8_t appearance[2]={(uint8_t)wallpaperEnabled,clockStyle};
  if(prefs.putBytes("appear",appearance,sizeof(appearance))!=sizeof(appearance)) {settingsChangedAt=millis();return false;}
  if(prefs.putBytes("variant",&faceVariant,1)!=1){settingsChangedAt=millis();return false;}
  syncAlarmSlot();
  uint8_t bank[16]={1};
  for(int i=0;i<ALARM_COUNT;++i) { bank[1+i*3]=alarmHours[i];bank[2+i*3]=alarmMinutes[i];bank[3+i*3]=alarmEnabledBank[i]; }
  if(prefs.putBytes("alarms",bank,sizeof(bank))!=sizeof(bank)) { settingsChangedAt=millis();return false; }
  uint8_t runtime[3]={1,(uint8_t)ecoMode,(uint8_t)doNotDisturb};
  if(prefs.putBytes("runtime",runtime,sizeof(runtime))!=sizeof(runtime)) {settingsChangedAt=millis();return false;}
  if(!saveSpecialization()) {settingsChangedAt=millis();return false;}
  if(!activitySaveProfile()) {settingsChangedAt=millis();return false;}
  settingsDirty = false;
  return true;
}

#include "NexusPower.h"

void loadRuntimeSettings() {
  uint8_t config[3]={};bool restored=false;
  if(prefsOK && prefs.getBytesLength("runtime")==sizeof(config) && prefs.getBytes("runtime",config,sizeof(config))==sizeof(config) && config[0]==1 && config[1]<=1 && config[2]<=1) {ecoMode=config[1];doNotDisturb=config[2];restored=true;}
  if(!restored) {brightness=std::min<uint8_t>(brightness,128);screenTimeoutIndex=0;markSettingsDirty();}
}
bool injectTestNotification() {
  if(!messageQueue) return false;
  MessageRecord test={};snprintf(test.app,sizeof(test.app),"NEXUS TEST");
  snprintf(test.title,sizeof(test.title),"Проверка экрана");
  snprintf(test.body,sizeof(test.body),"Если это сообщение видно, показ уведомлений работает. Для реальных сообщений проверьте подключение Chronos и доступ к уведомлениям Android.");
  test.receivedAt=millis();return xQueueSend(messageQueue,&test,0)==pdTRUE;
}
void printRuntimeStatus() {
  Serial.printf("STATUS LINK=%d SUB=%d RX=%lu LOST=%lu SILENT=%d DND=%d ECO=%d CPU=%lu CLOCK_OK=%d LCD_SLEEP=%d BAT_V=%.3f\n",chronosConnected,watch.isSubscribed(),(unsigned long)notificationRx,(unsigned long)droppedMessages,quietMode,doNotDisturb,ecoMode,(unsigned long)getCpuFrequencyMhz(),powerClockOK,lcdSleeping,batteryVolts);
}

// One 8x12 glyph per codepoint in U+0400..U+045F, stored in flash.
static const uint8_t CYRILLIC_FONT[96][12] = {
  {0x0,0x7c,0x40,0x40,0x7c,0x40,0x40,0x40,0x7c,0x0,0x0,0x0},
  {0x0,0x7c,0x40,0x40,0x7c,0x40,0x40,0x40,0x7c,0x0,0x0,0x0},
  {0x0,0xfc,0x20,0x20,0x3c,0x22,0x22,0x22,0x22,0x2,0xc,0x0},
  {0x0,0x7c,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x1c,0x22,0x40,0x7c,0x40,0x40,0x22,0x1c,0x0,0x0,0x0},
  {0x0,0x3c,0x42,0x40,0x78,0x6,0x2,0x42,0x3c,0x0,0x0,0x0},
  {0x0,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x80,0x0},
  {0x0,0x3e,0x22,0x22,0x23,0x22,0x22,0x42,0x83,0x0,0x0,0x0},
  {0x0,0x42,0x42,0x42,0x7f,0x42,0x42,0x42,0x43,0x0,0x0,0x0},
  {0x0,0xfc,0x20,0x20,0x3c,0x22,0x22,0x22,0x22,0x0,0x0,0x0},
  {0x0,0x42,0x44,0x48,0x58,0x68,0x44,0x42,0x43,0x0,0x0,0x0},
  {0x0,0x46,0x46,0x4a,0x4a,0x52,0x52,0x62,0x62,0x0,0x0,0x0},
  {0x0,0x84,0x48,0x48,0x28,0x30,0x30,0x20,0x60,0x0,0x0,0x0},
  {0x0,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x7f,0x8,0x8,0x0},
  {0x0,0x10,0x28,0x28,0x28,0x44,0x7c,0x44,0x82,0x0,0x0,0x0},
  {0x0,0x7c,0x40,0x40,0x7c,0x42,0x42,0x42,0x7c,0x0,0x0,0x0},
  {0x0,0x7c,0x42,0x42,0x7c,0x42,0x42,0x42,0x7c,0x0,0x0,0x0},
  {0x0,0x7c,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x1f,0x11,0x11,0x11,0x11,0x11,0x21,0x7f,0x40,0x40,0x0},
  {0x0,0x7c,0x40,0x40,0x7c,0x40,0x40,0x40,0x7c,0x0,0x0,0x0},
  {0x0,0xc4,0x64,0x35,0x1f,0x2e,0x24,0x44,0x84,0x0,0x0,0x0},
  {0x0,0x3c,0x42,0x2,0x1c,0x2,0x2,0x42,0x3c,0x0,0x0,0x0},
  {0x0,0x46,0x46,0x4a,0x4a,0x52,0x52,0x62,0x62,0x0,0x0,0x0},
  {0x0,0x46,0x46,0x4a,0x4a,0x52,0x52,0x62,0x62,0x0,0x0,0x0},
  {0x0,0x42,0x44,0x48,0x58,0x68,0x44,0x42,0x43,0x0,0x0,0x0},
  {0x0,0x3e,0x22,0x22,0x22,0x22,0x22,0x42,0x82,0x0,0x0,0x0},
  {0x0,0x41,0x63,0x63,0x55,0x55,0x49,0x41,0x41,0x0,0x0,0x0},
  {0x0,0x42,0x42,0x42,0x7e,0x42,0x42,0x42,0x42,0x0,0x0,0x0},
  {0x0,0x1c,0x22,0x41,0x41,0x41,0x41,0x22,0x1c,0x0,0x0,0x0},
  {0x0,0x7e,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x0,0x0,0x0},
  {0x0,0x78,0x44,0x44,0x44,0x78,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x1c,0x22,0x40,0x40,0x40,0x40,0x22,0x1c,0x0,0x0,0x0},
  {0x0,0xfe,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x0,0x0,0x0},
  {0x0,0x84,0x48,0x48,0x28,0x30,0x30,0x20,0x60,0x0,0x0,0x0},
  {0x0,0x4,0x1f,0x64,0x44,0x44,0x64,0x1f,0x4,0x0,0x0,0x0},
  {0x0,0xc6,0x44,0x28,0x10,0x10,0x28,0x44,0xc6,0x0,0x0,0x0},
  {0x0,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x7f,0x1,0x1,0x0},
  {0x0,0x42,0x42,0x42,0x3e,0x2,0x2,0x2,0x2,0x0,0x0,0x0},
  {0x0,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x7f,0x0,0x0,0x0},
  {0x0,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x7f,0x0,0x0,0x0},
  {0x0,0x70,0x10,0x10,0x1f,0x10,0x10,0x10,0x1f,0x0,0x0,0x0},
  {0x0,0x41,0x41,0x41,0x7d,0x43,0x43,0x43,0x7d,0x0,0x0,0x0},
  {0x0,0x40,0x40,0x40,0x7c,0x42,0x42,0x42,0x7c,0x0,0x0,0x0},
  {0x0,0x38,0x44,0x2,0x3e,0x2,0x2,0x44,0x38,0x0,0x0,0x0},
  {0x0,0x47,0x4c,0x48,0x78,0x48,0x48,0x44,0x43,0x0,0x0,0x0},
  {0x0,0x3e,0x42,0x42,0x7e,0x12,0x22,0x22,0x42,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x38,0x4,0x3c,0x44,0x44,0x3c,0x0,0x0,0x0},
  {0x20,0x40,0x78,0x44,0x44,0x44,0x44,0x38,0x0,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x7c,0x44,0x7c,0x44,0x44,0x78,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x78,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x1c,0x14,0x14,0x14,0x24,0x7e,0x42,0x42,0x0},
  {0x0,0x0,0x0,0x38,0x44,0x7c,0x40,0x64,0x38,0x0,0x0,0x0},
  {0x0,0x0,0x0,0xc9,0x6b,0x3e,0x3e,0x49,0x88,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x78,0x8,0x30,0x8,0x8,0x70,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x4c,0x4c,0x54,0x54,0x64,0x64,0x0,0x0,0x0},
  {0x28,0x38,0x0,0x4c,0x4c,0x54,0x54,0x64,0x64,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x4c,0x58,0x70,0x70,0x48,0x44,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x3c,0x24,0x24,0x24,0x64,0xc4,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x66,0x66,0x5a,0x5a,0x5a,0x42,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x44,0x44,0x7c,0x44,0x44,0x44,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x38,0x44,0x44,0x44,0x44,0x38,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x7c,0x44,0x44,0x44,0x44,0x44,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x78,0x44,0x44,0x44,0x44,0x78,0x40,0x40,0x0},
  {0x0,0x0,0x0,0x38,0x40,0x40,0x40,0x40,0x38,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x7c,0x10,0x10,0x10,0x10,0x10,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x84,0x84,0x48,0x48,0x30,0x30,0x20,0xc0,0x0},
  {0x4,0x4,0x4,0x3f,0x44,0x44,0x44,0x44,0x3f,0x4,0x4,0x0},
  {0x0,0x0,0x0,0x84,0x48,0x30,0x30,0x48,0x84,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x44,0x44,0x44,0x44,0x44,0x7e,0x2,0x2,0x0},
  {0x0,0x0,0x0,0x44,0x44,0x44,0x3c,0x4,0x4,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x44,0x44,0x44,0x44,0x44,0x7f,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x44,0x44,0x44,0x44,0x44,0x7f,0x0,0x0,0x0},
  {0x0,0x0,0x0,0xe0,0x20,0x3c,0x22,0x22,0x3c,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x41,0x41,0x79,0x45,0x45,0x79,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x40,0x40,0x78,0x44,0x44,0x78,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x70,0x8,0x38,0x8,0x8,0x70,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x4e,0x51,0x71,0x51,0x51,0x4e,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x3c,0x44,0x44,0x3c,0x24,0x44,0x0,0x0,0x0},
  {0x40,0x20,0x0,0x38,0x44,0x7c,0x40,0x64,0x38,0x0,0x0,0x0},
  {0x0,0x28,0x0,0x38,0x44,0x7c,0x40,0x64,0x38,0x0,0x0,0x0},
  {0x40,0x40,0xf8,0x40,0x40,0x78,0x44,0x44,0x44,0x8,0x10,0x0},
  {0x8,0x10,0x0,0x78,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x38,0x40,0x78,0x40,0x40,0x38,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x38,0x44,0x70,0xc,0x44,0x38,0x0,0x0,0x0},
  {0x0,0x40,0x0,0x40,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0xa0,0x0,0x40,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x40,0x0,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0xc0,0x0},
  {0x0,0x0,0x0,0x3c,0x24,0x27,0x24,0x64,0xc7,0x0,0x0,0x0},
  {0x0,0x0,0x0,0x44,0x44,0x7f,0x44,0x44,0x47,0x0,0x0,0x0},
  {0x40,0x40,0xf8,0x40,0x40,0x78,0x44,0x44,0x44,0x0,0x0,0x0},
  {0x8,0x10,0x0,0x4c,0x58,0x70,0x70,0x48,0x44,0x0,0x0,0x0},
  {0x20,0x10,0x0,0x4c,0x4c,0x54,0x54,0x64,0x64,0x0,0x0,0x0},
  {0x28,0x38,0x0,0x84,0x84,0x48,0x48,0x30,0x30,0x20,0xc0,0x0},
  {0x0,0x0,0x0,0x44,0x44,0x44,0x44,0x44,0x7c,0x10,0x10,0x0},
};

static const uint8_t UKRAINIAN_EXTRA[2][12] = {
  {0x4,0x7c,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
  {0x0,0x8,0x8,0x78,0x40,0x40,0x40,0x40,0x40,0x0,0x0,0x0},
};

uint32_t nextCodepoint(const char*& p) {
  uint8_t c = (uint8_t)*p++;
  if (c < 0x80) return c;
  int count = 0;
  uint32_t cp;
  if ((c & 0xE0) == 0xC0) { count=1; cp=c&0x1F; }
  else if ((c & 0xF0) == 0xE0) { count=2; cp=c&0x0F; }
  else if ((c & 0xF8) == 0xF0) { count=3; cp=c&7; }
  else return '?';
  for (int i=0; i<count; ++i) {
    uint8_t n=(uint8_t)*p;
    if (!n || (n & 0xC0) != 0x80) return '?';
    cp=(cp<<6)|(n&0x3F); ++p;
  }
  if ((count==1 && cp<0x80) || (count==2 && cp<0x800) ||
      (count==3 && cp<0x10000) || cp>0x10FFFF || (cp>=0xD800 && cp<=0xDFFF)) return '?';
  return cp;
}

void drawGlyph(uint32_t cp,int x,int y,uint16_t color) {premiumDraw(cp,x,y,1,color,themePanel);}
// Prefer word boundaries; long URLs/words still wrap by full UTF-8 codepoints.
const char* messageLineEnd(const char* start,const char*& next,int maxWidth) {
 const char* p=start;const char* space=nullptr;int width=0;
 while(*p){const char* cpStart=p;uint32_t cp=nextCodepoint(p);
  if(cp=='\r')continue;
  if(cp=='\n'){next=p;return cpStart;}
  if(cp==' ' || cp=='\t')space=cpStart;
  int advance=premiumAdvance(cp=='\t' ? ' ':cp);
  if(width+advance>maxWidth){
   if(space && space>start){next=space;while(*next==' ' || *next=='\t' || *next=='\r')++next;return space;}
   next=cpStart>start ? cpStart:p;return next;
  }
  width+=advance;
 }
 next=p;return p;
}
int drawTextPage(const char* text,int page,int x,int y,int rows,uint16_t color) {
 if(rows<=0)return 1;
 const char* p=text;int line=0;
 while(*p){const char* next;const char* end=messageLineEnd(p,next);int width=0;
  while(p<end){uint32_t cp=nextCodepoint(p);if(cp=='\r')continue;if(cp=='\t')cp=' ';
   if(page>=0 && line>=page*rows && line<(page+1)*rows)drawGlyph(cp,x+width,y+(line-page*rows)*16,color);
   width+=premiumAdvance(cp);
  }
  p=next;++line;
 }
 return std::max(1,(line+rows-1)/rows);
}
void drawTextLine(const char* text,int x,int y,uint16_t color) {
 const char* p=text;while(*p) {uint32_t cp=nextCodepoint(p);int a=premiumAdvance(cp);if(x+a>228) break;drawGlyph(cp,x,y,color);x+=a;}
}

#include "NexusUI.h"

/* ============================================================
   ПЕРЕКЛЮЧЕНИЕ ЭКРАНОВ
   ============================================================ */
void redrawCurrentScreen() {
  if (!screenAwake) return;
  if (alarmRinging || timerRinging) { drawAlertScreen(); return; }
  if (previewActive) { drawNotificationPreview(); return; }
  if(moveReminderActive) {drawMovementReminder();return;}
  switch (currentScreen) {
    case SC_CLOCK:    drawClockScreen();    break;
    case SC_ALARM:    drawAlarmScreen();    break;
    case SC_TIMER:    drawTimerScreen();    break;
    case SC_MESSAGES: drawMessagesScreen(); break;
    case SC_SETTINGS: drawSettingsScreen(); break;
    case SC_MENU:     drawMenuScreen();break;
    case SC_TIME: drawTimeSettings(); break;
    case SC_PROFILE: drawProfile();break;
    case SC_SPECIAL: drawSpecialization();break;
    case SC_STYLE: drawStylePicker();break;
    case SC_ACTIVITY: drawActivityScreen();break;
    case SC_HEALTH: drawActivityProfile();break;
    case SC_SETTINGS_SECTIONS: drawSettingsSections();break;
    default: break;
  }
}

void showScreen(Screen s) {
  currentScreen = s;
  currentMode = MODE_NAV;
  settingsActiveSection=-1;settingsReturnToSections=false;
  alarmCursor = 0;
  settingsCursor = 0;
  timerCursor = 0;
  redrawCurrentScreen();
}

void nextScreen() {showScreen(navigationPage((navigationIndex(currentScreen)+1)%7));}
void previousScreen() {showScreen(navigationPage((navigationIndex(currentScreen)+6)%7));}
#include "NexusTouch.h"
void returnToSettingsSections() {
 settingsActiveSection=-1;settingsReturnToSections=false;
 currentScreen=SC_SETTINGS_SECTIONS;currentMode=MODE_EDIT;redrawCurrentScreen();
}
void openSettingsSection() {
 const SettingsSection& section=SETTINGS_SECTIONS[settingsSectionCursor];
 if(settingsSectionCursor==8){showScreen(SC_SETTINGS);return;}
 settingsReturnToSections=true;settingsSectionStep=0;settingsActiveSection=-1;
 if(settingsSectionCursor==4){currentScreen=SC_HEALTH;activityProfileCursor=0;}
 else if(settingsSectionCursor==7){currentScreen=SC_TIME;draftH=hh;draftM=mm;draftD=day;draftMo=mon;draftY=yr;timeCursor=0;}
 else if(settingsSectionCursor==6){memset(ownerDraft,' ',8);ownerDraft[8]=0;memcpy(ownerDraft,ownerName,strlen(ownerName));profileCursor=0;currentScreen=SC_PROFILE;}
 else {settingsActiveSection=settingsSectionCursor;settingsCursor=section.fields[0];currentScreen=SC_SETTINGS;}
 currentMode=MODE_EDIT;redrawCurrentScreen();
}

int getMaxFields() {
  if (currentScreen == SC_ALARM) return 4;
  if (currentScreen == SC_SETTINGS) return 20;
  if(currentScreen==SC_HEALTH) return 8;
  if(currentScreen==SC_SPECIAL) return 1;
  if(currentScreen==SC_STYLE) return 3;
  if(currentScreen==SC_PROFILE) return 8;
  if (currentScreen == SC_TIME) return 5;
  if (currentScreen == SC_TIMER) return 4;
  return 0;
}

int& currentCursor() {
  if (currentScreen == SC_ALARM) return alarmCursor;
  if (currentScreen == SC_TIMER) return timerCursor;
  if(currentScreen==SC_TIME) return timeCursor;
  if(currentScreen==SC_PROFILE) return profileCursor;
  if(currentScreen==SC_SPECIAL) return specialCursor;
  if(currentScreen==SC_STYLE) return styleCursor;
  if(currentScreen==SC_HEALTH) return activityProfileCursor;
  return settingsCursor;
}

/* ============================================================
   БУДИЛЬНИК
   ============================================================ */
uint32_t alarmLastDate = 0;
int alarmLastMinute = -1;
int alarmLastPhase = -1;

void stopAlerts() {
  alarmRinging=false; timerRinging=false;
  noTone(BUZZER); wakeDisplay();
}

void alarmCheck() {
  syncAlarmSlot();
  const uint32_t dateKey=(uint32_t)(2000+yr)*10000+mon*100+day;
  if(timeValid) {
    for(int i=0;i<ALARM_COUNT;++i) {
      if(alarmEnabledBank[i] && hh==alarmHours[i] && mm==alarmMinutes[i] && alarmDates[i]!=dateKey) {
        alarmDates[i]=dateKey; // simultaneous alarms share one signal; all marked delivered
        if(!alarmRinging) {
          ringingAlarmSlot=i;alarmRinging=true;alarmStart=millis();alarmLastPhase=-1;
          if(previewActive) pendingPreview=true;
          previewActive=false;wakeDisplay();
        }
      }
    }
  }
  if(alarmRinging && ((ringingAlarmSlot>=0 && !alarmEnabledBank[ringingAlarmSlot]) || (uint32_t)(millis()-alarmStart)>=30000)) {
    alarmRinging=false;noTone(BUZZER);uiDirty=true;
  }
}

void timerCheck() {
  if (!timerRunning) return;
  uint32_t now=millis(), elapsed=now-timerLastUpdate;
  timerLastUpdate=now;
  if (elapsed < timerRemainingMs) { timerRemainingMs-=elapsed; return; }
  timerRemainingMs=0; timerRunning=false; timerRinging=true;
  timerCheckpointDirty=true;
  timerAlarmStart=now; alarmLastPhase=-1;
  if(previewActive) pendingPreview=true;
  previewActive=false;
  wakeDisplay();
}

void serviceAlertSound() {
  uint32_t now=millis();
  if (timerRinging && (uint32_t)(now-timerAlarmStart)>=30000) {
    timerRinging=false; noTone(BUZZER); uiDirty=true;
  }
  if (!alarmRinging && !timerRinging) { alarmLastPhase=-1; return; }
  uint32_t start=alarmRinging ? alarmStart : timerAlarmStart;
  int phase=(uint32_t)(now-start)/250;
  if (phase!=alarmLastPhase) {
    alarmLastPhase=phase;
    if ((phase&1)==0) tone(BUZZER,alarmRinging ? 2200 : 1800,120);
  }
}

void timerAction(int dir) {
  timerCheck();
  restoredTimerPending=false; timerCheckpointDirty=true;
  if (dir<0) {
    timerRunning=false; timerRemainingMs=(uint32_t)timerPresetSeconds*1000;
    timerCheckpointDirty=true;
    return;
  }
  if (timerRinging) return;
  if (timerRunning) timerRunning=false;
  else {
    if (!timerRemainingMs) timerRemainingMs=(uint32_t)timerPresetSeconds*1000;
    timerRunning=true; timerLastUpdate=millis();
  }
}

/* ============================================================
   ИЗМЕНЕНИЕ ПОЛЯ
   ============================================================ */
void adjustField(int dir) {
  if(currentScreen==SC_HEALTH) {activityAdjustField(dir);redrawCurrentScreen();return;}
  if(currentScreen==SC_STYLE) {if(styleCursor==0) {themeIndex=(themeIndex+8+dir)%8;applyTheme();markSettingsDirty();}else if(styleCursor==1){faceVariant=(faceVariant+8+dir)%8;applyTheme();markSettingsDirty();}else selectSpecialization(dir);redrawCurrentScreen();return;}
  if(currentScreen==SC_SPECIAL) {specialAction(dir);redrawCurrentScreen();return;}
  if(currentScreen==SC_PROFILE) {
    const char* alphabet=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
    const char* found=strchr(alphabet,ownerDraft[profileCursor]);
    int index=found ? found-alphabet:0;
    ownerDraft[profileCursor]=alphabet[(index+(int)strlen(alphabet)+dir)%strlen(alphabet)];
    drawProfile();return;
  }
  if((currentScreen==SC_ALARM && alarmCursor==0) || (currentScreen==SC_TIMER && timerCursor==0)) {
    syncAlarmSlot(); timeTool=(timeTool+ALARM_COUNT+1+dir)%(ALARM_COUNT+1);
    if(timeTool<ALARM_COUNT) {
      alarmSlot=timeTool;alarmH=alarmHours[alarmSlot];alarmM=alarmMinutes[alarmSlot];alarmEnabled=alarmEnabledBank[alarmSlot];
      currentScreen=SC_ALARM;
    } else currentScreen=SC_TIMER;
    alarmCursor=0;timerCursor=0;redrawCurrentScreen();return;
  }
  if(currentScreen==SC_TIME) {
    if(timeCursor==0) draftH=(draftH+24+dir)%24;
    if(timeCursor==1) draftM=(draftM+60+dir)%60;
    if(timeCursor==2) draftD=(draftD+daysInMonth(draftMo,draftY)-1+dir)%daysInMonth(draftMo,draftY)+1;
    if(timeCursor==3) draftMo=(draftMo+11+dir)%12+1;
    if(timeCursor==4) draftY=(draftY+100+dir)%100;
    draftD=constrain(draftD,1,daysInMonth(draftMo,draftY));
    drawTimeSettings(); return;
  }
  if (currentScreen==SC_TIMER) {
    if (timerCursor==3) timerAction(dir);
    else {
      timerCheck();
      if (!timerRunning && !timerRinging) {
        restoredTimerPending=false;
        int mins=timerPresetSeconds/60, secs=timerPresetSeconds%60;
        if (timerCursor==1) mins=(mins+100+dir)%100;
        else secs=(secs+60+dir)%60;
        timerPresetSeconds=mins*60+secs;
        if (!timerPresetSeconds) timerPresetSeconds=1;
        timerRemainingMs=(uint32_t)timerPresetSeconds*1000;
        markSettingsDirty();
      } else beep(600,40);
    }
    timerCheckpointDirty=true;
    redrawCurrentScreen(); return;
  }
  markSettingsDirty();
  if (currentScreen == SC_ALARM) {
    if (alarmCursor == 1)      alarmH = (alarmH + 24 + dir) % 24;
    else if (alarmCursor == 2) alarmM = (alarmM + 60 + dir) % 60;
    else                       { alarmEnabled = !alarmEnabled;
                                 if(ringingAlarmSlot==alarmSlot && !alarmEnabled) { alarmRinging = false; noTone(BUZZER); } }
    syncAlarmSlot();
    drawAlarmScreen();
    beep(dir > 0 ? 1800 : 1200, 25);
  }
  else if (currentScreen == SC_SETTINGS) {
    if (settingsCursor == 0) {
      const uint8_t levels[]={32,64,96,128,160,192,224,255};
      int index=0;
      while (index<7 && brightness>levels[index]) ++index;
      brightness=levels[(index+8+dir)%8];
      if (screenAwake) analogWrite(LCD_BL, brightness);
    } else if (settingsCursor == 1) {
      use24h = !use24h;
    } else if (settingsCursor == 2) {
      colonBlink = !colonBlink;
    } else if (settingsCursor == 3) {
      screenTimeoutIndex=(screenTimeoutIndex+5+dir)%5;
    } else if(settingsCursor==4) quietMode=!quietMode;
    else if(settingsCursor==5) privatePreview=!privatePreview;
    else if(settingsCursor==6 && dir<0) {
      memset(messages,0,sizeof(messages)); messageCount=0; messageIndex=0;
      messagePage=0; unreadMessages=0; droppedMessages=0; previewActive=false;pendingPreview=false;
    }
    else if(settingsCursor==7) {themeIndex=(themeIndex+8+dir)%8;applyTheme();}
    else if(settingsCursor==8) {
      memset(ownerDraft,' ',8);ownerDraft[8]=0;memcpy(ownerDraft,ownerName,strlen(ownerName));
      profileCursor=0;currentScreen=SC_PROFILE;currentMode=MODE_EDIT;drawProfile();return;
    }
    else if(settingsCursor==9) {autoReconnect=!autoReconnect;bleCheckAt=millis()-5000;}
    else if(settingsCursor==10 && dir<0) forgetPhone();
    else if(settingsCursor==11 && dir<0) {pairWindowActive=true;pairWindowStart=millis();bleCheckAt=millis()-5000;}
    else if(settingsCursor==12) wallpaperEnabled=!wallpaperEnabled;
    else if(settingsCursor==13) clockStyle=(clockStyle+2+dir)%2;
    else if(settingsCursor==14 && dir<0) removeWallpaper();
    else if(settingsCursor==15) selectSpecialization(dir);
    else if(settingsCursor==16) {doNotDisturb=!doNotDisturb;if(doNotDisturb) {previewActive=false;pendingPreview=false;}}
    else if(settingsCursor==17 && dir<0) injectTestNotification();
    else if(settingsCursor==18) {ecoMode=!ecoMode;powerService();}
    else if(settingsCursor==19) {activityEnabled=!activityEnabled;activityProbeAttempted=false;activityFilterReset();}
    drawSettingsScreen();
    beep(dir > 0 ? 1800 : 1200, 25);
  }
}

/* ============================================================
   ОБРАБОТЧИКИ КНОПКИ
   ============================================================ */
void onSingleClick() {
  if (alarmRinging || timerRinging) { stopAlerts(); return; }
  if(currentScreen==SC_SETTINGS_SECTIONS){settingsSectionCursor=(settingsSectionCursor+1)%SETTINGS_SECTION_COUNT;redrawCurrentScreen();return;}
  if (currentScreen==SC_MESSAGES && currentMode==MODE_EDIT) {
    if (messageCount) {
      int pages=drawTextPage(messages[messageIndex].body,-100,0,0,6,COL_TEXT);
      messagePage=(messagePage+1)%pages;
    }
    redrawCurrentScreen(); return;
  }
  if (currentMode == MODE_NAV) {
    nextScreen();
  } else {
    adjustField(+1);
  }
}

void onDoubleClick() {
  if (alarmRinging || timerRinging) { stopAlerts(); return; }
  if(currentScreen==SC_SETTINGS_SECTIONS){settingsSectionCursor=(settingsSectionCursor+SETTINGS_SECTION_COUNT-1)%SETTINGS_SECTION_COUNT;redrawCurrentScreen();return;}
  if (currentScreen==SC_MESSAGES && currentMode==MODE_EDIT) {
    if (messageCount) messageIndex=(messageIndex+1)%messageCount;
    messagePage=0; redrawCurrentScreen(); return;
  }
  if (currentMode == MODE_EDIT) {
    adjustField(-1);
  } else previousScreen();
}

void onLongPress() {
  if (alarmRinging || timerRinging) { stopAlerts(); return; }
  if(currentScreen==SC_SETTINGS_SECTIONS){openSettingsSection();return;}
  if(currentScreen==SC_SETTINGS && currentMode==MODE_NAV){settingsActiveSection=-1;settingsReturnToSections=false;currentScreen=SC_SETTINGS_SECTIONS;currentMode=MODE_EDIT;redrawCurrentScreen();return;}
  if(currentScreen==SC_SETTINGS && currentMode==MODE_EDIT && groupedSettings()){
    ++settingsSectionStep;
    if(settingsSectionStep>=SETTINGS_SECTIONS[settingsActiveSection].count){saveSettings();returnToSettingsSections();}
    else {settingsCursor=SETTINGS_SECTIONS[settingsActiveSection].fields[settingsSectionStep];redrawCurrentScreen();}
    return;
  }

  if(currentScreen==SC_ACTIVITY && !alarmRinging && !timerRinging) {settingsReturnToSections=false;currentScreen=SC_HEALTH;currentMode=MODE_EDIT;activityProfileCursor=0;redrawCurrentScreen();return;}
  if(currentScreen==SC_MENU && !alarmRinging && !timerRinging) {currentScreen=SC_STYLE;currentMode=MODE_EDIT;styleCursor=0;redrawCurrentScreen();return;}
  if(currentScreen==SC_CLOCK && !alarmRinging && !timerRinging) {currentScreen=SC_SPECIAL;currentMode=MODE_EDIT;specialCursor=0;redrawCurrentScreen();return;}
  if (alarmRinging || timerRinging) { stopAlerts(); return; }
  if (currentScreen==SC_MESSAGES) {
    currentMode=currentMode==MODE_NAV ? MODE_EDIT : MODE_NAV;
    messagePage=0; redrawCurrentScreen(); return;
  }
  if (currentMode == MODE_NAV) {
    if (currentScreen == SC_ALARM || currentScreen == SC_SETTINGS || currentScreen == SC_TIMER || currentScreen==SC_TIME || currentScreen==SC_SPECIAL) {
      if(currentScreen==SC_SPECIAL) specialCursor=0;
      if(currentScreen==SC_TIME) {
        draftH=hh; draftM=mm; draftD=day; draftMo=mon; draftY=yr; timeCursor=0;
      }
      currentMode = MODE_EDIT;
      alarmCursor = 0;
      settingsCursor = 0;
      timerCursor = 0;
      redrawCurrentScreen();
      beep(1500, 60);
    } else {
      beep(1000, 60);
      redrawCurrentScreen();
    }
  } else {
    int maxF = getMaxFields();
    int& c = currentCursor();
    c++;
    if (c >= maxF) {
      if(currentScreen==SC_TIME) commitDraftTime();
      if(currentScreen==SC_SPECIAL) currentScreen=SC_CLOCK;
      if(currentScreen==SC_STYLE) currentScreen=SC_MENU;
      if(currentScreen==SC_HEALTH) currentScreen=SC_ACTIVITY;
      if(currentScreen==SC_PROFILE) {
        memcpy(ownerName,ownerDraft,9);
        for(int i=7;i>=0 && ownerName[i]==' ';--i) ownerName[i]=0;
        if(!ownerName[0]) snprintf(ownerName,sizeof(ownerName),"NEXUS");
        markSettingsDirty();currentScreen=SC_SETTINGS;
      }
      if(settingsReturnToSections){saveSettings();returnToSettingsSections();beep(800,60);return;}
      currentMode = MODE_NAV;
      saveSettings();
      beep(800, 60);
    } else {
      beep(1500, 40);
    }
    redrawCurrentScreen();
  }
}

/* ============================================================
   ДЕТЕКТОР КНОПКИ
   ============================================================ */
#define BTN_DEBOUNCE_MS   50
#define LONG_PRESS_MS     700
#define DOUBLE_GAP_MS     300

bool btnStable    = true;
bool btnLastRead  = true;
uint32_t btnLastChange = 0;
uint32_t btnPressStart = 0;
bool btnPressed   = false;
bool btnLongFired = false;
uint32_t btnLastRelease = 0;
bool waitingDouble = false;
bool consumeButtonPress = false;
void cancelPendingButtonAction() {
 waitingDouble=false;
 if(btnPressed)consumeButtonPress=true;
}

void handleButton() {
  uint32_t cur = millis();
  bool raw = digitalRead(BTN_PIN);

  if (raw != btnLastRead) {
    btnLastRead = raw;
    btnLastChange = cur;
  }

  if ((cur - btnLastChange) < BTN_DEBOUNCE_MS) return;
  if (raw == btnStable) return;

  btnStable = raw;

  if (btnStable == LOW) {
    consumeButtonPress=!screenAwake || alarmRinging || timerRinging || previewActive || moveReminderActive;
    if (consumeButtonPress) {
      waitingDouble=false;
      if (alarmRinging || timerRinging) stopAlerts();
      if(moveReminderActive) movementReminderAcknowledge();
      if(previewActive) {previewActive=false;pendingPreview=false;}
      wakeDisplay();
    } else { lastActivity=millis(); }
    if (waitingDouble && (uint32_t)(btnLastChange - btnLastRelease) > DOUBLE_GAP_MS) {
      waitingDouble = false; onSingleClick();
    }
    btnPressStart = btnLastChange; // use the raw edge, before debounce delay
    btnPressed = true;
    btnLongFired = false;
  } else {
    if (btnPressed && !btnLongFired && !consumeButtonPress) {
      // Use the raw release edge, excluding release debounce time.
      uint32_t dur = (uint32_t)(btnLastChange - btnPressStart);
      if (dur >= LONG_PRESS_MS) {
        // A display transfer may have delayed the loop past the hold threshold.
        // Deliver the hold once even when release is processed first.
        btnLongFired = true;
        waitingDouble = false;
        onLongPress();
      } else {
        // Navigation also uses double click. Defer a single click until the
        // 300 ms window closes, so a double cannot advance two pages first.
        if (waitingDouble && (btnPressStart - btnLastRelease) <= DOUBLE_GAP_MS) {
          waitingDouble = false;onDoubleClick();
        } else {waitingDouble = true;btnLastRelease = cur;}

      }
    }
    btnPressed = false;
    consumeButtonPress = false;
  }
}

void checkLongPress() {
  if(btnPressed && !btnLongFired && !consumeButtonPress && digitalRead(BTN_PIN)==LOW &&
      (uint32_t)(millis()-btnPressStart)>=LONG_PRESS_MS) {
    btnLongFired=true; waitingDouble=false; onLongPress();
  }
}

void checkSingleTimeout() {
  if (waitingDouble && !btnPressed && digitalRead(BTN_PIN)==HIGH &&
      (millis() - btnLastRelease >= DOUBLE_GAP_MS)) {
    waitingDouble = false;
    onSingleClick();
  }
}

void loadPersonalization() {
  if(prefsOK) {
    uint8_t data[12]={};
    if(prefs.getBytesLength("personal")==sizeof(data) && prefs.getBytes("personal",data,sizeof(data))==sizeof(data) &&
        (data[0]==1 || data[0]==2) && data[1]<(data[0]==1 ? 4:8) && data[2]<=1 && data[11]==0) {
      bool valid=true;
      for(int i=3;i<11 && data[i];++i) if(!strchr(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_",data[i])) valid=false;
      if(valid && data[3]) {themeIndex=data[0]==1 ? (data[1]==0 ? 4:(data[1]==1 ? 3:(data[1]==2 ? 0:7))):data[1];autoReconnect=data[2];memcpy(ownerName,data+3,9);}
    }
    if(prefs.getBytesLength("peer")==sizeof(savedPeer)) prefs.getBytes("peer",savedPeer,sizeof(savedPeer));
    savedPeer[17]=0;
  }
  applyTheme();snprintf(bleName,sizeof(bleName),"NEXUS-%s",ownerName);
}
void bleService() {
  if((uint32_t)(millis()-bleCheckAt)<5000) return;
  bleCheckAt=millis();
  auto* server=NimBLEDevice::getServer();
  if(!server) return;
  if(server->getConnectedCount()>0) {
    bleLastConnectedAt=millis();
    auto peer=server->getPeerInfo((uint8_t)0);
    // Do not force security during the Chronos subscription handshake.
    // Bond keys are retained; pairing may be initiated by the phone.
    securityRequested=true;
    bool bonded=peer.isBonded();
    if(peerBonded!=bonded) {peerBonded=bonded;uiDirty=true;}
    if(bonded) {
      auto address=peer.getIdAddress().toString();
      if(address.size()==17 && strncmp(savedPeer,address.c_str(),17)!=0) {
        char candidate[18]={};snprintf(candidate,sizeof(candidate),"%s",address.c_str());
        if(prefsOK && prefs.putBytes("peer",candidate,sizeof(candidate))==sizeof(candidate)) {
          memcpy(savedPeer,candidate,sizeof(candidate));uiDirty=true;
        }
      }
    }
  } else {
    auto* advertising=NimBLEDevice::getAdvertising();
    // Chronos owns all BLE callbacks. Only restart advertising when it stopped.
    if(pairWindowActive && (uint32_t)(millis()-pairWindowStart)>=120000) pairWindowActive=false;
    bool available=autoReconnect || pairWindowActive;
    if(available)bleAdvertisingPowerService();
    if(available && !advertising->isAdvertising()) advertising->start();
    if(!available && advertising->isAdvertising()) advertising->stop();
  }
}
void forgetPhone() {
  // Remove this watch's bond keys, then drop the active link to avoid immediate re-bonding.
  auto* server=NimBLEDevice::getServer();
  if(server && server->getConnectedCount()>0) {
    auto peer=server->getPeerInfo((uint8_t)0);server->disconnect(peer.getConnHandle());
  }
  bool removed=NimBLEDevice::deleteAllBonds();
  forgetFailed=!removed;
  if(removed) {
    memset(savedPeer,0,sizeof(savedPeer));
    if(prefsOK && prefs.putBytes("peer",savedPeer,sizeof(savedPeer))!=sizeof(savedPeer)) {forgetFailed=true;Serial.println("Peer clear save failed");}
    peerBonded=false;securityRequested=true;uiDirty=true;
  }
}
void drawProfile() { drawModernProfile(); }

uint32_t clockEpoch() {
  uint32_t days=0;
  for(int y=0;y<yr;++y) days+=daysInMonth(2,y)==29 ? 366 : 365;
  for(int m=1;m<mon;++m) days+=daysInMonth(m,yr);
  return ((days+day-1)*24+hh)*3600+mm*60+ss;
}
void checkpointTimer() {
  if(!prefsOK || !timerCheckpointDirty || (uint32_t)(millis()-timerCheckpointAt)<1500) return;
  timerCheck();
  uint32_t data[4]={1,timerRemainingMs,(timerRunning && timeValid) ? clockEpoch()+(timerRemainingMs+999)/1000 : 0,(uint32_t)timerRunning};
  if(prefs.putBytes("timer",data,sizeof(data))==sizeof(data)) timerCheckpointDirty=false;
  timerCheckpointAt=millis();
}
void restoreTimer() {
  if(!restoredTimerPending || !timeValid) return;
  restoredTimerPending=false;
  uint32_t now=clockEpoch();
  if(restoredDeadline>now && restoredDeadline-now<=5999) {
    timerRemainingMs=(restoredDeadline-now)*1000; timerRunning=true; timerLastUpdate=millis();
  } else {
    timerRemainingMs=0; timerRunning=false;
    // Only ring for a recently expired deadline, never for an old abandoned run.
    if(now>=restoredDeadline && now-restoredDeadline<=300) {
      timerRinging=true; timerAlarmStart=millis(); wakeDisplay();
    }
    timerCheckpointDirty=true;
  }
}
void commitDraftTime() {
  hh=draftH; mm=draftM; ss=0; day=draftD; mon=draftMo; yr=draftY;
  wday=(clockEpoch()/86400+6)%7; timeValid=true; lastTick=millis();
  rtcOK=rtcPresent && rtcWriteTime(hh,mm,ss,wday,day,mon,yr);
  if(timerRunning) timerCheckpointDirty=true;
}
void drawTimeSettings() { drawModernDate(); }

/* ============================================================
   SETUP
   ============================================================ */
#include "NexusStudio.h"

void backlightService(){
 uint32_t timeout=(uint32_t)SCREEN_TIMEOUTS[screenTimeoutIndex]*1000;
 bool dim=screenAwake&&ecoMode&&timeout>=3000&&!themeReceiving&&!btnPressed&&!waitingDouble&&!alarmRinging&&!timerRinging&&!previewActive&&!moveReminderActive&&(uint32_t)(millis()-lastActivity)>=timeout-3000;
 if(dim!=screenDimmed){screenDimmed=dim;if(screenAwake)analogWrite(LCD_BL,dim?std::max(8,(int)brightness/4):brightness);}
}

void setup() {
  // Must precede Serial, delays, NVS, LCD and BLE startup.
  // PWR provides initial battery power; SYS_EN holds it after PWR is released.
  pinMode(SYS_EN, OUTPUT);
  digitalWrite(SYS_EN, HIGH);
  activityHapticBegin();

  Serial.begin(115200);
  delay(200);

  loadSettings();
  if(prefsOK) loadAlarmBank();
  loadThemeSettings();
  loadPersonalization();
  loadSpecialization();
  loadRuntimeSettings();
  activityLoad();
  powerService();
  if(prefsOK) {
    uint32_t state[4]={};
    if(prefs.getBytesLength("timer")==sizeof(state) && prefs.getBytes("timer",state,sizeof(state))==sizeof(state) &&
        state[0]==1 && state[1]<=5999000 && state[3]<=1) {
      timerRemainingMs=state[1];
      if(state[2] && state[3]) {restoredDeadline=state[2];restoredTimerPending=true;}
    }
  }
  pinMode(LCD_BL, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  digitalWrite(BUZZER, LOW);
  nexusButtonLoopTaskHandle=xTaskGetCurrentTaskHandle();
  attachInterrupt(digitalPinToInterrupt(BTN_PIN),buttonIsr,FALLING);
  gpio_wakeup_enable((gpio_num_t)BTN_PIN,GPIO_INTR_LOW_LEVEL); // wakes auto light sleep
  esp_sleep_enable_gpio_wakeup();

  if (!displayBegin()) {
    Serial.println("gfx->begin() FAILED!");
    // Retry initialization rather than remaining permanently stuck.
    for(int attempt=0;attempt<3;++attempt) {
      delay(250); if(displayBegin()) goto display_ready;
    }
    Serial.println("LCD failed after retries; restarting in 10 seconds");
    delay(10000); ESP.restart();
    while(true) delay(1000);
  }
display_ready:
  gfx->setTextWrap(false);
  gfx->fillScreen(COL_BG);
  analogWrite(LCD_BL, 0);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  Wire.setTimeOut(50);
  activityBusBegin();
  rtcPresent = i2cProbe(RTC_ADDR);
  rtcOK = rtcPresent && rtcReadTime();
  timeValid = rtcOK;
  Serial.println(rtcOK ? "RTC OK" : (rtcPresent ? "RTC time invalid" : "RTC not found"));

  NexusTouch::begin();
  activitySamplerBegin();
  /* ---------- Chronos ---------- */
  messageQueue=xQueueCreate(4,sizeof(MessageRecord));
  if (!messageQueue) Serial.println("Message queue allocation failed");
  watch.setConnectionCallback(connectionCallback);
  watch.setNotificationCallback(notificationCallback);
  watch.setConfigurationCallback(configCallback);
  watch.setName(bleName);
  watch.begin();
  NimBLEDevice::setSecurityAuth(true,false,true); // bonding + secure connections, Just Works (no MITM)
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  pairWindowActive=true;pairWindowStart=millis();

  Serial.printf("Chronos: advertising as %s\n",bleName);

  analogReadResolution(12);
  analogSetPinAttenuation(BAT_ADC, ADC_11db);
  batPercent = readBattery();

  showScreen(SC_CLOCK);
  displayPresent();analogWrite(LCD_BL,brightness);
  lastTick = millis();
  lastActivity = millis();
  if (batPercent >= 0) watch.setBattery(batPercent);
}

/* ============================================================
   LOOP
   ============================================================ */
void loop() {
  themeSerialService();
  specialService();
  watch.loop();
  bleService();

  bool gotTime, gotConnection, connected, gotFormat, format24, find;
  struct tm newTime = {};
  portENTER_CRITICAL(&eventMux);
  gotTime = timeUpdatePending; timeUpdatePending = false;
  if (gotTime) newTime = pendingTime;
  gotConnection = connChangePending; connChangePending = false;
  connected = pendingConnState;
  gotFormat = formatPending; formatPending = false; format24 = pending24h;
  find = findPending; findPending = false;
  portEXIT_CRITICAL(&eventMux);

  bool redraw = false;
  if (gotTime) {
    const int newYear = newTime.tm_year + 1900;
    if (newYear >= 2000 && newYear <= 2099 &&
        validTime(newTime.tm_hour, newTime.tm_min, newTime.tm_sec,
                  newTime.tm_wday, newTime.tm_mday, newTime.tm_mon + 1, newYear - 2000)) {
      hh = newTime.tm_hour; mm = newTime.tm_min; ss = newTime.tm_sec;
      day = newTime.tm_mday; mon = newTime.tm_mon + 1;
      yr = newYear - 2000; wday = newTime.tm_wday;
      timeValid = true;
      if(timerRunning) timerCheckpointDirty=true;
      if (rtcPresent) rtcOK = rtcWriteTime(hh, mm, ss, wday, day, mon, yr);
      lastTick = millis();
      redraw = true;
      Serial.printf("Time applied: %02d:%02d:%02d %02d.%02d.20%02d\n", hh,mm,ss,day,mon,yr);
    } else Serial.println("Invalid Chronos date ignored");
  }
  if (gotConnection) { blePowerMode=-1;blePowerAt=millis();chronosConnected = connected; securityRequested=false; peerBonded=false; bleCheckAt=millis()-5000; redraw = true; }
  if (gotFormat && use24h!=format24) { use24h = format24; markSettingsDirty(); redraw = true; }
  if (find) { wakeDisplay(); beep(2000,200); }
  MessageRecord incoming;
  bool received=false;
  // Bounded drain: callbacks cannot keep the UI trapped in this loop.
  for (int n=0; messageQueue && n<4 && xQueueReceive(messageQueue,&incoming,0)==pdTRUE; ++n) {
    // Keep a reader anchored on the same record when a newer message arrives.
    if (currentScreen==SC_MESSAGES && currentMode==MODE_EDIT && messageCount) {
      if (messageIndex<MESSAGE_HISTORY-1) ++messageIndex;
      else messagePage=0; // oldest record will be evicted
    }
    if(messageCount==MESSAGE_HISTORY && messages[MESSAGE_HISTORY-1].unread && unreadMessages>0) --unreadMessages;
    for (int i=MESSAGE_HISTORY-1; i>0; --i) messages[i]=messages[i-1];
    incoming.unread=true;
    if(timeValid) snprintf(incoming.stamp,sizeof(incoming.stamp),"%02d:%02d",hh,mm);
    else snprintf(incoming.stamp,sizeof(incoming.stamp),"--:--");
    messages[0]=incoming; ++unreadMessages;
    if (messageCount<MESSAGE_HISTORY) ++messageCount;
    previewMessage=incoming;
    received=true;
  }
  if(received) {
    if(!doNotDisturb) pendingPreview=true;
    redraw=true;
  }
  if(previewActive && (uint32_t)(millis()-previewStart)>=8000) {previewActive=false;redraw=true;}

  if((uint32_t)(millis()-lastRtcPoll)>=60000 && currentScreen!=SC_TIME) {
    lastRtcPoll=millis();
    rtcPresent=i2cProbe(RTC_ADDR);
    rtcOK=rtcPresent && rtcReadTime();
    if(rtcOK) { timeValid=true; lastTick=millis(); redraw=true; }
  }
  restoreTimer();
  uint32_t cur = millis();
  bool ticked = false;
  // Subtraction survives millis() wrap; catch up after delayed drawing/I2C.
  while ((uint32_t)(cur - lastTick) >= 1000) {
    lastTick += 1000;
    advanceSecond();
    alarmCheck();
    ticked = true;
  }
  activityService();
  alarmCheck();
  timerCheck();
  serviceAlertSound();
  if(doNotDisturb) {pendingPreview=false;previewActive=false;}
  else if(pendingPreview && !alarmRinging && !timerRinging) {
    pendingPreview=false;previewActive=true;wakeDisplay();previewStart=millis();
    beep(notificationToneHz,notificationToneMs);redraw=true;
  }
  movementReminderService();
  // Collect battery and UI changes before composing one frame.
  static uint32_t lastEnv = 0;
  if ((uint32_t)(cur - lastEnv) >= (ecoMode ? (screenAwake ? 30000u:180000u):5000u)) {
    lastEnv = cur;
    int previousBattery=batPercent;batPercent = readBattery();
    if (batPercent >= 0 && batPercent!=previousBattery) watch.setBattery(batPercent);
    if (screenAwake && currentScreen == SC_MENU) redraw=true;
    if (batPercent!=previousBattery) redraw=true;
  }
  if (screenAwake) {
    if (redraw || uiDirty) { uiDirty=false; redrawCurrentScreen(); }
    else if (ticked && !alarmRinging && !timerRinging && !previewActive && !moveReminderActive) {
      if (currentScreen==SC_CLOCK && (osdNeedsTick() || ss==0)) updateClockScreen();
      if (currentScreen==SC_TIMER && timerRunning) drawTimerScreen();
      if(currentScreen==SC_SPECIAL) drawSpecialization();
      if(currentScreen==SC_ACTIVITY) drawActivityScreen();
    }
  }

  checkpointTimer();
  // Optional CST816T touch. Same application handlers as the physical button:
  // NAV: swipe changes pages, tap opens an item, hold follows the next-field action.
  // EDIT: swipe adjusts values; tap increments; hold moves to next field.
  // Keep all alerts and physical button gestures intact.
  NexusTouch::serviceReconnect(screenAwake);
  bool touchAction = NexusTouch::service(screenAwake, !themeReceiving,
      currentMode == MODE_EDIT,
      currentMode == MODE_EDIT ? onSingleClick : nextScreen,
      currentMode == MODE_EDIT ? onDoubleClick : previousScreen,
      currentMode == MODE_EDIT ? onSingleClick : onLongPress,
      onLongPress);
  if (touchAction) { lastActivity = millis(); screenDimmed = false; }
  handleButton();
  checkLongPress();
  checkSingleTimeout();
  if (settingsDirty && (uint32_t)(millis()-settingsChangedAt)>=1500) saveSettings();
  backlightService();
  uint32_t timeout=(uint32_t)SCREEN_TIMEOUTS[screenTimeoutIndex]*1000;
  if (screenAwake && timeout && !themeReceiving && !btnPressed && !waitingDouble && !alarmRinging &&
      !timerRinging && !previewActive && !moveReminderActive && (uint32_t)(millis()-lastActivity)>=timeout) {
    // LCD sleep leaves the editor/reader and its cursor intact.
    saveSettings();
    suspendDisplay(); // LCD sleep; BLE and application logic remain active.
  }
  if(screenAwake)displayPresent();
  powerService();
  blePowerService();
  if(!screenAwake && ecoMode && !themeReceiving && !btnPressed && !waitingDouble &&
     !alarmRinging && !timerRinging && !powerSerialBusy()) {
    // Block until the next second tick, a button edge, or 250ms (BLE/queue events).
    // Level-low IRQ doubles as light-sleep wake source; ISR disables it, re-arm only when released.
    if(digitalRead(BTN_PIN)==HIGH){gpio_set_intr_type((gpio_num_t)BTN_PIN,GPIO_INTR_LOW_LEVEL);gpio_wakeup_enable((gpio_num_t)BTN_PIN,GPIO_INTR_LOW_LEVEL);gpio_intr_enable((gpio_num_t)BTN_PIN);}
    uint32_t toTick=1000-((uint32_t)(millis()-lastTick)%1000);
    ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(std::min<uint32_t>(toTick,250)));
  } else delay(ecoMode ? (screenAwake || themeReceiving || btnPressed || waitingDouble ? 5:20):1);
}
