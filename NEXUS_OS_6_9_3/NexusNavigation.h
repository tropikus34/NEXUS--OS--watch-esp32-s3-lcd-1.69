#pragma once
// IDs refer to existing saved settings; grouping does not change the NVS format.
constexpr const char* SETTINGS_LABELS[]={"Яркость","Формат времени","Мигание точек","Гашение экрана","Без звука","Скрыть текст","Очистить сообщения","Тема","Имя владельца","Автоподключение","Забыть телефон","Подключить телефон","Свой фон","Своя компоновка","Удалить фон","Специализация","Не беспокоить","Тест уведомления","Экономия энергии","Подсчёт шагов"};
struct SettingsSection {const char* name;const char* hint;uint8_t count;int8_t fields[5];uint8_t icon;};
constexpr SettingsSection SETTINGS_SECTIONS[]={
 {"Экран","Яркость и гашение",4,{0,3,1,2,-1},0},
 {"Стиль","Тема и инструмент",5,{7,15,12,13,14},1},
 {"Телефон","Связь с телефоном",3,{9,11,10,-1,-1},2},
 {"Сообщения","Звук и приватность",5,{16,4,5,17,6},3},
 {"Активность","Шаги и разминка",0,{-1,-1,-1,-1,-1},4},
 {"Энергия","Режим экономии",1,{18,-1,-1,-1,-1},5},
 {"Ваше имя","Имя Bluetooth",1,{8,-1,-1,-1,-1},6},
 {"Дата и время","Время и календарь",0,{-1,-1,-1,-1,-1},7},
 {"Назад","Закрыть настройки",0,{-1,-1,-1,-1,-1},8}
};
constexpr int SETTINGS_SECTION_COUNT=sizeof(SETTINGS_SECTIONS)/sizeof(SETTINGS_SECTIONS[0]);
int settingsSectionCursor=0,settingsSectionStep=0,settingsActiveSection=-1;
bool settingsReturnToSections=false;
bool groupedSettings(){return settingsActiveSection>=0 && settingsActiveSection<SETTINGS_SECTION_COUNT;}
int settingsVisibleIndex(){return groupedSettings() ? settingsSectionStep:settingsCursor;}
int settingsVisibleCount(){return groupedSettings() ? SETTINGS_SECTIONS[settingsActiveSection].count:20;}
int settingsNextField(){return groupedSettings() ? SETTINGS_SECTIONS[settingsActiveSection].fields[std::min<int>(settingsSectionStep+1,SETTINGS_SECTIONS[settingsActiveSection].count-1)]:std::min(settingsCursor+1,19);}
int navigationIndex(Screen screen){
 switch(screen){
 case SC_CLOCK:case SC_SPECIAL:return 0;
 case SC_ACTIVITY:case SC_HEALTH:return 1;
 case SC_ALARM:case SC_TIMER:return 2;
 case SC_MESSAGES:return 3;
 case SC_SETTINGS:case SC_PROFILE:case SC_SETTINGS_SECTIONS:return 4;
 case SC_MENU:case SC_STYLE:return 5;
 default:return 6;
 }
}
Screen navigationPage(int index){
 const Screen pages[]={SC_CLOCK,SC_ACTIVITY,timeTool==ALARM_COUNT ? SC_TIMER:SC_ALARM,SC_MESSAGES,SC_SETTINGS,SC_MENU,SC_TIME};
 return pages[index];
}
