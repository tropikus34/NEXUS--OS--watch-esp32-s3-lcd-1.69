#include <cassert>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
void openSection(int i){showScreen(SC_SETTINGS);onLongPress();assert(currentScreen==SC_SETTINGS_SECTIONS);settingsSectionCursor=i;onLongPress();}
int main(){setup();quietMode=true;timeValid=true;hh=12;mm=48;day=27;mon=10;yr=26;
 // Seven pages are a reversible loop, with one joint slot for alarm/timer.
 for(int tool:{0,5}){timeTool=tool;showScreen(SC_CLOCK);for(int i=0;i<7;++i){assert(navigationIndex(currentScreen)==i);Screen previous=currentScreen;onSingleClick();onDoubleClick();assert(currentScreen==previous);onSingleClick();}assert(currentScreen==SC_CLOCK);onDoubleClick();assert(currentScreen==SC_TIME);onSingleClick();assert(currentScreen==SC_CLOCK);}
 // Exercise the actual debouncer: a physical double click must go back,
 // without an intermediate forward navigation or an extra delayed click.
 auto resetButton=[](){fakeButton=1;btnStable=true;btnLastRead=true;btnPressed=false;btnLongFired=false;waitingDouble=false;consumeButtonPress=false;};
 auto tap=[](){fakeButton=0;handleButton();fakeMs+=51;handleButton();fakeMs+=80;fakeButton=1;handleButton();fakeMs+=51;handleButton();};
 showScreen(SC_CLOCK);resetButton();tap();assert(waitingDouble&&currentScreen==SC_CLOCK);tap();assert(!waitingDouble&&currentScreen==SC_TIME);fakeMs+=400;checkSingleTimeout();assert(currentScreen==SC_TIME);
 showScreen(SC_CLOCK);resetButton();tap();fakeMs+=301;checkSingleTimeout();assert(currentScreen==SC_ACTIVITY&&!waitingDouble);
 resetButton();screenAwake=false;tap();fakeMs+=400;checkSingleTimeout();assert(screenAwake&&currentScreen==SC_ACTIVITY&&!waitingDouble);
 // Every legacy setting is reachable through a short category, or the activity profile.
 bool covered[20]={};for(int g=0;g<SETTINGS_SECTION_COUNT-1;++g){if(g==4){covered[19]=true;continue;}for(int i=0;i<SETTINGS_SECTIONS[g].count;++i){int f=SETTINGS_SECTIONS[g].fields[i];assert(f>=0&&f<20);assert(!covered[f]);covered[f]=true;}}
 for(bool v:covered)assert(v);
 // Hold opens a category; it never modifies brightness or executes an action.
 uint8_t before=brightness;openSection(0);assert(currentScreen==SC_SETTINGS&&settingsCursor==0&&groupedSettings());assert(brightness==before);
 onSingleClick();assert(brightness!=before);assert(settingsVisibleCount()==4&&settingsVisibleIndex()==0);
 for(int f:{3,1,2}){onLongPress();assert(settingsCursor==f&&currentMode==MODE_EDIT);}
 onLongPress();assert(currentScreen==SC_SETTINGS_SECTIONS&&settingsSectionCursor==0&&currentMode==MODE_EDIT&&!settingsReturnToSections);
 // Category selection wraps backwards, and Back returns to normal page navigation.
 onDoubleClick();assert(settingsSectionCursor==8);onLongPress();assert(currentScreen==SC_SETTINGS&&currentMode==MODE_NAV);onDoubleClick();assert(currentScreen==SC_MESSAGES);
 for(int g:{1,2,3,5}){openSection(g);assert(settingsActiveSection==g&&settingsCursor==SETTINGS_SECTIONS[g].fields[0]);for(int i=0;i<SETTINGS_SECTIONS[g].count;++i)onLongPress();assert(currentScreen==SC_SETTINGS_SECTIONS&&settingsSectionCursor==g&&!groupedSettings());}
 // Local profile, owner name and date editor return to the category that opened them.
 openSection(4);assert(currentScreen==SC_HEALTH&&settingsReturnToSections);for(int i=0;i<8;++i)onLongPress();assert(currentScreen==SC_SETTINGS_SECTIONS&&settingsSectionCursor==4);
 openSection(6);assert(currentScreen==SC_PROFILE);snprintf(ownerDraft,sizeof(ownerDraft),"ED      ");for(int i=0;i<8;++i)onLongPress();assert(!strcmp(ownerName,"ED")&&currentScreen==SC_SETTINGS_SECTIONS&&settingsSectionCursor==6);
 openSection(7);assert(currentScreen==SC_TIME&&settingsReturnToSections);draftH=13;for(int i=0;i<5;++i)onLongPress();assert(hh==13&&currentScreen==SC_SETTINGS_SECTIONS&&settingsSectionCursor==7);
 showScreen(SC_ACTIVITY);onLongPress();assert(!settingsReturnToSections);for(int i=0;i<8;++i)onLongPress();assert(currentScreen==SC_ACTIVITY&&currentMode==MODE_NAV);
 // Phone previews preserve category/field position; alerts consume the press.
 openSection(2);onLongPress();assert(settingsCursor==11);assert(injectTestNotification());loop();assert(previewActive&&settingsActiveSection==2&&settingsSectionStep==1&&settingsCursor==11);
 previewActive=false;pendingPreview=false;alarmRinging=true;int step=settingsSectionStep;onLongPress();assert(!alarmRinging&&settingsSectionStep==step&&settingsCursor==11);
 // Theme changes never change navigation, selected group, or specialization.
 specialization=3;for(int t=0;t<7;++t){themeIndex=t;applyTheme();openSection(0);onLongPress();assert(settingsCursor==3&&settingsVisibleIndex()==1&&specialization==3);onDoubleClick();assert(settingsCursor==3);}
 // Word wrapping keeps short words whole and never splits a UTF-8 sequence.
 const char* sample="Один два три";const char* next;const char* end=messageLineEnd(sample,next,uiWidth("Один два"));assert(std::string(sample,end)=="Один два"&&!strcmp(next,"три"));
 const char* longWord="АБВГДЕЖЗИЙ";end=messageLineEnd(longWord,next,uiWidth("АБВ"));assert(std::string(longWord,end)=="АБВ"&&((uint8_t)*next&0xc0)!=0x80);
 const char* newline="Привет\nМир";end=messageLineEnd(newline,next);assert(std::string(newline,end)=="Привет"&&!strcmp(next,"Мир"));
 assert(drawTextPage("",-100,0,0,6,COL_TEXT)==1&&drawTextPage("one\ntwo\nthree",-100,0,0,2,COL_TEXT)==2);
 assert(uiWidth("Сообщения")>0&&uiRadius()>0);
 puts("PASS: reversible page loop, complete category coverage, short edits/save/return, category wrap/back, name/profile/date returns, notification and alert preservation, seven-theme invariance");
}
