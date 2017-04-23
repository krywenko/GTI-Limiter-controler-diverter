# GTI-Limiter-controler-diverter

 and arduino sketch. 
hi there for anyone who might want- update to my GTI limiter/ controller and diverted.. i combined my multiple units in one unit. it works with the original emonlib or my custom emonlibs using MCP chips to add multiple analogs to esp and arduino unos (
basically it will control 17 PWM in is zero crossing and/or phase angle and 6 pot control devices
operational option.. it probably still has a few bugs as i have not tested every option combination possible so something probably will arise ( and I have not tested phase angle intensively as I prefer zero crossing)
Diversion

0-23 SSR in combination of zero crossing or phase angle SSR ( use a triac and moc300X or random SSR for phase angle ) 17 controlled by PWM and 6 by variable resistance (POT)
cascading and/or in unison
GTI Limiting

also 0 -22 using combination PWM or variable resistance - one just add DC to DC SSR between the panel and the inverter - depending on inverter might need to add low pass filter or just a capacitor across the DC switch to flatten out the voltage pulse also works either in Unison ( all inverter reduce out put at the same level )or cascading ( reducing GTI output one GTI at a time )
one does not have to use it as GTI limiting you can also use it to transfer DC power to do other work instead ie powering DC motors for ventilation or water pumps, charging battery storage systems etc
GTI Controller

controls one GTI via PWM
in period of low alternative energy production you can draw power from a battery storage- you can use pretty much any cheap GTI for this to protect the GTI you adjust the maxium power output through the PWM . so the MPPT does not try to track at the highest possible amperage and voltage and is not running a 100% load all the time shortens the life dramatically. just adjust setting so your GTI runs at <80% of rated maximum output and it should work fine over a long period with out issue . to control and maintain batteries just place a cheap solar controller rated at the same or larger amperage and voltage then your GTI using the "light" control battery disconnect on your GTI due to battery discharge.

it is designed to work with emontx sheild, an arduino plus pca9685 PWM shield
but it should also work with my espemontx adaptor shield and library and wemos D1 or arduino. it handle 16 SSR (technically +900 SSR if stacking pca9685 PWM shields) either in level or cascading bucket mode . adjustable PWM frequency- ie if using zero cross or leading edge triac based SSR then lower 128 - 255 frequency is use . if using an analog based SSR 128 - 4096 frequency can be used to divert or GTI limiting . plus the sketch still includes controls of digital pot to handle PWM using pot design at 255 steps.
it still has an intelligent bubble searches for very fast grid zeroing and with the cascade mode can be extremely accurate. smaller diverter load ie 16 - 500 watt loads at 255 steps on a zero crossing SSR has an accuracy of ~2 watts 8 kw diversion load though the arduino a bit slow for that but with wemos more the fast enough for nearly instant zeroing ( the arduino will take a 1/2 moment)

library required
emonlib if using only an arduino
and
PCA9685-Arduino-master library found on github

emonlibMCPScalable if using esp/arduino mcp3008 emontx

MCP3008scalable 
if using emonMCP/mcp3008 for the esp/arduino you need to modify the sketch slightly
