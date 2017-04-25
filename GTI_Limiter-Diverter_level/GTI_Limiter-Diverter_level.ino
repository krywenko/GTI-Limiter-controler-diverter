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

#define FILTERSETTLETIME 5000 //  Time (ms) to allow the filters to settle before sending data

const int slaveSelectPin = 10;
const int CT1 = 0;                                                      //  element sensor - Set to 0 to disable 
const int CT2 = 1;                                                      // Inverter sensor - Set to 0 to disable 
const int CT3 = 1;                                                      //grid sensor 
const int CT4 = 1;                                                      // windgen  sensor - Set to 0 to disable 

float grid = 0;                                                          //grid   usage
float stepa = 0;   // 
float stepb = 1;
float stepc = 1;
float prestep =1;
float step1 = 0;   // 
float step2 = 1;
float step3 = 1;
float prestep1 =1;
float curinvt = 1; //percentage of power uage comparison over or below grid usage 
float curelem =1;
float kw = 0;
int curgrid = 0;                                                      // current  PMW step
int curgrid2 = 0;                                                     //current triac step
float invert =100;
float wind = 100;
float element = 5000; //wattage of  element  use incase the incase no inverter reading ot grid tie outside  inverter reading - -
float delement =1000;  //default element size  in case sensor not working at start up 
int start = 0;
int pulse = 5;
int invstatus = 3;  // led display  showing overproduction 
float per = 0;
int level=127;    // main pot  control for PWM
int rlevel=127;    // if you need reverse pot  direction of PWM


#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include "EmonLib.h"
EnergyMonitor ct1,ct2,ct3;   // Create  instances for each CT channel

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display


// Note: Please update emonhub configuration guide on OEM wide packet structure change:
// https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md
typedef struct { int power1, power2, power3, power4, Vrms;} PayloadTX;      // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;                                                       

const int LEDpin = 9;                                                   // On-board emonTx LED 

boolean settled = false;

void setup() 
{
 lcd.init();                      // initialize the lcd 
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(3,0);
  lcd.print("GTI Limiter");
  lcd.setCursor(2,1);
  lcd.print("Stephen krywenko!");


      pinMode(slaveSelectPin, OUTPUT);
      SPI.begin();
      
  Serial.begin(9600);
   //while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
   
  //Serial.println("emonTX Shield CT123 Voltage example"); 
 // Serial.println("OpenEnergyMonitor.org");


  if (CT1) ct1.current(1, 66.606);                                     // Setup emonTX CT channel (ADC input, calibration)
  if (CT2) ct2.current(2, 60.606);                                     // Calibration factor = CT ratio / burden resistance
  if (CT3) ct3.current(3, 60.606);                                     // emonTx Shield Calibration factor = (100A / 0.05A) / 33 Ohms
  if (CT4) ct1.current(1, 60.606); 
  
  if (CT1) ct1.voltage(0, 146.54, 1.7);                                // ct.voltageTX(ADC input, calibration, phase_shift) - make sure to select correct calibration for AC-AC adapter  http://openenergymonitor.org/emon/modules/emontx/firmware/calibration. Default set for Ideal Power adapter                                         
  if (CT2) ct2.voltage(0, 146.54, 1.7);                                // 268.97 for the UK adapter, 260 for the Euro and 130 for the US.
  if (CT3) ct3.voltage(0, 146.54, 1.7);
  if (CT4) ct1.voltage(0, 146.54, 1.7);
  


  pinMode(LEDpin, OUTPUT);                                              // Setup indicator LED
  digitalWrite(LEDpin, HIGH);
  
                                                                                     
}

void loop() 
{ 
  if (CT1) {
    if ( start ==10 ) {
   analogWrite(pulse, 254);
   delay (5000);
   //Serial.println(element);
    }
    ct1.calcVI(20,2000);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power1 = ct1.realPower;
    if (start ==10){ 
      element =  emontx.power1;
      analogWrite(pulse, 0);
      
      if (element <10) {(element = delement);}
     // Serial.print(element);
      }
    //Serial.print(emontx.power1);                                         
  }
  
  emontx.Vrms = ct1.Vrms*100;                                            // AC Mains rms voltage 
  
  if (CT2) {
   
    ct2.calcVI(20,2000);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power2 = ct2.realPower;
    invert = emontx.power2;
   
    Serial.print("cmd_1("); Serial.print(invert); Serial.println(")");
    //Serial.print(" "); Serial.print(emontx.power2);
  } 

  if (CT3) {
    ct3.calcVI(20,2000);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power3 = ct3.realPower;
    grid = emontx.power3; 
    
     Serial.print ("cmd_2("); Serial.print(grid); Serial.println(")");
    //Serial.print(" "); Serial.print(emontx.power3);
  } 
  
   if (CT4) {
     ct1.calcVI(20,2000);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power1 = ct1.realPower;
    wind = emontx.power1; 
   
  Serial.print ("cmd_3("); Serial.print(wind); Serial.println(")");
    //Serial.print(" "); Serial.print(emontx.power1);
  } 
  if (invert <0){       // for capture ac adaptor errors is it display consant zero on inverter display -- ct or ac adaptor need to be reversed
    invert = 0;
  }
  if (wind <0){       // for capture ac adaptor errors is it display consant zero on inverter display -- ct or ac adaptor need to be reversed
    wind = 0;
  }
  //Serial.print(" "); Serial.print(ct1.Vrms);


 if (start <= 10){
  start = (start +1);
 }

if (grid != 0 ) {  
if (invert >=0) {

  step1 = ( grid / invert);

  prestep1 = (step2);

step2 = (prestep1 + step1);
  if (step2 > 1) {
    step2 =1;
  }
  if (step2 < 0) {
    step2 = 0;
  }
  curinvt = (0  + step2);
  
  curgrid2 = ( 254 * curinvt  );
  curgrid2 =(254-curgrid2);  //inverts the value of curgrid if need be 
  //  Serial.print(" curgrid2 ");
  //Serial.println(curgrid2);

  
}
}
if (CT1){
if (grid !=0) {
 
  
  //curgrid = 0;
  stepc = (grid / element); 

  prestep = (stepb);

stepb = (prestep + stepc);
  if (stepb > 1) {
    stepb =1;
  }
  if (stepb < 0) {
    stepb = 0;
  }
  curelem = (0  + stepb);


  curgrid = ( 254 * curelem  );
  curgrid =(254-curgrid);  //inverts the value of curgrid if need be 
  //  Serial.print(" curgrida ");
  //Serial.println(curgrid);
}
}
if (CT4){
if (grid !=0) {
 
  
  //curgrid = 0;
  stepc = (grid / element); 

  prestep = (stepb);

stepb = (prestep + stepc);
  if (stepb > 1) {
    stepb =1;
  }
  if (stepb < 0) {
    stepb = 0;
  }
  curelem = (0  + stepb);


  curgrid = ( 254 * curelem  );
  curgrid =(254-curgrid);  //inverts the value of curgrid if need be 
 //   Serial.print(" curgrida ");
  //Serial.println(curgrid);
  
  
 
   
  
}
}

kw =  (grid / 1000) ;
per = ( curgrid / 254);
per = (1 - per);
//per = ( 100 * per);
    lcd.backlight();
    lcd.clear();      
    lcd.setCursor(0,0);
    lcd.print("KW ");
    lcd.print(kw);
    lcd.print(" V ");
    lcd.print(ct1.Vrms);
    lcd.setCursor(0,1);
    lcd.print("I ");
    lcd.print(invert);
    //lcd.setCursor(1,7);
    lcd.print("-");
    if (CT1){
    lcd.print(curgrid);   //displays current step of triac and ad5206 chip 
    lcd.print ( "-");
    lcd.print (curgrid2);
    }
    if (CT4){
      lcd.print("W ");   // displays  wind inverter output 
      lcd.print (wind);
    }
analogWrite(pulse, curgrid);

analogWrite(invstatus, curgrid);  // led display  showing overproduction 
//Serial.println ("");
level = curgrid2;
rlevel = (254-curgrid2);

if(grid < 0){
 
    for (int scan = 0; scan < 1 ; scan++) {
    digitalPotWrite(0, level);
        //Serial.print("Hchannel ");
  // Serial.println(level);
 
     // delay(10);
    digitalPotWrite(1, level);
  //      Serial.println("Hchanne2");
   // Serial.println(level);
  //  Serial.println(2);
      //delay(10);
    digitalPotWrite(2, level);
   //     Serial.println("Hchanne3");
   // Serial.println(level);
   // Serial.println(3);
      //delay(10);
    digitalPotWrite(3, level);
    //    Serial.println("Hchanne4");
   // Serial.println(rlevel);
    //Serial.println(4);
      //delay(10);
    digitalPotWrite(4, level);
  //      Serial.println("Hchanne5");
  //  Serial.println(rlevel);
  //  Serial.println(5);
      //delay(10);
    digitalPotWrite(5, level);
   // Serial.print("Hchannel6 ");
    //Serial.println(rlevel);
 
   // Serial.println(Irms);
    //level=level+1;
   
   
      //delay(10);
   
  }
 }
 if(grid>0)
{
  //if(level>=0)
   for (int scan = 0; scan < 1; scan++) {
  {
    digitalPotWrite(0, level);
       // Serial.print("lchannel ");
   // Serial.println(level);
 
     // delay(10);
    digitalPotWrite(1, level);
    //    Serial.println("lchanne2");
   // Serial.println(level);
   // Serial.println(2);
      //delay(10);
    digitalPotWrite(2, level);
  //      Serial.println("lchanne3");
   // Serial.println(level);
   // Serial.println(3);
      //delay(10);
    digitalPotWrite(3, level);
   //     Serial.println("lchanne4");
 //   Serial.println(rlevel);
  //  Serial.println(4);
     // delay(10);
    digitalPotWrite(4, level);
   //     Serial.println("lchanne5");
   // Serial.println(rlevel);
  //  Serial.println(5);
     // delay(10);
    digitalPotWrite(5, level);
   // Serial.print("lchannel6  ");
   // Serial.println(rlevel);
     
    //Serial.println(Irms);
   // level=level-1;
    
  }

  }
 

 }

    // Serial.print("level ");
   // Serial.println(level);
    // Serial.print("rlevel ");
    // Serial.println(rlevel);  
     // Serial.println(" "); 
}


void digitalPotWrite(int address, int value) {
  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPin, LOW);
  //  send in the address and value via SPI:
  SPI.transfer(address);
  SPI.transfer(value);
  // take the SS pin high to de-select the chip:
 digitalWrite(slaveSelectPin, HIGH);
          

  // because millis() returns to zero after 50 days ! 
  if (!settled && millis() > FILTERSETTLETIME) settled = true;

  if (settled)                                                            // send data only after filters have settled
  { 
        
 



   
 //   send_rf_data();                                                       // *SEND RF DATA* - see emontx_lib
 //   digitalWrite(LEDpin, HIGH); delay(2); digitalWrite(LEDpin, LOW);      // flash LED
   // delay(500);                                                          // delay between readings in ms
  }
}
