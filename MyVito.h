// MyVitoRD

const char* logname PROGMEM = "Vitotronic";

#include <Arduino.h>
#include "esphome.h"

//#define DEBUG

const int maxparam = 99;
const int statuslen = 22;
const int maxreclen = 50;

uint16_t writeaddr;
uint8_t writelen;  // 0, 1 or 2
uint16_t writedata;
uint8_t writeexec;  // 0:Nothing; 1:ExecWrite; 2:WriteDoneOk; 3:WriteDoneKO

// full read
//  uint16_t paramaddr[maxparam] = {0x00f8, 0x5525, 0x5527, 0x0800, 0x0802, 0x0804, 0xA309, 0x0810, 0x0812, 0x0814, 0x0816, 0x0818, 0x081a, 0x0896, 0x089f, 0x2544, 0x555a, 0x080A, 0x080C,
//                                  0x551E, 0x55E3, 0x0883, 0x0842, 0x0849, 0x7574, 0x088A, 0x08A7, 0x08AB, 0x0845, 0x0846, 0x254C, 0x2906, 0x5555, 0x0847, 0x2500, 0x3500,
//                                  0xA38F, 0xA305, 0x08A1, 0x2301, 0x2302, 0x2303, 0x2304, 0x2305, 0x2306, 0x2307, 0x2308, 0x2501, 0x2502, 0x2508, 0x250A, 0x250B, 0x250C, 0x2510, 0};

//  uint8_t paramlen[maxparam]   = {2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,      2,
//                                  1,      1,      1,      1,      1,      4,      4,      4,      4,      1,      1,      1,      1,      1,      1,      statuslen,     statuslen,
//                                  2,      2,      2,      1,      1,      1,      1,      1,      1,      1,      1,      1,      4,      1,      1,      1,      2,      1,      0};

// Selected read
  uint16_t paramaddr[maxparam] = {0x00f8, 0x0800, 0x5525, 0x5527, 0x0802, 0x0810, 0x0804, 0x0812, 0x0896, 
                                  0x551E, 0x0883, 0x0842, 0x7574, 0x088A, 0x08A7, 0x0845, 0x0846, 0x2906, 0x0847, 0x2500,
                                  0x2301, 0x2302, 0x2303, 0x2304, 0x2305, 0x2306, 0x2307, 0x2308, 0x2309, 0};

  uint8_t paramlen[maxparam]   = {2,      2,      2,      2,      2,      2,      2,      2,      2,
                                  1,      1,      1,      4,      4,      4,      1,      1,      1,      1,      statuslen,
                                  1,      1,      1,      1,      1,      1,      1,      1,      4,      0};

  char paramtxt[maxparam][20]  = {"ID",   "T Ext Sonde", "T Ext Fltr", "T Ext Moy",    "T Chaud",    "T Chaud Fltr", "T Boiler",  "T Boiler Fltr",  "T Amb Fltr",
                                  "Bruleur", "Disf bruleur",    "Allure 1",    "Conso",    "Allumage",    "Temps Al. 1",     "Pompe Boiler",    "Pompe Circ",    "Pompe Chauf",    "Faute",    "Status",
                                  "Mode",    "Eco",    "Fete",    "Offset",    "Pente",    "Consigne Norm",    "Consigne Red",    "Consigne Fete",    "", ""};

  uint8_t param2500status[statuslen];
  uint32_t paramvalue[maxparam];
  uint8_t paramreaded[maxparam];

float target_temp = 0;
float target_temp_last = 0;
float current_temp = 0;
float current_temp_last = 0;
int vitomode = -1;
int vitomode_last = -1;

int txtupdate = 0;
int binupdate = 0;


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
  uint8_t idx = 0;
  while ((paramaddr[idx] != 0) && (idx < 250)) {
    if (paramaddr[idx] == addr) paramvalue[idx] = val;
    idx++;
  }
}


class MyVitoBinarySensorsComponent : public Component, public BinarySensor {

public:
  BinarySensor *WaterPriority_sensor = new BinarySensor();

  void loop() override {
    if (binupdate != 0) {
      WaterPriority_sensor->publish_state(false);
      binupdate = 0;
    }
  }

};

class MyVitoTextSensorsComponent : public Component, public TextSensor {

public:
  TextSensor *ModeTxt_sensor = new TextSensor();
  TextSensor *SetTxt_sensor = new TextSensor();
  
  void loop() override {
    if (txtupdate != 0) {

      uint8_t current_mode = getvalue(0x2301);
      uint8_t eco_mode = getvalue(0x2302);
      uint8_t reception_mode = getvalue(0x2303);
      uint8_t b2509 = param2500status[0x09];
      uint8_t b250a = param2500status[0x0a];

      char StMode[20]; strcpy(StMode, "");
      if (current_mode == 5) strcpy(StMode, "Veille");
      if (current_mode == 0) strcpy(StMode, "ECS");
      if (current_mode == 3) {
        strcpy(StMode, "Chauffage");
        if (b2509 == 3) strcat(StMode, " Normal");
        if (b2509 == 4) strcat(StMode, " Réduit");
      }
      ModeTxt_sensor->publish_state(StMode);
      char StSet[20]; strcpy(StSet, "");
      if (current_mode == 3) {
        if (b250a == 1) strcpy(StSet, "Normale");
        if (b250a == 3) strcpy(StSet, "Réduite");
      }
      SetTxt_sensor->publish_state(StSet);
      txtupdate = 0;
    }
  }
};

class MyVitoSensorsComponent : public PollingComponent, public Sensor {

public:

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

  uint8_t paramidx;

  uint8_t publish;




  // Temperature sensors
  Sensor *HeaterTemperature_sensor = new Sensor();
  Sensor *BoilerTemperature_sensor = new Sensor();
  Sensor *ExternalTemperature_sensor = new Sensor();
  Sensor *ExternalMeanTemperature_sensor = new Sensor();
  Sensor *AmbiantTemperature_sensor = new Sensor();
  Sensor *NormalSetTemperature_sensor = new Sensor();
  Sensor *ReducedSetTemperature_sensor = new Sensor();
  Sensor *ReceptionSetTemperature_sensor = new Sensor();

  // Pump sensors
  Sensor *HWGPump_sensor = new Sensor();
  Sensor *HeatingPump_sensor = new Sensor();
  Sensor *HWCirculatingPump_sensor = new Sensor();

  // Burner sensors
  Sensor *BurnerON_sensor = new Sensor();
  Sensor *BurnerDefault_sensor = new Sensor();
  Sensor *BurnerTimeCounter_sensor = new Sensor();
  Sensor *BurnerConsumeCounter_sensor = new Sensor();
  Sensor *BurnerStartCounter_sensor = new Sensor();

  Sensor *HeaterMode_sensor = new Sensor();
  Sensor *HeaterDefault_sensor = new Sensor();
  Sensor *HeaterEco_sensor = new Sensor();
  Sensor *HeateReception_sensor = new Sensor();


  void setup() override {

    ESP_LOGD(logname, "Initialise communication");
    initserial(&Serial);
    state = 102;  laststate = 255;
    paramidx = 0;  synced = 0; commstatus = 0;
    delay = 0; delaydone = 0; currentdelay = 0; loopdelay = 0;
    lastms = millis(); lastpollms = millis()-20000;
    publish = 0;

  }
 
  int getsigned(int ivalue) {
    if (ivalue < 32768) return ivalue;
    return ivalue - 65536;
  }
 
  void update() override {
    uint32_t ivalue;
    float fvalue;
    if (publish != 0) {
      // Temperatures
      fvalue = getsigned(getvalue(0x0810)) / 10.0f;  if (fvalue>=1 && fvalue<=99) HeaterTemperature_sensor->publish_state(fvalue);
      fvalue = getsigned(getvalue(0x0812)) / 10.0f;  if (fvalue>=1 && fvalue<=99) BoilerTemperature_sensor->publish_state(fvalue);
      fvalue = getsigned(getvalue(0x0896)) / 10.0f;  if (fvalue>=0 && fvalue<=99) { AmbiantTemperature_sensor->publish_state(fvalue); current_temp = fvalue; }
      fvalue = getsigned(getvalue(0x5525)) / 10.0f;  if (fvalue>=-30 && fvalue<=99) ExternalTemperature_sensor->publish_state(fvalue);
      fvalue = getsigned(getvalue(0x5527)) / 10.0f;  if (fvalue>=-30 && fvalue<=99) ExternalMeanTemperature_sensor->publish_state(fvalue);
      fvalue = getsigned(getvalue(0x2306)) / 1.0f;   if (fvalue>=0 && fvalue<=99) { NormalSetTemperature_sensor->publish_state(fvalue); target_temp = fvalue; }
      fvalue = getsigned(getvalue(0x2307)) / 1.0f;   if (fvalue>=0 && fvalue<=99) ReducedSetTemperature_sensor->publish_state(fvalue);
      fvalue = getsigned(getvalue(0x2308)) / 1.0f;   if (fvalue>=0 && fvalue<=99) ReceptionSetTemperature_sensor->publish_state(fvalue);

      // Pumps
      ivalue = getvalue(0x0845);  if (ivalue>=0 && ivalue<=1) HWGPump_sensor->publish_state(ivalue); 
      ivalue = getvalue(0x2906);  if (ivalue>=0 && ivalue<=1) HeatingPump_sensor->publish_state(ivalue); 
      ivalue = getvalue(0x0846);  if (ivalue>=0 && ivalue<=1) HWCirculatingPump_sensor->publish_state(ivalue); 

      // Burner
      ivalue = getvalue(0x551E);  if (ivalue>=0 && ivalue<=1) BurnerON_sensor->publish_state(ivalue); 
      ivalue = getvalue(0x0883);  if (ivalue>=0 && ivalue<=255) BurnerDefault_sensor->publish_state(ivalue); 
      ivalue = getvalue(0x088a);  if (ivalue>=0 && ivalue<=1000000) BurnerStartCounter_sensor->publish_state(ivalue); 
      fvalue = getvalue(0x08A7)/3600.0f;  if (fvalue>=0 && fvalue<=100000) BurnerTimeCounter_sensor->publish_state(fvalue); 
      fvalue = getvalue(0x7574)/10000.0f; if (fvalue>=0 && fvalue<=100000) BurnerConsumeCounter_sensor->publish_state(fvalue); 

      // Modes
      ivalue = getvalue(0x2301);  if (ivalue>=0 && ivalue<=255) { HeaterMode_sensor->publish_state(ivalue); vitomode = ivalue; }
      ivalue = getvalue(0x0847);  if (ivalue>=0 && ivalue<=255) HeaterDefault_sensor->publish_state(ivalue); 
      ivalue = getvalue(0x2302);  if (ivalue>=0 && ivalue<=255) HeaterEco_sensor->publish_state(ivalue); 
      ivalue = getvalue(0x2303);  if (ivalue>=0 && ivalue<=255) HeateReception_sensor->publish_state(ivalue); 

      txtupdate = 1;
      binupdate = 1;

      publish = 0;
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG(logname, "VitotronicComponent:");
//    LOG_UPDATE_INTERVAL(this);
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
   char St[3*maxreclen+1];
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
          if (paramlen[paramidx-1] > 4)
            // 350 min for 22 bytes received
            setdelay(500);
          else
            // 20 min for 2 bytes received / 50 min for 4 bytes received
            setdelay(60);
        }
      }
      break;

      case 5: { // Receiving
        if (delaydone != 0){ // Timeout
          if (paramreaded[paramidx-1] != paramlen[paramidx-1])
          {
            readerror = 1;
            ESP_LOGD(logname, "Read error - incorect length received");
          }
          if (readerror != 0) { paramvalue[paramidx-1] = 0xffffffff; paramreaded[paramidx-1]=0; }
          state = 6;
        }
        if (com->available()){
          c = readcom();
          if (readidx == 0) paramvalue[paramidx-1] = c;
          else paramvalue[paramidx-1] += c << 8*readidx;
          if ((paramaddr[paramidx-1]==0x2500) && (readidx < statuslen)) param2500status[readidx]=c;
          if (readidx==maxreclen) readerror = 1;
          if (readidx<maxreclen) readidx++;
          paramreaded[paramidx-1]=readidx;
          synced = 1;
        }
      }
      break;

      case 6: { // data treatment
        if (readerror == 0)
        {
          if (paramreaded[paramidx-1]) {
             if (paramreaded[paramidx-1] > 4){
               St[0] = 0;
               for (int i=0; i<paramreaded[paramidx-1]; i++) {
                 sprintf(Hex, "%02X", param2500status[i]);
                 St[3*i]=Hex[0]; St[3*i+1]=Hex[1]; St[3*i+2]=' '; St[3*i+3]=0;
               }
               ESP_LOGD(logname, "Readed addr %04x : %02d (%02d) byte(s) - %-13s - Hex: %s", paramaddr[paramidx-1], paramreaded[paramidx-1], paramlen[paramidx-1], paramtxt[paramidx-1], St);
             }
             else
               ESP_LOGD(logname, "Readed addr %04x : %02d (%02d) byte(s) - %-13s - Hex: %8x  Dec: %10d", paramaddr[paramidx-1], paramreaded[paramidx-1], paramlen[paramidx-1],
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
        if (writeexec == 1) {
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
        addr = writeaddr;
        len = writelen;

        if (getvalue(addr) != writedata) {

          ESP_LOGD(logname, "Write %d bytes to %d : %d (was %d)", len, addr, writedata, getvalue(addr));

          while (com->available()) readcom();  // clean rec buffer
          if ((addr == 0) && (len == 0)) {
            setdelay(1000);
            state = 101;
            paramidx = 0;
            writeexec = 3;
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
          ESP_LOGD(logname, "No Write %d bytes to %d : %d (same data)", len, addr, writedata);
          setdelay(1000);
          state = 101;
          paramidx = 0;
          writeexec = 3;
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
          if (len == 1) { writebuf[4] = writedata & 0xff; }
          if (len == 2) { writebuf[4] = writedata & 0xff00 >>8;   writebuf[5] = writedata & 0x00ff; }
          writecom(writebuf, 4+len);
          state = 204;
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
            writeexec = 2;
            if (addr == 0x2301) { if (writedata>=0 && writedata<=255) HeaterMode_sensor->publish_state(writedata); } 
            if (addr == 0x2302) { if (writedata>=0 && writedata<=255) HeaterEco_sensor->publish_state(writedata); } 
            if (addr == 0x2303) { if (writedata>=0 && writedata<=255) HeateReception_sensor->publish_state(writedata); } 
            if (addr == 0x2306) { fvalue = writedata / 1.0f;   if (fvalue>=0 && fvalue<=99) { NormalSetTemperature_sensor->publish_state(fvalue); target_temp = fvalue; } }
            if (addr == 0x2307) { fvalue = writedata / 1.0f;   if (fvalue>=0 && fvalue<=99) ReducedSetTemperature_sensor->publish_state(fvalue); }
            if (addr == 0x2308) { fvalue = writedata / 1.0f;   if (fvalue>=0 && fvalue<=99) ReceptionSetTemperature_sensor->publish_state(fvalue); }
            setvalue(addr, writedata);
            ESP_LOGD(logname, "Write OK"); } else {
            writeexec = 3;
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

// API Class
class MyVitoAPIComponent : public Component, public CustomAPIDevice {
 public:
  void setup() override {
    // This will be called once to set up the component
    // think of it as the setup() call in Arduino
    writeexec = 0;
    writelen = 0;
    writedata = 0;
    writeaddr = 0;

    // Declare a service "hello_world"
    //  - Service will be called "esphome.<NODE_NAME>_hello_world" in Home Assistant.
    //  - The service has no arguments
    //  - The function on_hello_world declared below will attached to the service.
    register_service(&MyVitoAPIComponent::on_hello_world, "hello_world");

    // Declare a second service "start_write_cycle"
    //  - Service will be called "esphome.<NODE_NAME>_start_washer_cycle" in Home Assistant.
    //  - The service has three arguments (type inferred from method definition):
    //     - cycle_duration: integer
    //     - silent: boolean
    //     - string_argument: string
    //  - The function start_washer_cycle declared below will attached to the service.
    register_service(&MyVitoAPIComponent::on_start_write_cycle, "start_write_cycle",
                     {"address", "datalen", "data"});

    // Subscribe to a Home Assistant state "sensor.temperature"
    //  - Each time the ESP connects or Home Assistant updates the state, the function
    //    on_state_changed will be called
    //  - The state is a string - if you want to use it as an int you must parse it manually
    //subscribe_homeassistant_state(&MyVitoAPIComponent::on_state_changed, "sensor.temperature");
  }
  void on_hello_world() {
    ESP_LOGD("vito_api", "Hello World!");

    if (is_connected()) {
      // Example check to see if a client is connected
    }
  }
  void on_start_write_cycle(int address, int datalen, int data) {
    if (datalen >= 0 && datalen <=2) {
      if ((address == 0x2306 || address == 0x2307 || address == 0x2308) && datalen == 1 && data >=3 && data <= 35) {
        writelen = datalen;
        writedata = data;
        writeaddr = address;
        ESP_LOGD("vito_api", "Starting write cycle - SetTempUpdate");
        writeexec = 1;
      }
      if (address == 0x2301 && datalen == 1 && (data == 0 || data == 3 || data == 5)) {
        writelen = datalen;
        writedata = data;
        writeaddr = address;
        ESP_LOGD("vito_api", "Starting write cycle - Mode Set");
        writeexec = 1;
      }
      if ((address == 0x2302 || address == 0x2303) && datalen == 1 && data >=0 && data <= 1) {
        writelen = datalen;
        writedata = data;
        writeaddr = address;
        ESP_LOGD("vito_api", "Starting write cycle - Eco/Reception mode");
        writeexec = 1;
      }
    }

    // Call a homeassistant service
    //call_homeassistant_service("homeassistant.service");
  }
  //void on_state_changed(std::string state) {
  //  ESP_LOGD(TAG, "Temperature has changed to %s", state.c_str());
  //}
};

// Climate Class
class MyVitoClimateComponent : public Component, public Climate {
 public:
  void setup() override {
    // This will be called by App.setup()
  }
  void loop() override {
    if ((target_temp != target_temp_last) || (current_temp != current_temp_last) || (vitomode != vitomode_last)) {
      target_temp_last = target_temp;
      this->target_temperature = target_temp;
      current_temp_last = current_temp;
      this->current_temperature = current_temp;
      vitomode_last = vitomode;
      if (vitomode == 3) this->mode = CLIMATE_MODE_AUTO; else this->mode = CLIMATE_MODE_OFF;
      this->publish_state();
    }
  }
  void control(const ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      // User requested mode change
      ClimateMode mode = *call.get_mode();
      // Send mode to hardware
      // ...
      ESP_LOGD("vito_climate", "Climate ctrl set");

      // Publish updated state
      this->mode = mode;
      this->publish_state();
    }
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      float temp = *call.get_target_temperature();
      // Send target temp to climate
      // ...
      if (temp >= 3 && temp <= 30) {
        ESP_LOGD("vito_climate", "Climate ctrl set target temp : %4.1f", temp);
        writelen = 1;
        writedata = temp;
        writeaddr = 0x2306;
        ESP_LOGD("vito_climate", "Starting write cycle - SetTempUpdate");
        writeexec = 1;
        target_temp = temp;
        this->target_temperature = temp;
      } else {
        ESP_LOGD("vito_climate", "Temperature requested out of range");
      }
    }
  }
  ClimateTraits traits() override {
    // The capabilities of the climate device
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_AUTO});
    traits.set_supported_presets({climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_COMFORT});
    //traits.set_visual_min_temperature(10);
    //traits.set_visual_max_temperature(30);
    //traits.set_visual_temperature_step(1);
    return traits;
  }
};
