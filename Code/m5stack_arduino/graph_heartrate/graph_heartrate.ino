#include <M5Stack.h>
#include <Wire.h>
#include "MAX30100.h"
#include "MAX30100_PulseOximeter.h"

#define BPM_INTERVAL 10 //SAMPLING EVERY 10milliSECS
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


PulseOximeter pox;

// Sampling is tightly related to the dynamic range of the ADC.
// refer to the datasheet for further info
#define SAMPLING_RATE                       MAX30100_SAMPRATE_100HZ

// The LEDs currents must be set to a level that avoids clipping and maximises the
// dynamic range
#define IR_LED_CURRENT                      MAX30100_LED_CURR_50MA
#define RED_LED_CURRENT                     MAX30100_LED_CURR_27_1MA

// The pulse width of the LEDs driving determines the resolution of
// the ADC (which is a Sigma-Delta).
// set HIGHRES_MODE to true only when setting PULSE_WIDTH to MAX30100_SPC_PW_1600US_16BITS
#define PULSE_WIDTH                         MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE                        true


// Instantiate a MAX30100 sensor class
MAX30100 sensor;
void onBeatDetected()
{
//    Serial.println("Beat!");
//    m5Clear(0, 50, 100, 120);
//    M5.Lcd.setCursor(5,50);
//    M5.Lcd.drawBitmap(5, 50, 320, 240, (uint16_t *)imgName);
}

//global var
int nextCursor, curCursor;


void setup() {
  // put your setup code here, to run once:
    M5.begin();
    M5.Speaker.setVolume(0);
    dacWrite (25, 0); //turn off noise??? 
    M5.lcd.clear();
    M5.lcd.setBrightness(BRIGHTNESS);
    M5.update();
    
    Serial.begin(115200);
    Serial.print("Initializing pulse oximeter..");

    // Initialize the PulseOximeter instance
    // Failures are generally due to an improper I2C wiring, missing power supply
    // or wrong target chip
    
    if (!pox.begin()) {
      Serial.println("FAILED");
      for(;;);
    } else {
        Serial.println("SUCCESS");
    }
    pox.setOnBeatDetectedCallback(onBeatDetected);

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
    
    sensProc(0);

    static int graphBeatMin, graphBeatMax;
    graphBeatMin = 0;
    graphBeatMax = 100;
}

float pre_w = 0;
int val;

  int tsLastReport;
void loop() {
//  sensor.update();
  pox.update();
  if (millis() - tsLastReport > 100) {
        Serial.print("Heart rate:");
        Serial.print(pox.getHeartRate());
        Serial.print("bpm / SpO2:");
        Serial.print(pox.getSpO2());
        Serial.println("%");

        tsLastReport = millis();
    }
    sensor.update();  
  sensProc(1);
  
}
//
//float pre_w = 0;

void sensProc(int s_cmd) {
  static unsigned long preTime, preB_Time;
  static int preVal;
  static int highCnt, lowCnt, bpmCnt;
//  static int bpmSum, bpmMax, bpmMin, bpmCnt, bpmAvg, bpmMaxTmp, bpmMinTmp;
  static int bpm;
  float alpha = 0.95;

  
  uint16_t ir, red;
  switch(s_cmd) {
    case S_SETUP:
      preTime = preB_Time = millis();
      bpm = 0;
      highCnt = lowCnt = bpmCnt = 0;
      curCursor = nextCursor = GRAPH_X_MIN;
      nextCursor++;
//      bpmMinTmp = MAX_BPM_VAL; //Temp val
//      bpmMaxTmp = 0;
      
//      sensor.update();
      
//      for(int i = 0; i < 20; i++) {
//        //display waiting - setup screen
        while (sensor.getRawValues(&ir, &red)) {
          sensor.getRawValues(&ir, &red);
          preVal = dcRemoval(ir, &pre_w, alpha);
//          Serial.println(preVal);
        }
//        delay(10);
//      }
      
      break;
    case S_LOOP:
      if(millis() - preB_Time > BPM_INTERVAL) {//update      
//        pox.update();
//        bpm = pox.getHeartRate();
        
//        Serial.println("BPM: " + String(pox.getHeartRate()));
//        if(mode == MODE_GRAPH) {
//          lcdProc()  
//        }
        
        sensor.update();
        while (sensor.getRawValues(&ir, &red)) {
          val = dcRemoval(ir, &pre_w, alpha);
          preB_Time = millis();
        }
//        Serial.println("Val: " + String(val));
//        //clear next vitical line
        M5.Lcd.drawLine(nextCursor, GRAPH_Y_MIN, nextCursor, GRAPH_Y_MAX, GRAPH_BG);
        M5.Lcd.drawLine(nextCursor, GRAPH_Y_MIDLINE, nextCursor, GRAPH_Y_MIDLINE, GRAPH_MIDLINE);
        M5.Lcd.drawLine(curCursor, graphYFilter(preVal), curCursor, graphYFilter(val), GRAPH_FG);
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
        
        preVal = val;
        preB_Time = millis();
      }
      break;
  }
}

int graphYFilter(float v) {
  int max, min;
  max = 100;
  min = -100;
  v = round(v/1.5);
  if(v > max) v = max;
  if(v < min) v = min;
  return((int)((GRAPH_Y_MIDLINE - v)));// - (((v - min) * GRAPH_Y_MAX) / (max - min))));
}

float dcRemoval(int x, float *pre_w, float alpha) {
  float w = x + alpha*(*pre_w);
  Serial.println("x: " + String(x)+ "\t" + "w: " + String(w) + "\t" + "pre: " + String(*pre_w));
  float result = w - *pre_w;
  *pre_w = w;
//  Serial.println(result);
  return result;
}
