// Project Name : ESPHome - MyVito
// Author       : Romuald Dufour
// License      : LGPL v2.1
// Release      : 2022-02-01

#include <Arduino.h>
#include "esphome.h"

// Uncomment next line to activate verbose debug mode
//#define DEBUG

// Uncomment to read all adresses - Take longer - No data on every systems
//#define FULL_READ

// --------------------------------------------------------------------------------------------------------------------
// Global parameters

#define MODE_STANDBY 5
#define MODE_HEATING 3
#define MODE_HOTWATER 0

const char* logname PROGMEM = "Vitotronic";

const int maxparam  = 99;     // Max number of parameters in param read arrays
const int maxwrite  = 10;     // Max number of parameters in the write arrays
const int maxreclen = 99;     // Receive buffer size
const int statuslen = 22;     // Len of the status stream (2500, 3500 address)
const int timelen   = 8;      // Len of the time & timers stream (088e; 2000..2030; 2100..2130; 2200..2230)
const int timercnt  = 7;

char DayOfWeek[] = "Xx\0Lu\0Ma\0Me\0Je\0Ve\0Sa\0Di\0";   // Fr
//char DayOfWeek[] = "Xx\0Mo\0Tu\0We\0Th\0Fr\0Sa\0Su\0";  // En

uint16_t paramaddr[maxparam] = {0x00f8, 0x088e,                                                                  // ID & System Date/Time
                                0x0800, 0x5525, 0x5527, 0x0802, 0x0810, 0x0896,                                  // Chauffage
                                0x551E, 0x0883, 0x0842, 0x7574, 0x088A, 0x08A7, 0x2906, 0x0847,                  // Bruleur
                                0x2301, 0x2302, 0x2303, 0x2304, 0x2305, 0x2306, 0x2307, 0x2308,                  // Modes+Consignes
                                0x6300, 0x6760, 0x0804, 0x0812, 0x0845, 0x0846,                                  // ECS
                                0x7507, 0x7510, 0x7519, 0x7522, 0x752b, 0x7534, 0x753d, 0x7546, 0x754f, 0x7558,  // Error codes
                                0x2000, 0x2008, 0x2010, 0x2018, 0x2020, 0x2028, 0x2030,                          // Timers Chauffage
                                0x2100, 0x2108, 0x2110, 0x2118, 0x2120, 0x2128, 0x2130,                          // Timers Charge ECS
                                0x2200, 0x2208, 0x2210, 0x2218, 0x2220, 0x2228, 0x2230,                          // Timers Boucle ECS
                                0x2309, 0x2311,                                                                  // Timers vacances
                                0x2500,                                                                          // Status
#ifdef FULL_READ
                                0x55E3, 0x0849, 0x08AB, 0x254C, 0x5555, 0x2309, 0x0808, 0x2335, 0x7700,
                                0xA309, 0x0814, 0x0816, 0x0818, 0x081a, 0x089f, 0x2544, 0x555a, 0x080A, 0x080C,
                                0xA38F, 0xA305, 0x08A1, 0x2501, 0x2502, 0x2508, 0x250A, 0x250B, 0x250C, 0x2510,
#endif
                                0};

uint8_t paramlen[maxparam]   = {2,      8,
                                2,      2,      2,      2,      2,      2,
                                1,      1,      1,      4,      4,      4,      1,      1,
                                1,      1,      1,      1,      1,      1,      1,      1,
                                1,      1,      2,      2,      1,      1,
                                1,      1,      1,      1,      1,      1,      1,      1,      1,      1,
                                8,      8,      8,      8,      8,      8,      8,
                                8,      8,      8,      8,      8,      8,      8,
                                8,      8,      8,      8,      8,      8,      8,
                                8,      8,
                                statuslen,
#ifdef FULL_READ
                                1,      1,      4,      1,      1,      4,      2,      1,      1,
                                2,      2,      2,      2,      2,      2,      2,      2,      2,      2,
                                2,      2,      2,      1,      4,      1,      1,      1,      2,      1,
#endif
                                0};

char paramtxt[maxparam][20]  = {"Sys ID",       "Date/Time",
                                "T Ext Sensor", "T Ext Fltr",   "T Ext Mean",   "T Boiler",      "T Boiler Fltr", "T Amb Fltr",
                                "Burner",       "Burner malf.", "Rate 1",       "Consumpt.",     "Starts",        "Time Rate 1",   "Heating pump", "Fault",
                                "Mode",         "Eco",          "Party",        "Offset",        "Slope",         "Normal set",    "Reduced set",  "Party set",
                                "HW set",       "HW offset",    "T HW sensor",  "T HW Fltr",     "HW pump",       "HW Cycl. Pump",
                                "Error 1",      "Error 2",      "Error 3",      "Error 4",       "Error 5",       "Error 6",       "Error 7",      "Error 8",       "Error 9", "Error 10",
                                "Tmr Heat. Mo", "Tmr Heat. Tu", "Tmr Heat. We", "Tmr Heat. Th",  "Tmr Heat. Fr",  "Tmr Heat. Sa",  "Tmr Heat. Su",
                                "Tmr HW. Mo",   "Tmr HW. Tu",   "Tmr HW. We",   "Tmr HW. Th",    "Tmr HW. Fr",    "Tmr HW. Sa",    "Tmr HW. Su",
                                "Tmr Cyc. Mo",  "Tmr Cyc. Tu",  "Tmr Cyc. We",  "Tmr Cyc. Th",   "Tmr Cyc. Fr",   "Tmr Cyc. Sa",   "Tmr Cyc. Su",
                                "Vac. Begin",   "Vac. End",
                                "Status",
                                "",     "",     "",     "",     "",     "",     "T exhaust",
                                "",     "",     "",     "",     "",     "",     "",     "",     "",     "",
                                "",     "",     "",     "",     "",     "",     "",     "",     "",     "",
                                ""};

// --------------------------------------------------------------------------------------------------------------------
// Globals vars
uint16_t writeaddr[maxwrite] = {0x2301, 0x2302, 0x2303, 0x2306, 0x2307, 0x2308, 0x6300, 0x0000};
uint32_t writedata[maxwrite] = {0,      0,      0,      0,      0,      0,      0,      0};
uint8_t  writelen[maxwrite]  = {0,      0,      0,      0,      0,      0,      0,      0};

uint32_t paramvalue[maxparam];
uint8_t paramreaded[maxparam];

uint8_t param2500status[statuslen];
uint8_t systemtime[timelen];
uint8_t vactimer[2][timelen];
uint8_t heattimer[timercnt+1][timelen];  // 1..7; 0 not used to stick with dayidx
uint8_t watertimer[timercnt+1][timelen];
uint8_t watercyclingtimer[timercnt+1][timelen];

struct climate {
  float   target = 0;
  float   target_last = 0;
  float   current = 0;
  float   current_last = 0;
  int     mode = -1;
  int     mode_last = -1;
  uint8_t mode2301 = -1;
};

struct climate heaterclimate;
struct climate waterclimate;

int txtupdate = 0;
int binupdate = 0;

uint8_t current_mode = 5;
uint8_t b2509 = 2;
uint8_t b250a = 0;
bool eco_mode = false;
bool reception_mode = false;
bool water_priority = false;


// --------------------------------------------------------------------------------------------------------------------
// Global functions
void initvalue(){
  for (int idx = 0; idx < maxparam; idx++) {
    paramvalue[idx] = 0;
    paramreaded[idx] = 0;
  }
  for (int idx = 0; idx < statuslen; idx++) param2500status[idx] = 0;
}

uint32_t getvalue(uint16_t addr){
  uint8_t idx = 0;
  uint32_t val = 0;
  while ((paramaddr[idx] != 0) && (idx < 250)) {
    if (paramaddr[idx] == addr) val = paramvalue[idx];
    idx++;
  }
  return val;
}

void setvalue(uint16_t addr, uint32_t val){
  // Sometime communication error give big difference on 32 bits value, poluting graph
  // Limiting range and forcing increase only on monotone values
  if ((addr == 0x7574) || (addr == 0x088a) || (addr == 0x08a7)){
    uint32_t old_val = getvalue(addr);
    if (val < old_val) val = old_val;
    if ((old_val > 100) && (val > old_val*1.1)) val = old_val * 1.1;
  }
  // Save new data
  uint8_t idx = 0;
  while ((paramaddr[idx] != 0) && (idx < 250)) {
    if (paramaddr[idx] == addr) paramvalue[idx] = val;
    idx++;
  }
  idx = 0;
  while ((writeaddr[idx] != 0) && (idx < maxwrite)) {
    if ((writeaddr[idx] == addr) && (writelen[idx] == 0)) writedata[idx] = val; //Keep updated if no write request
    idx++;
  }
}

void dowrite(uint16_t addr, uint32_t val, uint8_t len = 1){
  uint8_t idx = 0;
  while ((paramaddr[idx] != 0) && (idx < maxwrite)) {
    if (writeaddr[idx] == addr) { writedata[idx] = val; writelen[idx] = len; }
    idx++;
  }
}

int haswrite(){
  uint8_t idx = 0;
  while ((writeaddr[idx] != 0) && (idx < maxwrite)) {
    if (writelen[idx] != 0) return idx;
    idx++;
  }
  return -1;
}

int getsigned(int ivalue) {
  if (ivalue < 32768) return ivalue;
  return ivalue - 65536;
}

void getstatus() { 
  current_mode = getvalue(0x2301);
  eco_mode = (getvalue(0x2302) != 0);
  reception_mode = (getvalue(0x2303) != 0);
  b2509 = param2500status[0x09];
  b250a = param2500status[0x0a];
  water_priority = (b250a == 0) && ((b2509 == 3) || (b2509 == 4));
}

uint8_t getsystemtimedayidx(uint8_t timebuf[timelen]) {
  uint8_t dayidx = timebuf[4];
  if ((dayidx >= 1) && (dayidx <= 7) && ((timebuf[0] == 0x20) || (timebuf[0] == 0x19))) return dayidx; // 1 (Monday) .. 7 (Sunday)
  else return 0; // invalid
}

bool systemtimetotext(uint8_t timebuf[timelen], char *St) {
  uint8_t dayidx = getsystemtimedayidx(timebuf);
  if (dayidx) {
    sprintf(St, "%2s %02x-%02x-%x%02x %02x:%02x:%02x", &DayOfWeek[3*dayidx],
                timebuf[3], timebuf[2], timebuf[0], timebuf[1],
                timebuf[5], timebuf[6], timebuf[7]);
    return true;
  } else {
    strcpy(St, "");  // Invalid date/time
    return false;
  }
}

void bytetotimer(char *St, uint8_t b) {
  uint8_t h = b / 8;
  uint8_t m = b % 8 * 10;
  if (b != 0xff) sprintf(St, "%02d:%02d ", h, m); else strcpy(St, "--:-- ");
}


bool timertotext(char *St, uint8_t Timer[], bool dash = false) {
  uint8_t dayidx = getsystemtimedayidx(systemtime);
  memset(St, 0, 6*8+1);
  if (dayidx) {
    for (int i = 0; i < 8; i++) {
      if (dash || (Timer[i] != 0xff)) bytetotimer(&St[i*6], Timer[i]);
      if ((i+1) % 2) St[i*6+5]='-';
    }
    return true;
  } else {
    strcpy(St, "");  // Invalid date/time
    return false;
  }
}



// --------------------------------------------------------------------------------------------------------------------
// Binary sensors class
class MyVitoBinarySensorsComponent : public Component, public BinarySensor {
private:
  uint32_t ivalue;

public:
  // Calculated sensors
  BinarySensor *WaterPriority_sensor = new BinarySensor();
  
  // Pump sensors
  BinarySensor *HWGPump_sensor = new BinarySensor();
  BinarySensor *HeatingPump_sensor = new BinarySensor();
  BinarySensor *HWCirculatingPump_sensor = new BinarySensor();

  // Burner sensors
  BinarySensor *BurnerON_sensor = new BinarySensor();
  BinarySensor *BurnerDefault_sensor = new BinarySensor();
  
  // Mode sensors
  BinarySensor *HeaterDefault_sensor = new BinarySensor();
  BinarySensor *HeaterEco_sensor = new BinarySensor();
  BinarySensor *HeaterReception_sensor = new BinarySensor();

  void loop() override {
    if (binupdate != 0) {
      getstatus();

      // Calculated status
      WaterPriority_sensor->publish_state(water_priority);
      
      // Pumps
      ivalue = getvalue(0x0845);  if (ivalue>=0 && ivalue<=1) HWGPump_sensor->publish_state((ivalue != 0)); 
      ivalue = getvalue(0x2906);  if (ivalue>=0 && ivalue<=1) HeatingPump_sensor->publish_state((ivalue != 0)); 
      ivalue = getvalue(0x0846);  if (ivalue>=0 && ivalue<=1) HWCirculatingPump_sensor->publish_state((ivalue != 0)); 

      // Burner
      ivalue = getvalue(0x551E);  if (ivalue>=0 && ivalue<=1) BurnerON_sensor->publish_state((ivalue != 0)); 
      ivalue = getvalue(0x0883);  if (ivalue>=0 && ivalue<=255) BurnerDefault_sensor->publish_state((ivalue != 0)); 
      
      // Modes
      ivalue = getvalue(0x0847);  if (ivalue>=0 && ivalue<=255) HeaterDefault_sensor->publish_state((ivalue != 0)); 
      ivalue = getvalue(0x2302);  if (ivalue>=0 && ivalue<=255) HeaterEco_sensor->publish_state((ivalue != 0)); 
      ivalue = getvalue(0x2303);  if (ivalue>=0 && ivalue<=255) HeaterReception_sensor->publish_state((ivalue != 0)); 

      binupdate = 0;
    }
  }
};


// --------------------------------------------------------------------------------------------------------------------
// Text sensors class
class MyVitoTextSensorsComponent : public Component, public TextSensor {
public:
  TextSensor *ModeTxt_sensor = new TextSensor();
  TextSensor *SetTxt_sensor = new TextSensor();
  TextSensor *SystemTimeTxt_sensor = new TextSensor();
  TextSensor *HeatTimerTxt_sensor = new TextSensor();
  TextSensor *WaterTimerTxt_sensor = new TextSensor();
  TextSensor *WaterCyclingTimerTxt_sensor = new TextSensor();

  
  void loop() override {
    char St[255];
    if (txtupdate != 0) {
      getstatus();
      char StMode[20]; strcpy(StMode, "Unknown");
      if (current_mode == MODE_STANDBY) strcpy(StMode, "Veille");
      if (current_mode == MODE_HOTWATER) strcpy(StMode, "ECS");
      if (current_mode == MODE_HEATING) {
        strcpy(StMode, "Chauffage");
        if (b2509 == 3) strcat(StMode, " Normal");
        if (b2509 == 4) strcat(StMode, " Réduit");
        if (eco_mode) strcat(StMode, " Eco");
        if (reception_mode) strcat(StMode, " Réception");
      }
      if (b2509 == 1) strcpy(StMode, "Vacances");
      ModeTxt_sensor->publish_state(StMode);
      char StSet[20]; strcpy(StSet, "Veille");
      if ((current_mode == MODE_HEATING) && (b2509 != 1)) {
        if (b250a == 1) strcpy(StSet, "Normale");
        if (b250a == 3) strcpy(StSet, "Réduite");
        if (reception_mode) strcpy(StSet, "Réception");
        if (water_priority) strcpy(StSet, "ECS");
      }
      SetTxt_sensor->publish_state(StSet);

      uint8_t dayidx = getsystemtimedayidx(systemtime);
      if (dayidx) {
        systemtimetotext(systemtime, St);
        SystemTimeTxt_sensor->publish_state(St);
        timertotext(St, &heattimer[dayidx][0]);
        HeatTimerTxt_sensor->publish_state(St);
        timertotext(St, &watertimer[dayidx][0]);
        WaterTimerTxt_sensor->publish_state(St);
        timertotext(St, &watercyclingtimer[dayidx][0]);
        WaterCyclingTimerTxt_sensor->publish_state(St);
      }

      txtupdate = 0;
    }
  }
};


// --------------------------------------------------------------------------------------------------------------------
// Sensor Class - Manage numerical sensors and communication protocol
class MyVitoSensorsComponent : public PollingComponent, public Sensor {
private:
  // Communication vars
  Stream* com;
  uint8_t state;
  uint8_t laststate;
  uint32_t lastms;
  uint32_t lastpollms;
  uint32_t delay;
  uint32_t loopdelay;
  uint32_t currentdelay;
  uint8_t delaydone;
  uint8_t writebuf[10];
  uint8_t c;
  uint8_t readidx;
  uint8_t synced;
  uint8_t readerror;

  uint8_t commstatus;
  uint8_t cmd;
  uint16_t addr;
  uint8_t len;
  uint32_t data;
  uint8_t writeidx;
  uint8_t readbuffer[maxreclen];

  // Other vars
  uint8_t paramidx;
  uint8_t publish;


public:
  // Temperature sensors
  Sensor *HeaterTemperature_sensor = new Sensor();
  Sensor *BoilerTemperature_sensor = new Sensor();
  Sensor *ExternalTemperature_sensor = new Sensor();
  Sensor *ExternalMeanTemperature_sensor = new Sensor();
  Sensor *AmbiantTemperature_sensor = new Sensor();
  Sensor *NormalSetTemperature_sensor = new Sensor();
  Sensor *ReducedSetTemperature_sensor = new Sensor();
  Sensor *ReceptionSetTemperature_sensor = new Sensor();
  Sensor *HotWaterSetTemperature_sensor = new Sensor();
  Sensor *HotWaterOffsetTemperature_sensor = new Sensor();

  // Burner sensors
  Sensor *BurnerTimeCounter_sensor = new Sensor();
  Sensor *BurnerConsumeCounter_sensor = new Sensor();
  Sensor *BurnerStartCounter_sensor = new Sensor();

  // Mode sensors
  Sensor *HeaterMode_sensor = new Sensor();


  void setup() override {
    ESP_LOGD(logname, "Initialise communication");
    initserial(&Serial);
    initvalue();
    state = 102;  laststate = 255;
    paramidx = 0;  synced = 0; commstatus = 0;
    delay = 0; delaydone = 0; currentdelay = 0; loopdelay = 0;
    lastms = millis(); lastpollms = millis()-20000;
    publish = 0;
  }
 
  void update() override {
    uint32_t ivalue;
    float fvalue;
    if (publish != 0) {
      // Temperatures
      fvalue = getsigned(getvalue(0x0810)) / 10.0f;  if (fvalue>=1 && fvalue<=99) HeaterTemperature_sensor->publish_state(fvalue);
      fvalue = getsigned(getvalue(0x0812)) / 10.0f;  if (fvalue>=1 && fvalue<=99) { BoilerTemperature_sensor->publish_state(fvalue); waterclimate.current = fvalue; }
      fvalue = getsigned(getvalue(0x0896)) / 10.0f;  if (fvalue>=0 && fvalue<=99) { AmbiantTemperature_sensor->publish_state(fvalue); heaterclimate.current = fvalue; }
      fvalue = getsigned(getvalue(0x5525)) / 10.0f;  if (fvalue>=-30 && fvalue<=99) ExternalTemperature_sensor->publish_state(fvalue);
      fvalue = getsigned(getvalue(0x5527)) / 10.0f;  if (fvalue>=-30 && fvalue<=99) ExternalMeanTemperature_sensor->publish_state(fvalue);

      fvalue = getsigned(getvalue(0x2306)) / 1.0f;   if (fvalue>=0 && fvalue<=99) { NormalSetTemperature_sensor->publish_state(fvalue); heaterclimate.target = fvalue; }
      fvalue = getsigned(getvalue(0x2307)) / 1.0f;   if (fvalue>=0 && fvalue<=99) ReducedSetTemperature_sensor->publish_state(fvalue);
      fvalue = getsigned(getvalue(0x2308)) / 1.0f;   if (fvalue>=0 && fvalue<=99) ReceptionSetTemperature_sensor->publish_state(fvalue);

      fvalue = getsigned(getvalue(0x6300)) / 1.0f;   if (fvalue>=0 && fvalue<=99) { HotWaterSetTemperature_sensor->publish_state(fvalue); waterclimate.target = fvalue; }
      fvalue = getsigned(getvalue(0x6760)) / 1.0f;   if (fvalue>=0 && fvalue<=99) HotWaterOffsetTemperature_sensor->publish_state(fvalue);

      // Burner counters
      // Cap value to about 60 to 100 years of usage
      ivalue = getvalue(0x088a);  if (ivalue>=0 && ivalue<=1000000) BurnerStartCounter_sensor->publish_state(ivalue); 
      fvalue = getvalue(0x08A7)/3600.0f;  if (fvalue>=0 && fvalue<=100000) BurnerTimeCounter_sensor->publish_state(fvalue); 
      fvalue = getvalue(0x7574)/10000.0f; if (fvalue>=0 && fvalue<=200000) BurnerConsumeCounter_sensor->publish_state(fvalue); 

      // Modes
      ivalue = getvalue(0x2301);  if (ivalue>=0 && ivalue<=255) { HeaterMode_sensor->publish_state(ivalue); heaterclimate.mode = ivalue; }

      txtupdate = 1;
      binupdate = 1;

      publish = 0;
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG(logname, "VitotronicComponent:");
    ESP_LOGCONFIG(logname, "  Commstatus %02d", commstatus);
  }


  // Serial communication functions
  void initserial(HardwareSerial* serial)
  {
    serial->begin(4800, SERIAL_8E2);
    com = serial;
    serial->flush();
  }

  uint8_t readcom()
  {
    uint8_t c;
    if (com->available()) {
      c = com->read();
      #ifdef DEBUG
        ESP_LOGD(logname, "DBG: Rec char %02x", c);
      #endif
    }
    else {
      c = 0;
    }
    return c;
  }

  void writecom(uint8_t *buf, uint8_t len){
    char St[2*maxreclen+1];
    char Hex[2];
    St[0] = 0;
    for (int i=0; i<len; i++) {
      sprintf(Hex, "%02X", buf[i]);
      St[2*i]=Hex[0]; St[2*i+1]=Hex[1]; St[2*i+2]=0;
    }
    #ifdef DEBUG
      ESP_LOGD(logname, "DBG: Commstatus %02d - Sending %s", commstatus, St);
    #endif
    com->write(buf, len);
  }


  // General functions
  void setdelay(uint32_t d){
    delay = d;
    delaydone = 0;
    loopdelay = 0;
    lastms = millis();
  }


  // main loop
  void loop() override {
   char StHex[2*maxreclen+1];
   char St[80];
   char Hex[2];
   float fvalue;

   if (state != laststate){
      laststate = state;
      #ifdef DEBUG
        ESP_LOGD(logname, "DBG: State changed %02d", state);
      #endif
   }    

   currentdelay = millis() - lastms;
   if (currentdelay >= delay){
     #ifdef DEBUG  
       if ((delaydone == 0) && (loopdelay>0)) ESP_LOGD(logname, "DBG: Delay of %d ms is over (%d ms - %d loop)", delay, currentdelay, loopdelay);
     #endif
     delaydone = 1;
   }
   else {
     loopdelay++;
   }

    // Communication protocol state machine (0xx = READ; 1XX = IDLE ; 2xx = WRITE)
    switch (state) {

      case 0: {
 
        cmd = 0xf7;      // Read
        addr = paramaddr[paramidx];
        len = paramlen[paramidx];
        paramidx++;

        while (com->available()) readcom();  // clean rec buffer
        if ((addr == 0) && (len == 0)) {
          state = 100;
          paramidx = 0;
        }
        else if (synced == 0){
          state = 1;
          setdelay(5000);
        }
        else {
          state = 3;
          setdelay(10);
        }
      }
      break;

      case 1: { // waiting sync 0x05
        if (com->available()) {
          c = readcom();
          if (c == 5){
             state = 2;  // Start to KW protcol
             setdelay(10);
          }
        }
        if (delaydone != 0) { // Timeout - Try sending 0x04
          writebuf[0]=4;
          writecom(writebuf, 1);
          state = 0;
          setdelay(10);
          if (commstatus > 0) commstatus--;
        }
      }
      break;

      case 2: { // Send ACK 0x01
        if (delaydone != 0){
          writebuf[0]=1;
          writecom(writebuf, 1);
          state = 3;
          setdelay(10);
        }
      }
      break;

      case 3: { // Send CMD
        if (delaydone != 0){
          writebuf[0]=cmd;
          writebuf[1]=(addr & 0xff00) >> 8;
          writebuf[2]=(addr & 0x00ff);
          writebuf[3]=len;
          writecom(writebuf, 4);
          state = 4;
          setdelay(10);
        }
      }
      break;

      case 4: { // Wait 1 loop for sending done
        if (delaydone != 0){
          state = 5;
          readidx = 0;
          readerror = 0;
          synced = 0;
          paramreaded[paramidx-1]=0;
          setdelay(26 + 16 * len);  // receive timeout delay : 42 to 372 ms
        }
      }
      break;

      case 5: { // Receiving
        if ((delaydone != 0) || (readidx == len)) {  // Timeout or received correct number of bytes
          if (readidx != len)
          {
            readerror = 1;  readidx = 0;
            ESP_LOGD(logname, "Read error - incorect length received - %d received / %d expected", readidx, len);
          }
          state = 6;
        }
        if (com->available()){
          c = readcom();
          readbuffer[readidx] = c;

          if (readidx == maxreclen) readerror = 1;
          if (readidx < maxreclen) readidx++;

          synced = 1;
        }
      }
      break;

      case 6: { // data treatment
        if (readerror == 0)
        {
          if (readidx) {
             paramreaded[paramidx-1]=readidx;
             paramvalue[paramidx-1] = readbuffer[0];
             for (int i = 1; i < readidx; i++) paramvalue[paramidx-1] += readbuffer[i] << 8*i;
             if (addr == 0x2500) memcpy(param2500status, readbuffer, sizeof(param2500status));
             if (readidx > 4){
               StHex[0] = 0;
               for (int i = 0; i < readidx; i++) {
                 sprintf(Hex, "%02X", readbuffer[i]);
                 StHex[3*i]=Hex[0]; StHex[3*i+1]=Hex[1]; StHex[3*i+2]=' '; StHex[3*i+3]=0;
               }
               St[0] = 0;
               if ((addr == 0x088e) || (addr == 0x2309) || (addr == 0x2311)) {
                 if (!(readbuffer[4] >=0 && readbuffer[4] <=7)) readbuffer[4] = 0;
                 switch (addr) {
                   case 0x088e:
                     memcpy(systemtime, readbuffer, 8);
                     systemtimetotext(systemtime, St);
                     break;
                   case 0x2309:
                     memcpy(vactimer[0], readbuffer, 8);
                     systemtimetotext(vactimer[0], St);
                     break;
                   case 0x2311:
                     memcpy(vactimer[1], readbuffer, 8);
                     systemtimetotext(vactimer[1], St);
                     break;
                 }
               }
               if (((addr & 0xfcc0) == 0x2000) && (addr < 0x22ff) && (readidx == 8)) {
                 uint8_t dayidx = ((addr & 0x0038) >> 3) + 1;  // 1..7
                 uint8_t timertype = ((addr & 0x0300) >> 8); // 0..2
                 if (timertype == 0) memcpy(&heattimer[dayidx][0], &readbuffer[0], 8);
                 else if (timertype == 1) memcpy(&watertimer[dayidx][0], &readbuffer[0], 8);
                 else if (timertype == 2) memcpy(&watercyclingtimer[dayidx][0], &readbuffer[0], 8);
                 timertotext(St, &readbuffer[0], true);
               }
               ESP_LOGD(logname, "Readed addr %04x : %02d (%02d) byte(s) - %-13s - Hex: %s %s", addr, readidx, len, paramtxt[paramidx-1], StHex, St);
             }
             else
               ESP_LOGD(logname, "Readed addr %04x : %02d (%02d) byte(s) - %-13s - Hex: %8x  Dec: %10d", addr, readidx, len,
                        paramtxt[paramidx-1], paramvalue[paramidx-1], paramvalue[paramidx-1]);
             if (commstatus < 10) commstatus++;
          } else {
             ESP_LOGD(logname, "Read error - timeout");
             if (commstatus > 0) commstatus--;
          }
        }
        else {
           ESP_LOGD(logname, "Read error");
           if (commstatus > 0) commstatus--;
        }
        state = 0;
      }
      break;

      // WAIT States
      case 100: { // minimal wait before next roll
        publish = 1;  // poll done, publish values
        state = 101;
        setdelay(2000);
      }
      break;

      case 101: { // wait min delay
        while (com->available()) readcom();  // clean rec buffer
        if (delaydone != 0){ // Timeout
          state = 102;
          synced = 0;
        }
      }
      break;

      case 102: { // wait next cycle (last+60s)
        while (com->available()) readcom();  // clean rec buffer
        if (haswrite() != -1) {
          state = 200; // Start write
          synced = 0;
        }

          if ((millis() - lastpollms)>60000) { // Timeout
          state = 0;
          synced = 0;
          lastpollms = millis();
          ESP_LOGD(logname, "------------ launch new poll cycle ------------");
        }
      }
      break;


      // WRITE States
      case 200: {

        cmd = 0xf4;      // Write

        writeidx = haswrite(); 
        addr = writeaddr[writeidx];
        len = writelen[writeidx];
        data = writedata[writeidx];

        if (getvalue(addr) != data) {

          ESP_LOGD(logname, "Write %d bytes to %04x : %d (was %d)", len, addr, data, getvalue(addr));

          while (com->available()) readcom();  // clean rec buffer
          if ((addr == 0) && (len == 0)) {
            setdelay(1000);
            state = 101;
            paramidx = 0;
          }
          else if (synced == 0){
            state = 201;
            setdelay(5000);
          }
          else {
            state = 203;
            setdelay(10);
          }
        } else {
          ESP_LOGD(logname, "No Write %d bytes to %04x : %d (same data)", len, addr, data);
          setdelay(1000);
          state = 101;
          paramidx = 0;
          writelen[writeidx] = 0; // clean write request
        }
      }
      break;

      case 201: { // waiting sync 0x05
        if (com->available()) {
          c = readcom();
          if (c == 5){
             state = 202;  // Start to KW protcol
             setdelay(10);
          }
        }
        if (delaydone != 0) { // Timeout - Try sending 0x04
          writebuf[0]=4;
          writecom(writebuf, 1);
          state = 200;
          setdelay(10);
          if (commstatus > 0) commstatus--;
        }
      }
      break;

      case 202: { // Send ACK 0x01
        if (delaydone != 0){
          writebuf[0]=1;
          writecom(writebuf, 1);
          state = 203;
          setdelay(10);
        }
      }
      break;

      case 203: { // Send CMD
        if (delaydone != 0){
          writebuf[0]=cmd;
          writebuf[1]=(addr & 0xff00) >> 8;
          writebuf[2]=(addr & 0x00ff);
          writebuf[3]=len;
          if (len == 1) { writebuf[4] = data & 0xff; }
          if (len == 2) { writebuf[4] = data & 0xff00 >>8;   writebuf[5] = data & 0x00ff; }
          writecom(writebuf, 4+len);
          state = 204;
          writelen[writeidx] = 0;  // Cmd sent, remove from send list
          setdelay(10);
        }
      }
      break;

      case 204: { // Wait 1 loop for sending done
        if (delaydone != 0){
          state = 205;
          readidx = 0;
          readerror = 0;
          synced = 0;
          setdelay(60);
        }
      }
      break;

      case 205: { // Receiving
        if (delaydone != 0){ // Timeout
          if (readidx != 1)
          {
            readerror = 1;
            ESP_LOGD(logname, "Write error - incorect length received");
            if (commstatus > 0) commstatus--;
          }
          setdelay(1000);
          state = 101;
        }
        if (com->available()){
          c = readcom();
          if (c == 0) {
            if (addr == 0x2306) { fvalue = data / 1.0f;   if (fvalue>=0 && fvalue<=99) heaterclimate.target = fvalue; }
            if (addr == 0x6300) { fvalue = data / 1.0f;   if (fvalue>=0 && fvalue<=99) waterclimate.target = fvalue; }
            setvalue(addr, data);
            ESP_LOGD(logname, "Write OK");
            publish = 1;
          } else {
            ESP_LOGD(logname, "Write error");
          }
          readidx = 1;
          synced = 1;
        }
      }
      break;

      default: {  // state Error -> init stuff again
        state = 0;
        break;
      }
    }
  }
};


// --------------------------------------------------------------------------------------------------------------------
// API Class
class MyVitoAPIComponent : public Component, public CustomAPIDevice {
 public:
  void setup() override {
    // Declare a service "start_write_cycle"
    //  - Service will be called "esphome.<NODE_NAME>_start_write_cycle" in Home Assistant.
    //  - The service has three arguments (type inferred from method definition):
    //     - address : address to write
    //     - len : data length
    //     - data : data to write (number)
    register_service(&MyVitoAPIComponent::on_start_write_cycle, "start_write_cycle",
                     {"address", "datalen", "data"});
  }

  void on_start_write_cycle(int address, int datalen, int data) {
    if (datalen >= 0 && datalen <=2) {
      if ((datalen == 1) &&
          (((address == 0x2306 || address == 0x2307 || address == 0x2308) && (data >=3 && data <= 35)) ||
           ((address == 0x6300) && (data >=10 && data <= 60)))) {
        dowrite(address, data, datalen);
        ESP_LOGD("vito_api", "Starting write cycle - SetTemp Update %04x %d", address, data);
      }
      if (address == 0x2301 && datalen == 1 && (data == MODE_STANDBY || data == MODE_HEATING || data == MODE_HOTWATER)) {
        dowrite(address, data, datalen);
        ESP_LOGD("vito_api", "Starting write cycle - Mode Set");
      }
      if (address == 0x2302 && datalen == 1 && data >=0 && data <= 1) {
        dowrite(address, data, datalen);
        if ((data == 1) && (getvalue(0x2303)==1)) dowrite(0x2303, 0, 1);
        ESP_LOGD("vito_api", "Starting write cycle - Eco mode");
      }
      if (address == 0x2303 && datalen == 1 && data >=0 && data <= 1) {
        dowrite(address, data, datalen);
        if ((data == 1) && (getvalue(0x2302)==1)) dowrite(0x2302, 0, 1);
        ESP_LOGD("vito_api", "Starting write cycle - Reception mode");
      }
    }
  }
};


// --------------------------------------------------------------------------------------------------------------------
// Climate Class for heating
class MyVitoClimateComponent : public Component, public Climate {
 public:
  void setup() override {
  }

  void loop() override {
    uint8_t val2301 = getvalue(0x2301);
    if ((heaterclimate.target != heaterclimate.target_last) || (heaterclimate.current != heaterclimate.current_last) || (heaterclimate.mode != heaterclimate.mode_last) || (val2301 != heaterclimate.mode2301)) {
      heaterclimate.target_last = heaterclimate.target;
      this->target_temperature = heaterclimate.target;
      heaterclimate.current_last = heaterclimate.current;
      this->current_temperature = heaterclimate.current;
      heaterclimate.mode_last = heaterclimate.mode;
      if (val2301 == MODE_HEATING) this->mode = CLIMATE_MODE_AUTO; else this->mode = CLIMATE_MODE_OFF;
      heaterclimate.mode2301 = val2301;
      this->publish_state();
    }
  }

  void control(const ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      // User requested mode change
      ClimateMode mode = *call.get_mode();
      uint8_t val2301 = getvalue(0x2301);
      if (mode != CLIMATE_MODE_OFF) {
        dowrite(0x2301, MODE_HEATING, 1);
      } else {
        if (val2301 != MODE_STANDBY) dowrite(0x2301, MODE_HOTWATER, 1);
      }
      ESP_LOGD("vito_climate_heater", "Climate ctrl set");

      // Publish updated state
      this->mode = mode;
      this->publish_state();
    }
    if (call.get_preset().has_value()) {
      // User requested preset change
      ClimatePreset preset = *call.get_preset();
      switch (preset) {
        case CLIMATE_PRESET_ECO:
          dowrite(0x2302, 1); dowrite(0x2303, 0);
          ESP_LOGD("vito_climate_heater", "Climate preset to ECO");
          break;
        case CLIMATE_PRESET_COMFORT:
          dowrite(0x2303, 1); dowrite(0x2302, 0);
          ESP_LOGD("vito_climate_heater", "Climate preset to CONFORT");
          break;
        default:
          preset = CLIMATE_PRESET_NONE;
          dowrite(0x2302, 0); dowrite(0x2303, 0);
          ESP_LOGD("vito_climate_heater", "Climate preset to NONE");
          break;
      }
      //this->set_preset(preset);      
    }
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      float new_target = *call.get_target_temperature();
      // Send target temp to climate
      if (new_target >= 3 && new_target <= 30) {
        ESP_LOGD("vito_climate_heater", "Climate ctrl set target temp : %4.1f", new_target);
        dowrite(0x2306, new_target, 1);
        ESP_LOGD("vito_climate_heater", "Starting write cycle - SetTempUpdate");
        heaterclimate.target = new_target;
        this->target_temperature = new_target;
      } else {
        ESP_LOGD("vito_climate_heater", "Temperature requested out of range");
      }
    }
  }

  ClimateTraits traits() override {
    // The capabilities of the climate device
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_AUTO});
    traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_COMFORT});
    traits.set_visual_min_temperature(10);
    traits.set_visual_max_temperature(30);
    traits.set_visual_temperature_step(1);
    return traits;
  }
};


// --------------------------------------------------------------------------------------------------------------------
// Climate Class for water generator
class MyVitoWaterClimateComponent : public Component, public Climate {
 public:
  void setup() override {
  }

  void loop() override {
    uint8_t val2301 = getvalue(0x2301);
    if ((waterclimate.target != waterclimate.target_last) || (waterclimate.current != waterclimate.current_last) || (waterclimate.mode != waterclimate.mode_last) || (val2301 != waterclimate.mode2301)) {
      waterclimate.target_last = waterclimate.target;
      this->target_temperature = waterclimate.target;
      waterclimate.current_last = waterclimate.current;
      this->current_temperature = waterclimate.current;
      waterclimate.mode_last = waterclimate.mode;
      if ((val2301 == MODE_HEATING) || (val2301 == MODE_HOTWATER)) this->mode = CLIMATE_MODE_AUTO; else this->mode = CLIMATE_MODE_OFF;
      waterclimate.mode2301 = val2301;
      this->publish_state();
    }
  }

  void control(const ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      // User requested mode change
      ClimateMode mode = *call.get_mode();
      uint8_t val2301 = getvalue(0x2301);
      if (mode != CLIMATE_MODE_OFF) {
        if (val2301 == MODE_STANDBY) dowrite(0x2301, MODE_HOTWATER, 1);
      } else {
        if (val2301 == MODE_HOTWATER) dowrite(0x2301, MODE_STANDBY, 1);
        mode = CLIMATE_MODE_AUTO; // HEATING only without hotwater is not supported
      }
      ESP_LOGD("vito_climate_water", "Climate ctrl set");

      // Publish updated state
      this->mode = mode;
      this->publish_state();
    }
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      float new_target = *call.get_target_temperature();
      // Send target temp to climate
      if (new_target >= 10 && new_target <= 60) {
        ESP_LOGD("vito_climate_water", "Climate ctrl set water set temp : %4.1f", new_target);
        dowrite(0x6300, new_target, 1);
        ESP_LOGD("vito_water_climate_water", "Starting write cycle - SetTempUpdate");
        waterclimate.target = new_target;
        this->target_temperature = new_target;
      } else {
        ESP_LOGD("vito_climate_water", "Temperature requested out of range");
      }
    }
  }

  ClimateTraits traits() override {
    // The capabilities of the climate device
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_AUTO});
    traits.set_supported_presets({});
    traits.set_visual_min_temperature(10);
    traits.set_visual_max_temperature(60);
    traits.set_visual_temperature_step(1);
    return traits;
  }
};
