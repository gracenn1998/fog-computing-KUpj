#include <WiFi.h>
#include <PubSubClient.h>
#include <M5Stack.h>
#include <ArduinoJson.h>
#include "MAX30100.h"

#define SAMPLING_RATE                       MAX30100_SAMPRATE_100HZ
#define IR_LED_CURRENT                      MAX30100_LED_CURR_50MA
#define RED_LED_CURRENT                     MAX30100_LED_CURR_27_1MA
#define PULSE_WIDTH                         MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE                        true

#define REPORTING_PERIOD_MS     10
MAX30100 sensor;

uint32_t tsLastReport = 0;
#define PERIOD 60
 
// Wi-Fi の ​​SSID
//char *ssid = "ifdl";
//char *password = "hogeupip5";
char *ssid = "A";
char *password = "cfkx3236";
// MQTT connection destination IP
const char *endpoint = "192.168.43.131";
// MQTT port
const int port = 1883;
// Device ID
char * deviceID = "M5Stack"; // Device ID must be unique for each device
// topic that informs the message
char *pubTopic = "/pub/M5Stack";
// topic waiting for message
char *subTopic = "/sub/M5Stack";
 
////////////////////////////////////////////////////////////////////////////////
   
WiFiClient httpsClient;
PubSubClient mqttClient(httpsClient);




void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 for (int i=0;i<length;i++) {
  char receivedChar = (char)payload[i];
  Serial.print(receivedChar);
  Serial.println();
  }
}
   
void setup() {
    Serial.begin(115200);
     
    // Initialize the M5Stack object
    M5.begin();
 
    // START
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor (10, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.printf("START");
     
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
    M5.Lcd.setCursor (10, 40);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.printf("WiFi Connected.");

    
    mqttClient.setServer(endpoint, port);
    mqttClient.setCallback(callback);
    connectMQTT();
    

    if (!sensor.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }
    sensor.setMode(MAX30100_MODE_SPO2_HR);
    sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
    sensor.setLedsPulseWidth(PULSE_WIDTH);
    sensor.setSamplingRate(SAMPLING_RATE);
    sensor.setHighresModeEnabled(HIGHRES_MODE);
}
   
void connectMQTT() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect(deviceID)) {
            Serial.println("Connected.");
            int qos = 0;
            mqttClient.subscribe(subTopic, qos);
            Serial.println("Subscribed.");
        } else {
            Serial.print("Failed. Error state=");
            Serial.print(mqttClient.state());
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
   
long messageSentAt = 0;
int count = 0;
char pubMessage [128];
int led,red,green,blue;

  
void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}
 
//void loop() {
// 
//  // Always check and restore if disconnected
//  mqttLoop();
// 
//  // skip message every 5 seconds
//  long now = millis();
//  if (now - messageSentAt > 5000) {
//      messageSentAt = now;
//      sprintf(pubMessage, "{\"count\": %d}", count++);
//      Serial.print("Publishing message to topic ");
//      Serial.println(pubTopic);
//      Serial.println(pubMessage);
//      
//      mqttClient.publish(pubTopic, buffer);
//      Serial.println("Published.");
//  }
//}

const int capacity = JSON_OBJECT_SIZE(3);

char buffer[512];

void loop()
{
    StaticJsonDocument<200> record;
    record["did"] = 1;
    record["stype"] = 1;
    // Make sure to call update as fast as possible
    sensor.update();
    uint16_t ir, red;
    
    JsonObject val = record.createNestedObject("val");
    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        while (sensor.getRawValues(&ir, &red)) {
          Serial.print(ir);
          Serial.print('\t');
          Serial.println(red);
          val["ir"] = ir;
          val["red"] = red;
          tsLastReport = millis();
        }

        
        
//        doc["timestampt"] = millis();
        serializeJson(record, buffer);
////        doc["lon"] = 2.293491;
        serializeJson(record, Serial);
        Serial.println("");
//        mqttClient.publish("/ir", buffer);
        
    }
}
