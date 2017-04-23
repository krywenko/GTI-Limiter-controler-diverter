

#define FILTERSETTLETIME 5000 //  Time (ms) to allow the filters to settle before sending data

const int slaveSelectPin = 10;
const int CT1 = 0;                                                      //  element sensor - Set to 0 to disable -- placement CT 1
const int CT2 = 1;                                                      // Inverter sensor - Set to 0 to disable 
const int CT3 = 1;                                                      //grid sensor 
const int CT4 = 1;                                                      // windgen  sensor - Set to 0 to disable  -- placement CT 1
// bubble search1
float grid = 0;                                                          //grid   usage
float stepa = 0;   // 
float stepb = 1;
float stepc = 1;
float prestep =0; //1
int stepbu;
int stat ;
//bubble search2
float stepa3 = 0;   // 
float stepb3 = 1;
float stepc3 = 1;
float prestep3 =0;
int stepbu3;
int stat3 ;
//bubble search3
float stepa4 = 0;   // 
float stepb4 = 1;
float stepc4 = 1;
float prestep4 =0; //1
int stepbu4;
int stat4 ;
//bubble seacch 4
float step1 = 0;   // 
float step2 = 1;
float step3 = 1;
float prestep1 =0;
int stat2 ;
int stepbv;
// non adjustable for the most part
float curinvt = 1; //percentage of power usage comparison over or below grid usage 
float curelem =1;
float curelem3 =1;
float curelem4 =1;

int curgrid = 0;                                                      // current  triac step
int curgrid2 = 0;  //current pot step
int curgrid3 = 0; 
int curgrid4 = 0; 
// adjustable under certian conditions
float kw = 0;
float per = .80;  // max % GTI out on battery 
float inverter =800; // gti size on battery
float invert =1000;
float wind = 1000;

float element = 1000; //wattage of  element use incase no  GTI outside reading - can be  benificial to use  slightly larger- it allow bubble search to  progess forward  into the correct stetting
float delement =1000;  //default element size  in case sensor not working at start up 
int start = 0;
int pulse = 5;   // pwm for SSR
int invstatus = 3;  // led display  showing overproduction 

int level=127;    // main pot  control for PWM
int rlevel=127;    // if you need reverse pot  direction of PWM
// adjustable setting depending on your configuration 
int ios = 8;   //number of IOs for diversion 1-16
int type = 0; // 0= casdading -  1 = equal for diverting 
int io;
int GTI= 1; // 0= disable  1 = enable  if you wish to enable GTI limiting  on remaing ios
int GTItype = 0;  //0 = casdading  1 = equal
int GTIios = 8 ;  // Number of IOs for GTI limiting 1-16 ( proportional to diversion)
int GTIio ; 
int GRID = 0; // 0= disable  1 = enable  if you wish to enable GTI battery   reduce io count by one on other ios last pin
int POT = 0; // enable POT control
int POTtype =1; // 0=uses grid values  1= inverter values 
int ssr=0; // 0= zerocrossing 1 = phase angle 
int sV;
int full; // once Diverter reach limit GTI limiting comes into effect

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
 
//--------------------setup of emonlib -------------

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
//-------------------------------seting pwm  --------
  
  if (type == 0){
   io= (ios*256);  // PWM freq 128 256 1024 .. 4096
  // element= (element*ios);
  }
  else{
  io = 255;
} 

  if (GTItype == 0){
  //  GTIios = (16 - ios);
    GTIio = (GTIios*256); // PWM freq 128 256 1024 .. 4096
  }
  else{
    GTIio=255;
}
//----------------------------------PCA9685 ---------------------

 Wire.begin();                       // Wire must be started first
    Wire.setClock(400000);              // Supported baud rates are 100kHz, 400kHz, and 1000kHz

    pwmController.resetDevices();       // Software resets all PCA9685 devices on Wire line

    pwmController.init(B000000);        // Address pins A5-A0 set to B000000
    pwmController.setPWMFrequency(110); // Default is 200Hz, supports 24Hz to 1526Hz --if 60hz set pulse freq too 110 - 112  for cleanest fire and miminal flicker
                                        // if firing  a phase angle set freq to 120hz  for 60 hz and 100 hz for 50hz 
    //pwmController.setChannelPWM(0, 255 << 4);// Set PWM to 128/255, but in 4096 land

  //  Serial.println(pwmController.getChannelPWM(0)); // Should output 2048, which is 128 << 4
  
  for(int i=0;i < ios; i++){
     pwmController.setChannelPWM(i, 0 << 4);  // set up start values for Diverter --off
                                                                                
}
  
    for(int i=ios;i < (ios+GTIios); i++){       // setup start up value for GTI -- on
     pwmController.setChannelPWM(i, 255 << 4);
                                                                                
}
}

void loop() 
{   //------------------ emonlib  routine -----------
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
//---------------------------start bubble search for GTI output contol------
if (grid != 0 ) {  
if (invert >=0) {
  invert= (invert/(GTIios/((0-stepbv)+1)));// divides the inverter output based on the remaing GTI
  step1 = ( grid / (invert));  //  and then bubble searches to zero
//Serial.println(step1);
  prestep1 = (step2);        //stepping  of previous values 

step2 = (prestep1 + step1);  // just  to stop the positive values creation
  if (step2 > 0) {
    step2 =0;
  }
   if (step2 < (0-GTIios)) {    // stops the creation of values passed the  the usable ios
    step2 = (0-GTIios);
  }
  curinvt = (0  + step2);  // step increase
  stepbv=curinvt;          // strips the decimal to determine the step
  curinvt = (curinvt - stepbv);  // stips int to leave decimal point 
  curgrid2 = (256 * curinvt  ); // calculates what step
 
  curgrid2 =(0-curgrid2);  // value  inversion from neg to positive 
  

  
}
}

//--------------------------end of buuble seach for digitalpot control----
//--------------------------start of bubble search for element based control and GRID battery------
// was a little lazy here as i am no using auto element sizing  if you wish to use element sizing un comment the lines 
if(GRID ==1){           //GRID
  if (grid!=0){      //GRID

//if (CT1){           // element --uncomment this and  comment GRID options for element control 
//if (grid !=0) {    //element
 
  
  //curgrid = 0;
  stepc3 = ( (0-grid)/inverter);  //GRID
//  stepc3 = (grid / element);   // element --for auto element sizing and calculationg probable step 
  
  prestep3 = (stepb3);          // then bubble searches to zero

stepb3 = (prestep3 + stepc3);
  if (stepb3 > 0) {
    stepb3 =0;
  }
   if (stepb3 < (0-1)) { 
    stepb3 = (0-1);      
  }
  curelem3 = (0  + stepb3);
  
stepbu3=curelem3; 

curelem3 = (curelem3 - stepbu3);

  curgrid3 = ( 256 * curelem3  );
  curgrid3 =(0-curgrid3);  
  if (stepbu3 < 0) {curgrid3= 256;}

}
}
//----------------------end of bubble search------
//----------------------start of bubble seach for wind
if (CT4){
if (grid !=0) {
 
  
  //curgrid = 0;
  stepc = (grid / element); // uses a fixed element size  to calculate step

  prestep = (stepb);        // then bubble searches to zero

stepb = (prestep + stepc);
  if (stepb > 0) {
    stepb =0;
  }
  if (stepb < (0-ios)) {
    stepb = (0-ios);
  }
  curelem = (0  + stepb);
stepbu=curelem;
curelem = (curelem - stepbu);
  curgrid = ( 256 * curelem  );
  curgrid =(0-curgrid);  //inverts the value of curgrid if need be 

}
}
//-------------------end of bubble search for wind--------
// bubble search for single SSR control
if (grid !=0) {
 
  
  //curgrid = 0;
  stepc4 = (grid / element); 

  prestep4 = (stepb4);

stepb4 = (prestep4 + stepc4);
  if (stepb4 > 1) {
    stepb4 =1;
  }
  if (stepb4 < 0) {
    stepb4 = 0;
  }
  curelem4 = (0  + stepb4);


  curgrid4 = ( 255 * curelem4  );
  curgrid4 =(255-curgrid4);  //inverts the value of curgrid if need be 
 //   Serial.print(" curgrida ");
  //Serial.println(curgrid);
}
//-------------------LCD screen -------------------------
kw =  (grid / 1000) ;


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
  
//-----------------------------diverter-----------------

int statc ;
int ivar;
int statb ;

stat = (0-stepbu);
if (curgrid==256){curgrid=0;}



analogWrite(pulse,curgrid4); // single pulse signal for SSR off arduino board 


 
if (stat > (ios-1)) {stat=(ios-1);curgrid=255;full=1;}
if (stat ==0) {ivar = 1;}
else  {ivar = 0;}
  if ( type == 0){
   // Serial.println(" solar Diversion - Cascading");
if (stat != statb) {
  statc=(stat+1);
  statb=stat;
  for(int i=ivar;i < stat; i++){
   
     pwmController.setChannelPWM(i, 255 << 4);
  //    Serial.print(i);
   //     Serial.println(" 255");
   
  }
   
  for(int i=statc;i <ios; i++){
     pwmController.setChannelPWM(i, 0 << 4);
     
   //     Serial.print(i);
   //     Serial.println(" O");
     }

     }
      if (ssr==1){
      boolean st=false;                                  //an indicator to exit the while loop

  unsigned long start = millis();    //millis()-start makes sure it doesnt get stuck in the loop if there is an error.

  while(st==false)                                   //the while loop...
  {
    sV = analogRead(0);                    //using the voltage waveform
    if ((sV < (1024*0.55)) && (sV > (1024*0.45))) st=true;  //check its within range
    if ((millis()-start)>2000) st = true;
  }
  sV=map(curgrid,0,255,10000,0);  //delay before pulse  
     delayMicroseconds(sV); 
     
    }
     pwmController.setChannelPWM(stat, curgrid << 4);
  }

  
  
  //  Serial.print(stat);
  //  Serial.print(" ");
   // Serial.println(curgrid);
    
  if (type == 1){
   // Serial.println(" solar Diversion -  In Unison");
    for(int i=0;i < ios; i++){
     pwmController.setChannelPWM(i, curgrid << 4);
     
  //  Serial.print(i);
   // Serial.print(" ");
   // Serial.println(curgrid);
  } 
  }


analogWrite(invstatus,curgrid) ;  // led display  showing overproduction 

//--------------------------------GTIlimiter---------------
curgrid2= (256-curgrid2); // to inverts the value
if (curgrid2==256){curgrid2=255;}
if (full == 1){
stat2 = (0-stepbv);
if (GTI == 1) {

int statb2;
int statc2;

if (stat2 > (GTIios-1)) {stat2=(GTIios-1); curgrid2=0;}


if (GTItype == 0){
if(grid != 0){
// Serial.println(" GTI limiting  - Cascading");
  if ( GTItype == 0){
if (stat2 != statb2) {
  statc2=(stat2+1);
  statb2=stat2;
  for(int i=ios ;i < (ios+stat2); i++){
     pwmController.setChannelPWM(i, 0 << 4);
     
    //  Serial.print(i);
     // Serial.println(" 0");
 
  }
  
  for(int i=(GTIios+statc2);i < (GTIios+ios); i++){
    
     pwmController.setChannelPWM(i, 255 << 4);
     
   // Serial.print(i);
    //Serial.println(" 255");
  }

} 
 
  pwmController.setChannelPWM((stat2+ios), curgrid2 << 4);
  
   /// Serial.print(stat2+ios);
    //Serial.print(" - ");
    //Serial.println(curgrid2);
    
  }
}
  
  if (GTItype == 1){
  //  Serial.println(" GTI Limiting - Unison");
    for(int i=GTIios;i < (ios+GTIios); i++){
     pwmController.setChannelPWM(i, curgrid2 << 4);
     
  //  Serial.print(i);
  //  Serial.print(" ");
  //  Serial.println(curgrid2);
  
  } 
  }
 
   
  }

}

 }

if (GRID == 1){
 // Serial.println(curgrid3);
 // invert=(0-invert);
 //if (curgrid3<0){curgrid3=0;}
  if(grid!=0){ 
  //per=(per/100);
  
 // Serial.println(" Battery to GTI ");
  //if (curgrid3 > per){curgrid3 = per;}
  curgrid3=(curgrid3*per);
  pwmController.setChannelPWM(15, curgrid3 << 4);
 // Serial.print("15");
  //  Serial.print(" ");
//Serial.println(curgrid3);
  //
 }
}
 


if(POT == 1){
  if (POTtype == 1){
 level = curgrid3;
rlevel = (255-curgrid3); 
  }
  else{
     level = curgrid2;
rlevel = (255-curgrid2); 
  }
if(grid != 0){
 
    for (int scan = 0; scan < 3 ; scan++) {
    digitalPotWrite(scan, level);
   // Serial.print(" pot forward ");
    // Serial.println(level);
  }
 }
 
 if(grid!=0){
  
   for (int scan = 3; scan < 6; scan++) {
  
    digitalPotWrite(scan, rlevel);
 // Serial.print("oot reverse ");
   //  Serial.println(rlevel);
  }
 }  
}

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
