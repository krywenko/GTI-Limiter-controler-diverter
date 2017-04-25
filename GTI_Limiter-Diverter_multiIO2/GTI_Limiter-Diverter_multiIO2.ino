

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
float curinvt = 1; //percentage of power usage comparison over or below grid usage 
float curelem =1;
float kw = 0;
int curgrid = 0;                                                      // current  triac step
int curgrid2 = 0;                                                     //current pot step
float invert =100;
float wind = 100;
float element = 1000; //wattage of  element use incase no  GTI outside reading - benificial to use larger- it allow bubble search to progess into the correct stetting
float delement =1000;  //default element size  in case sensor not working at start up 
int start = 0;
int pulse = 5;   // pwm for SSR
int invstatus = 3;  // led display  showing overproduction 
float per = 0;
int level=127;    // main pot  control for PWM
int rlevel=127;    // if you need reverse pot  direction of PWM
int ios = 16;   //number of IO
int type = 0; // 0= casdading -  1 = equal
int io;
int stepbu;
int stat=0 ;
int iost;

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include "EmonLib.h"
EnergyMonitor ct1,ct2,ct3;   // Create  instances for each CT channel
//PCF8574 IO;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#include "PCA9685.h"

PCA9685 pwmController;                  // Library using default Wire and default linear phase balancing scheme

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
  if (type == 0){
   io= (ios*256);
   //element= (element*ios);
  }
  else{
  io = 255;
} 
   iost=(ios-1);
 Wire.begin();                       // Wire must be started first
    Wire.setClock(400000);              // Supported baud rates are 100kHz, 400kHz, and 1000kHz

    pwmController.resetDevices();       // Software resets all PCA9685 devices on Wire line

    pwmController.init(B000000);        // Address pins A5-A0 set to B000000
    pwmController.setPWMFrequency(500); // Default is 200Hz, supports 24Hz to 1526Hz

    pwmController.setChannelPWM(0, 255 << 4);// Set PWM to 128/255, but in 4096 land

    Serial.println(pwmController.getChannelPWM(0)); // Should output 2048, which is 128 << 4
                                                                               
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
   
    Serial.print("cmd_1("); Serial.print(invert); Serial.println(")");  // for esp daughter board on an arduino
    //Serial.print(" "); Serial.print(emontx.power2);
  } 

  if (CT3) {
    ct3.calcVI(20,2000);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power3 = ct3.realPower;
    grid = emontx.power3; 
    
     Serial.print ("cmd_2("); Serial.print(grid); Serial.println(")");  // for esp daughter board on an arduino
    //Serial.print(" "); Serial.print(emontx.power3);
  } 
  
   if (CT4) {
     ct1.calcVI(20,2000);                                                  // Calculate all. No.of crossings, time-out 
    emontx.power1 = ct1.realPower;
    wind = emontx.power1; 
   
  Serial.print ("cmd_3("); Serial.print(wind); Serial.println(")");     // for esp daughter board
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
//---------------------------start bubble search digitalpot contol------
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
  
  curgrid2 = ( 255 * curinvt  ); //was254
  curgrid2 =(255-curgrid2);  //inverts the value of curgrid if need be 
  /*
    Serial.print(" curgrid2 ");
  Serial.println(curgrid2);
  */
}
}

//--------------------------end of buuble seach for digitalpot control----
//--------------------------start of bubble search for element based control------
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


  curgrid = ( 255 * curelem  );
  curgrid =(255-curgrid);  //inverts the value of curgrid if need be 
  //  Serial.print(" curgrida ");
  //Serial.println(curgrid);
}
}
//----------------------end of bubble search------
//----------------------start of bubble seach for wind
if (CT4){
if (grid !=0) {
 
  
  //curgrid = 0;
 // Serial.print(" element ");
 //   Serial.println(element);
  stepc = (grid / element); 

  prestep = (stepb);

stepb = (prestep + stepc);

  if (stepb > 0) {
    stepb = 0;
  }
  if (stepb < (0-ios)) {
    stepb = (0-ios);
  }
  curelem = (0  + stepb);
  stepbu=curelem;
 Serial.print(" stepb ");
    Serial.println(stepbu); 
//stepbd = (stepbu-stepb);
curelem = (curelem - stepbu);
Serial.print(" cuelm ");
    Serial.println(curelem);
  curgrid = (256 * curelem  );
  curgrid =(0-curgrid);  //inverts the value of curgrid if need be 
  
  Serial.print(" curgrid ");
  Serial.println(curgrid); 
  Serial.print(" io ");
  Serial.println(io); 
  
}
}
//-------------------end of bubble search for wind--------

kw =  (grid / 1000) ;
per = ( curgrid / io);
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
  

int statc ;

int statb=0 ;
int tcurgrid;
int curgridt;  
tcurgrid =((curgrid/ios)-1); 
curgridt =((curgrid/256));
//if (curgridt > 255); curgridt=255;
if  (tcurgrid <0);tcurgrid=0;
analogWrite(pulse, tcurgrid); // single pulse signal for SSR
stat = (0-stepbu);

if (curgrid==256){curgrid=0;}
if (stat > (ios-1)) {stat=(ios-1);}
    Serial.print(" stat ");
    Serial.println(stat); 
 
  if ( type == 0){
    
if (stat != statb) {
  statc=(stat+1);
  statb=stat;
  for(int i=0;i < stat; i++){
    
     pwmController.setChannelPWM(i, 255 << 4);
     
     Serial.print(" stat1 ");
     Serial.print(i);
     Serial.println(" 255"); 
     
  }


  
  for(int i=statc;i <= (ios-1); i++){
     pwmController.setChannelPWM(i, 0 << 4);
   
     Serial.print(" stat ");
     Serial.print(i);
     Serial.println(" 0 "); 
     
  }

}

 
  }
  //if (stat == 15){curgrid=255;}
 pwmController.setChannelPWM(stat, curgrid << 4);

  Serial.print("stat3 ");
  Serial.print(stat);
  Serial.print(" ");
Serial.println(curgrid); 

  
  if (type == 1){
    for(int i=0;i <= ios; i++){
     pwmController.setChannelPWM(i, curgrid << 4);
   /*  
     Serial.print(" stat static ");
     Serial.print(i);
     Serial.print(" ");
     Serial.println(curgrid); 
   */  
  } 
  }

tcurgrid =((curgrid)/ios);
analogWrite(invstatus,tcurgrid) ;  // led display  showing overproduction 
//Serial.println ("");
level = curgrid2;
rlevel = (255-curgrid2); //was254

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
        
  }
}
