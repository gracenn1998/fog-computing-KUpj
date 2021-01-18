#include <M5Stack.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <MAX30100_PulseOximeter.h>
//meomeo
//char ssid[] = "ifdl";
//char password[] = "hogeupip5";
//const char *endpoint = "192.168.11.6";

char ssid[] = "A";
char password[] = "cfkx3236";
const char *endpoint = "192.168.43.131";
// MQTT port
const int port = 1883;
char *deviceID = "1";// Device ID must be unique for each device
// topic that informs the message
char *pubTopic = "/pulseoximeter";
// topic waiting for message
//char *subTopic = "/sub/M5Stack";
WiFiClient httpsClient;
PubSubClient mqttClient(httpsClient);


#define imgName heart_icon2
#define PicArray extern unsigned char
PicArray imgName[];

#define REPORTING_PERIOD_MS     1000
#define DISPLAY_PERIOD_MS     5*1000
#define MEAN_FILTER_SIZE 150

#define GRAPH_MODE 1
#define INFO_MODE 0
#define PULOXY_INTERVAL 10 //SAMPLING EVERY 10milliSECS
#define S_SETUP 0
#define S_LOOP 1

#define BRIGHTNESS 200
#define GRAPH_X_MAX 300
#define GRAPH_X_MIN 20
#define GRAPH_Y_MIDLINE 130
#define GRAPH_Y_MAX 240
#define GRAPH_Y_MIN 0
#define GRAPH_SIZING 200
#define GRAPH_BG BLACK
#define GRAPH_FG WHITE
#define GRAPH_MIDLINE RED




//PulseOximeter pox;
MAX30100 sensor;
#define SAMPLING_RATE                       MAX30100_SAMPRATE_100HZ
#define IR_LED_CURRENT                      MAX30100_LED_CURR_50MA
#define RED_LED_CURRENT                     MAX30100_LED_CURR_27_1MA
#define PULSE_WIDTH                         MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE                        true


//global var
int displayMode = -1;
int soundOn = 0;
int mqttSendOn = 0;
//timestamp
unsigned long prePul_Time, preO2_Time, preInfoDisplay_Time;
//for graph
float w, pre_w;
float pre_irVal, irVal, redVal;
int nextCursor, curCursor;
uint16_t ir, red;
float alpha = 0.95;
//for info
float sumHRate = 0;
float sumSpO2 = 0;
float hRate = 0;
int SpO2 = 0;
int cnt = 0;
int preAvgHRate = 0;

struct MeanDiffFilter
{
  float values[MEAN_FILTER_SIZE];
  byte index;
  float sum;
  byte count;
};
MeanDiffFilter meanDiffFilter;
//for mean diff filter
struct ButterworthFilter
{
  float v[2];
  float result;
};
ButterworthFilter butterworthFilter;


void setup() {
  // put your setup code here, to run once:
    M5.begin();
//    M5.Speaker.setVolume(0);
//    dacWrite (25, 0); //turn of noise??? 
    M5.lcd.clear();
    M5.lcd.setBrightness(BRIGHTNESS);
    M5.update();
    
    Serial.begin(115200);

    // Start WiFi
    Serial.println("Connecting to ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);
   
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // WiFi Connected
    Serial.println("\nWiFi Connected.");
    
    mqttClient.setServer(endpoint, port);
//    mqttClient.setCallback(callback);
    connectMQTT();
    
    Serial.print("Initializing pulse oximeter..");
//    if (!pox.begin()) {
//        Serial.println("FAILED");
//        for(;;);
//    } else {
//        Serial.println("SUCCESS");
//    }
//    pox.setOnBeatDetectedCallback(onBeatDetected);
    if (!sensor.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }
    // Set up the wanted parameters
    sensor.setMode(MAX30100_MODE_SPO2_HR);
    sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
    sensor.setLedsPulseWidth(PULSE_WIDTH);
    sensor.setSamplingRate(SAMPLING_RATE);
    sensor.setHighresModeEnabled(HIGHRES_MODE);

//    initProc();
}


const int capacity = JSON_OBJECT_SIZE(3);
char buffer[512];

void loop() {
  M5.update();
//  pox.update();
  sensor.update();
  mqttLoop();
  readMode();

  StaticJsonDocument<200> record;
  record["did"] = 1;
  record["stype"] = 1;
  JsonObject val = record.createNestedObject("val");

  //record for request-on: <did?>, msg:on
  

  if(millis() - prePul_Time > PULOXY_INTERVAL) {//update
//    pox.update();
//    if (meanDiffFilter.count != 0) {
//      avg = meanDiffFilter.sum/meanDiffFilter.count;
//    }
    while (sensor.getRawValues(&ir, &red)) {
      irVal = irFiltering(ir);
//      redVal = red;
      prePul_Time = millis();
    }
//    Serial.println("Val: " + String(irVal));
    
    val["ir"] = irVal;
//    val["red"] = redVal;
    serializeJson(record, buffer);
    serializeJson(record, Serial);
    Serial.println("");
    if(mqttSendOn) {
      mqttClient.publish("/pulseoximeter", buffer);
      //publish + subcribe monitor/did 
    }
    
    displayProc();
    
    pre_irVal = irVal;
  }
  
//    infoScreen();
//    M5.Lcd.setCursor(250, 5);
//    m5Clear(250, 5, 20, 10);
//    m5Print("10:36", 2, WHITE);
//        M5.Lcd.setTextSize(2);
//        M5.Lcd.setTextColor(WHITE);
//        M5.Lcd.print("10:36");
}
//#define BEEP_FREQ_1 2000 // beep frequency for low bpm
//#define BEEP_FREQ_2 4000 // beep frequency for high bpm
void onBeatDetected()
{ 
  if(soundOn) {
    M5.Speaker.tone(2000, 50);
  }
    Serial.println("Beat!");
    
//    m5Clear(0, 50, 100, 120);
//    M5.Lcd.setCursor(5,50);
//    M5.Lcd.drawBitmap(5, 50, 320, 240, (uint16_t *)imgName);
}

void connectMQTT() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect(deviceID)) {
            Serial.println("Connected.");
            int qos = 0;
//            mqttClient.subscribe(subTopic, qos);
            Serial.println("Subscribed.");
        } else {
            Serial.print("Failed. Error state=");
            Serial.print(mqttClient.state());
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
 
void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}


void m5Clear(int x, int y, int w, int h) {
  M5.Lcd.fillRect(x, y, w, h, BLACK);
}
void m5Print(String content, int tsize, uint16_t color) {
  M5.Lcd.setTextSize(tsize);
  M5.Lcd.setTextColor(color);
  M5.Lcd.print(content);
}
float minBPM = 4096;
void infoScreen() {
  
//  pox.update();
  // Asynchronously dump heart rate and oxidation levels to the serial
  // For both, a value of 0 means "invalid"
  if (millis() - preO2_Time > REPORTING_PERIOD_MS) {
//    pox.update();
    float bpm;// = pox.getHeartRate();
    float spo2;// = pox.getSpO2();
    if(bpm!=0 && bpm < minBPM) {
      minBPM = bpm;
    }
    if(bpm == 0) {
      bpm = 50;
    }
    if(spo2 == 0) {
      spo2 = 94;
    }
//    Serial.print("Heart rate:");
//    Serial.print(pox.getHeartRate());
//    Serial.print("bpm / SpO2:");
//    Serial.print(pox.getSpO2());
    Serial.print("Heart rate:");
    Serial.print(bpm);
    Serial.print("bpm / SpO2:");
    Serial.print(spo2);
      Serial.println("%");
//    hRate= bpm;
//    SpO2 = spo2;
    sumHRate+=bpm;
    sumSpO2+=spo2;
    cnt++;
 
    preO2_Time = millis();
  }

  if (preInfoDisplay_Time==0 ||millis() - preInfoDisplay_Time > DISPLAY_PERIOD_MS) {
      float avgHRate = sumHRate/cnt;
      float avgSpO2 = sumSpO2/cnt;
      if(preInfoDisplay_Time==0) {
        avgHRate = hRate;
        avgSpO2 = SpO2;
      }
      
      M5.Lcd.setCursor(130, 75);
      m5Clear(130, 75, 200, 100);
      m5Print(String((int)avgHRate), 8, WHITE);
      M5.Lcd.setTextSize(3);
      M5.Lcd.setTextColor(RED);
      M5.Lcd.print("BPM");
    
      M5.Lcd.setCursor(5, 150);
      m5Clear(5, 150, 300, 50);
      if(millis() - preInfoDisplay_Time >= 60000) {
        String minute = String((int)((millis() - preInfoDisplay_Time)/60000)) + "min(s)"; 
        m5Print(minute, 2, WHITE);
      }
      else {
        String seconds = String((int)((millis() - preInfoDisplay_Time)/1000)) + "secs"; 
        m5Print(seconds, 2, WHITE);
      }
      String preirVal = " ago: " + String(preAvgHRate) + "bpm";
      m5Print(preirVal, 2, WHITE);
    
      M5.Lcd.setCursor(100, 200);
      m5Clear(100, 200, 300, 50);
      m5Print(String((int)avgSpO2), 3, WHITE);
      m5Print("%", 3, WHITE);

      
//      Serial.print("cnt: ");
//      Serial.println(cnt);
//      Serial.print("sum: ");
//      Serial.println(sumSpO2);
      
      preInfoDisplay_Time = millis();
      sumHRate=0;
      sumSpO2=0;
      cnt=0;
      preAvgHRate=avgHRate;
  }
}


void graphScreen() {
//  sensor.update();
  static int highCnt, lowCnt, bpmCnt;
//  static int bpmSum, bpmMax, bpmMin, bpmCnt, bpmAvg, bpmMaxTmp, bpmMinTmp;
  static int bpm;
//  if(millis() - prePul_Time > PULOXY_INTERVAL) {//update      
//        pox.update();
//        bpm = pox.getHeartRate();
    
//        Serial.println("BPM: " + String(pox.getHeartRate()));
//        if(mode == MODE_GRAPH) {
//          lcdProc()  
//        }
    
//    sensor.update();
    float avg =  0;
    
//        //clear next vitical line
    M5.Lcd.drawLine(nextCursor, GRAPH_Y_MIN, nextCursor, GRAPH_Y_MAX, GRAPH_BG);
    M5.Lcd.drawLine(nextCursor, GRAPH_Y_MIDLINE, nextCursor, GRAPH_Y_MIDLINE, GRAPH_MIDLINE);
    M5.Lcd.drawLine(curCursor, graphYFilter(pre_irVal), curCursor, graphYFilter(irVal), GRAPH_FG);
    curCursor = nextCursor;
    nextCursor++;
    curCursor = curCursor%GRAPH_X_MAX;
    nextCursor = nextCursor%GRAPH_X_MAX;
    if(nextCursor == 0) {
      nextCursor = GRAPH_X_MIN;
    }
//        Serial.print(nextCursor);
    if(curCursor == 0) {// clear 1st virtical line area
      curCursor = GRAPH_X_MIN;
      M5.Lcd.drawLine(GRAPH_X_MIN,GRAPH_Y_MIDLINE, GRAPH_X_MIN, GRAPH_Y_MIDLINE + GRAPH_Y_MAX, GRAPH_BG);
      nextCursor = GRAPH_X_MIN+1;
    }
    
//    pre_irVal = irVal;
//    prePul_Time = millis();
//  }
}

void initProc() {
  M5.Lcd.clear();
  initScreen();
  initSensor();  
}

void initScreen() {
  M5.Lcd.clear();
  switch(displayMode) {
    case INFO_MODE:
      M5.Lcd.setCursor(5, 0);
      m5Print("Heart Rate", 3, RED);
      M5.Lcd.drawBitmap(5, 50, 320, 240, (uint16_t *)imgName);
      M5.Lcd.setCursor(5, 200);
      m5Print("SpO2: ", 3, WHITE);
      break;
    case GRAPH_MODE:
      M5.Lcd.drawLine(GRAPH_X_MIN, GRAPH_Y_MIDLINE, GRAPH_X_MAX, GRAPH_Y_MIDLINE, GRAPH_MIDLINE);
      break;
  }
}

void initSensor() {
  switch(displayMode) {
    case INFO_MODE:
    
      break;
    case GRAPH_MODE:
      static float pre_w;
      prePul_Time = millis();
//      bpm = 0;
//      highCnt = lowCnt = bpmCnt = 0;
      curCursor = nextCursor = GRAPH_X_MIN;
      nextCursor++;
//      bpmMinTmp = MAX_BPM_VAL; //Temp val
//      bpmMaxTmp = 0;      
//      for(int i = 0; i < 20; i++) {
//        //display waiting - setup screen
//        while (sensor.getRawValues(&ir, &red)) {
//          sensor.getRawValues(&ir, &red);
//          preirVal = dcRemoval(ir, &pre_w, alpha);
//        }
//        delay(10);
//      }
      break;
  }
}


int graphYFilter(float v) {
  int max, min;
  max = 75;
  min = -75;
  v = round(v/2.5);
//  if(v > max) v = max;
//  if(v < min) v = min;
  return((int)((GRAPH_Y_MIDLINE - v)));// - (((v - min) * GRAPH_Y_MAX) / (max - min))));
}

float irFiltering(float ir) {
  float irVal, result;
  irVal = dcRemoval(ir, &pre_w, alpha);
  irVal = meanDiff(irVal, &meanDiffFilter);
  lowPassButterworthFilter(irVal, &butterworthFilter);
  result = butterworthFilter.result;
  return result;
}

float dcRemoval(int x, float *pre_w, float alpha) {
  float w = x + alpha*(*pre_w);
//  Serial.println("x: " + String(x)+ "\t" + "w: " + String(w) + "\t" + "pre: " + String(*pre_w));
  float result = w - *pre_w;
  *pre_w = w;
//  Serial.println(result);
  return result;
}

float meanDiff(float v, MeanDiffFilter* filterValues)
{
  float avg = 0;

  filterValues->sum -= filterValues->values[filterValues->index];
  filterValues->values[filterValues->index] = v;
  filterValues->sum += filterValues->values[filterValues->index];

  filterValues->index++;
  filterValues->index = filterValues->index % MEAN_FILTER_SIZE;

  if(filterValues->count < MEAN_FILTER_SIZE)
    filterValues->count++;
    
  avg = filterValues->sum / filterValues->count;
//  Serial.println("avg:" + String(avg));
  return avg - v;
}

void lowPassButterworthFilter(float x, ButterworthFilter * filterResult)
{  
  filterResult->v[0] = filterResult->v[1];

  //Fs = 100Hz and Fc = 10Hz
  filterResult->v[1] = (2.452372752527856026e-1 * x) + (0.50952544949442879485 * filterResult->v[0]);

  //Fs = 100Hz and Fc = 4Hz
  //filterResult->v[1] = (1.367287359973195227e-1 * x) + (0.72654252800536101020 * filterResult->v[0]); //Very precise butterworth filter 

  filterResult->result = filterResult->v[0] + filterResult->v[1];
}

StaticJsonDocument<200> requestMsg;
void readMode() {
  char* reqChannel = "/monitor/request";
  char* resChannel = "/monitor/response/1";
  if(M5.BtnA.wasReleased()){
    Serial.println("A");
    if(displayMode != INFO_MODE) {
      displayMode = INFO_MODE;
      preInfoDisplay_Time = 0;
      initProc();
      Serial.println("info");
    }
  }
  if (M5.BtnC.wasReleased()){
    Serial.println("C");
    if(displayMode != GRAPH_MODE) {
      displayMode = GRAPH_MODE;
      initProc();
      Serial.println("GRAPH");
    }
  }
  if (M5.BtnB.wasReleased()){
    Serial.println("mqtt send mode change");
    if(mqttSendOn) {
      mqttSendOn = false;
      requestMsg["req"] = "off";
      requestMsg["targetID"] = "1";
      serializeJson(requestMsg, buffer);
      mqttClient.publish(reqChannel, buffer);
      mqttClient.unsubscribe(resChannel);
      //publish:on + subcribe monitor/did 
    }
    else {
      mqttSendOn = true;
      requestMsg["req"] = "on";
      requestMsg["targetID"] = "1";
      serializeJson(requestMsg, buffer);
      mqttClient.publish(reqChannel, buffer);
      mqttClient.subscribe(resChannel);
      //publish:off + unsubcribe? 
    }
  }
  
}
void displayProc() {
  switch(displayMode) {
    case INFO_MODE:
      infoScreen();
      break;
    case GRAPH_MODE:
      graphScreen();
//      M5.Lcd.clear();
      break;
  }
}
