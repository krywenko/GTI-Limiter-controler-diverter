/*
 emonTx Shield 4 x CT + Voltage example
 
 An example sketch for the emontx Arduino shield module for
 CT and AC voltage sample electricity monitoring. Enables real power and Vrms calculations. 
 
 Part of the openenergymonitor.org project
 Licence: GNU GPL V3
 
 Authors: Glyn Hudson, Trystan Lea
 Builds upon JeeLabs RF12 library and Arduino
 
 emonTx documentation:   http://openenergymonitor.org/emon/modules/emontxshield/
 emonTx firmware code explination: http://openenergymonitor.org/emon/modules/emontx/firmware
 emonTx calibration instructions: http://openenergymonitor.org/emon/modules/emontx/firmware/calibration
 THIS SKETCH REQUIRES:
 Libraries in the standard arduino libraries folder:
  - JeeLib    https://github.com/jcw/jeelib
  - EmonLib   https://github.com/openenergymonitor/EmonLib.git
 Other files in project directory (should appear in the arduino tabs above)
  - emontx_lib.ino
 
*/

/*Recommended node ID allocation
------------------------------------------------------------------------------------------------------------
-ID-  -Node Type- 
0 - Special allocation in JeeLib RFM12 driver - reserved for OOK use
1-4     - Control nodes 
5-10  - Energy monitoring nodes
11-14 --Un-assigned --
15-16 - Base Station & logging nodes
17-30 - Environmental sensing nodes (temperature humidity etc.)
31  - Special allocation in JeeLib RFM12 driver - Node31 can communicate with nodes on any network group
-------------------------------------------------------------------------------------------------------------
emonhub.conf node decoder:
See: https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md
[[6]]
    nodename = emonTxShield
    firmware =emonTxShield
    hardware = emonTxShield
    [[[rx]]]
       names = power1, power2, power3, power4, Vrms
       datacode = h
       scales = 1,1,1,1,0.01
       units =W,W,W,W,V
       
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <MCP3008s.h>
#define CS_PIN 15    ///edit these vlusee in the emonlib.cpp
#define CLOCK_PIN 14
#define MOSI_PIN 13
#define MISO_PIN 12

#define FILTERSETTLETIME 5000                                           //  Time (ms) to allow the filters to settle before sending data

const char* ssid = "IOT";
const char* password = "";
const char* mqtt_server = "192.168.168.150";

char jdata[180];
char data[180];
StaticJsonBuffer<200> jsonBuffer;
String jvalue = "value" ; 
const int CT0 = 1; 
const int CT1 = 1; 
const int CT2 = 1;                                                      // Set to 0 to disable 
const int CT3 = 1;
const int CT4 = 1;
const int CT5 = 1;
const int CT6 = 1;
const int CT7 = 1;
float ct_0 = 0;
float ct_1 = 0;
float ct_2 = 0;
float ct_3 = 0;
float ct_4 = 0;
float ct_5 = 0;
float ct_6 = 0;
float ct_7 = 0;
float vrm_0 = 0;

#include "EmonLibmcpS.h"
EnergyMonitor ct0,ct1,ct2,ct3,ct4,ct5,ct6,ct7;                                              // Create  instances for each CT channel

// Note: Please update emonhub configuration guide on OEM wide packet structure change:
// https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md
typedef struct { int power0, power1, power2, power3, power4, power5, power6, power7, Vrms;} PayloadTX;      // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;                                                       

boolean settled = false;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup() 
{

pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
    setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
{
  Serial.println("emonTX Shield CT123 Voltage example"); 
  Serial.println("OpenEnergyMonitor.org");
}
   if (CT0) ct0.current(1, 66.606);  
  if (CT1) ct1.current(1, 66.606);                                     // Setup emonTX CT channel (ADC input, calibration)
  if (CT2) ct2.current(2, 60.606);                                     // Calibration factor = CT ratio / burden resistance
  if (CT3) ct3.current(3, 60.606);                                     // emonTx Shield Calibration factor = (100A / 0.05A) / 33 Ohms
  if (CT4) ct4.current(4, 60.606); 
  if (CT5) ct5.current(5, 66.606);                                     // Setup emonTX CT channel (ADC input, calibration)
  if (CT6) ct6.current(6, 60.606);                                     // Calibration factor = CT ratio / burden resistance
  if (CT7) ct7.current(7, 60.606);                                     // emonTx Shield Calibration factor = (100A / 0.05A) / 33 Ohms
 
  if (CT0) ct0.voltage(0, 149, 1.7);
  if (CT1) ct1.voltage(0, 149, 1.7);                                // ct.voltageTX(ADC input, calibration, phase_shift) - make sure to select correct calibration for AC-AC adapter  http://openenergymonitor.org/emon/modules/emontx/firmware/calibration. Default set for Ideal Power adapter                                         
  if (CT2) ct2.voltage(0, 149, 1.7);                                // 268.97 for the UK adapter, 260 for the Euro and 130 for the US.
  if (CT3) ct3.voltage(0, 149, 1.7);
  if (CT4) ct4.voltage(0, 149, 1.7);
  if (CT5) ct5.voltage(0, 149, 1.7);                                // ct.voltageTX(ADC input, calibration, phase_shift) - make sure to select correct calibration for AC-AC adapter  http://openenergymonitor.org/emon/modules/emontx/firmware/calibration. Default set for Ideal Power adapter                                         
  if (CT6) ct6.voltage(0, 149, 1.7);                                // 268.97 for the UK adapter, 260 for the Euro and 130 for the US.
  if (CT7) ct7.voltage(0, 149, 1.7);
  

                                              
//  digitalWrite(LEDpin, HIGH);    // Setup indicator LED
  
                                                                                     
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() 
{ 

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("emontx", data);
  }
   if (CT0) {
    ct0.calcVID(20,2000,1.51);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power1 = ct0.realPower;
    Serial.print(" ct0 ");Serial.println(emontx.power0);  
    ct_0=(emontx.power0); 
                                          
  }
  
  if (CT1) {
    ct1.calcVID(20,2000, 1.51);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power1 = ct1.realPower;
    Serial.print(" ct1 ");Serial.println(emontx.power1);  
    ct_1=(emontx.power1); 
                                          
  }
  
  emontx.Vrms = ct1.Vrms; // AC Mains rms voltage 
  vrm_0=(emontx.Vrms);
  
  if (CT2) {
    ct2.calcVID(20,2000, 1.51);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power2 = ct2.realPower;
   ct_2=(emontx.power2);
    Serial.print(" ct2 "); Serial.println(emontx.power2);
  } 

  if (CT3) {
    ct3.calcVID(20,2000, 1.51);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power3 = ct3.realPower;
    ct_3=(emontx.power3);
    Serial.print(" ct3 "); Serial.println(emontx.power3);
  } 
  
   if (CT4) {
     ct4.calcVID(20,2000,1.51);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power4 = ct4.realPower;
    ct_4=(emontx.power4);
    Serial.print(" ct4 "); Serial.println(emontx.power4);
  } 
 
  if (CT5) {
    ct5.calcVID(20,2000,1.51);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power5 = ct5.realPower;
    ct_5=(emontx.power5);
   Serial.print(" ct5 "); Serial.println(emontx.power5);                                         
  }
  
   
  if (CT6) {
    ct6.calcVID(20,2000,1.51);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power6 = ct6.realPower;
  ct_6=(emontx.power6);
    Serial.print(" ct6 "); Serial.println(emontx.power6);
  } 

  if (CT7) {
    ct7.calcVID(20,2000,1.51);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power7 = ct7.realPower;
    ct_7=(emontx.power7);
    Serial.print(" ct7 "); Serial.println(emontx.power7);
    
  } 
  
  // because millis() returns to zero after 50 days ! 
  if (!settled && millis() > FILTERSETTLETIME) settled = true;

  if (settled)                                                            // send data only after filters have settled
  { 
String value1;
String value2;
String value3;
String value4;
String value5;
String value6;
String value7;

String value = "\"volts\": " + String(vrm_0) ;
if (CT1) value1 = ",\"ct1\": " + String(ct_1) ;
if (CT2) value2 = ",\"ct2\": " + String(ct_2) ;
if (CT3) value3 = ",\"ct3\": " + String(ct_3) ;
if (CT4) value4 = ",\"ct4\": " + String(ct_4) ;
if (CT5) value5 = ",\"ct5\": " + String(ct_5) ;
if (CT6) value6 = ",\"ct6\": " + String(ct_6) ;
if (CT7) value7 = ",\"ct7\": " + String(ct_7) ;
   value = value + value1 + value2 + value3 + value4 + value5 + value6 + value7 ;
  String payload = "{" + value + "}";
   payload.toCharArray(data, (payload.length() + 1));
      
  
  Serial.print("Publish message: ");
    Serial.println(data);    
    delay(50);
 //   Serial.println(PayloadTX);
 //   send_rf_data();                                                       // *SEND RF DATA* - see emontx_lib
  // digitalWrite(LEDpin, LOW); delay(500); digitalWrite(LEDpin, HIGH);      // flash LED
    //delay(500);                                                          // delay between readings in ms
  }
}
