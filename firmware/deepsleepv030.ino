/*
v0.3.0, made by Rolf Hut.
low power GPS logger with GSM upload and local storage.

Based on the Particle Electron Asset Tracker V1 combined with the Sparkfun OpenLog 
SD logger.
- Particle Asset Tracker: https://store.particle.io/products/asset-tracker
- Sparkfun OpenLog: https://www.sparkfun.com/products/13712


FUNCTIONS

- logs GPS every 60 seconds onto OpenLog SD logger
- calls into GSM network every 15 minutes to upload last 15 GPS locations to 
  Particle Cloud
- goes into deep sleep in between GPS measurements.
- works for two days on 2000 mAh battery, given good GSM coverage.
- option to use a third party GSM provider, not the Particle provided sim card. 
  See https://community.particle.io/t/electron-3rd-party-sim-tips/26490
  for details.
- shuts down for half an hour when battery levels drop below 5%. Boots up again 
  when battery above 10%
- optional debugging information is provided over serial when DEBUGSERIAL is 
  defined as TRUE.

WIRING
- To make sure the GPS is always on: connect a jumper wire from D6 to GND.
- Connect the Openlog SD logger using a male to female jumper cable. Connect as 
  follows
Openlog   ->    Particle Electron
   +                3V3
   -                GND
   TX               C2 (RX Electron side)
   RX               C3 (TX Electron side)

WARNING: as of februari 2017, it came to my attention that the Particle electron 
suffers from a brownout error when it tries to start from a power source that 
cannot sustain it. This brownout error completly bricks the device, a complete 
reinstall of the firmware using a JTAG programmer is needed to fix this.

This can be escpially problematic when a relatively small solar panel is 
connected but no battery. The electron will try to start up using the energy 
received from the panel, but than brownout. When using the Electron together 
with a solar panel, make sure the solar panel can be easily disconnected and 
always disconnect it before disconnecting the battery. Make sure to monitor the 
battery level (remotely, this code publishes battery levels to the Particle 
Cloud) and replace the battery if it takes too long for the device to wake back 
up after going into prolonged sleep mode due to low battery.

License: GNU FOOBAR link.
*/


/*SEMI AUTOMATIC MODE give control over when the electron connects to the
  Particle Cloud.*/
SYSTEM_MODE(SEMI_AUTOMATIC);

/*SYSTEM THREAD makes sure that the network activity runs in the a parallel
  process (in a different thread), so that it can be monitored from this code
  and can be stopped when it takes too long to connect to the cloud.*/
SYSTEM_THREAD(ENABLED);

/*If the sim provided by Particle is not used, because there is no service in 
  your country, you need a third party sim. The two lines below are the settings 
  for the two major providers in Myanmar, where our project run. Uncomment the 
  line for the provided that you use, or add a line for your own provider if 
  needed. For more information, see: 
  https://community.particle.io/t/electron-3rd-party-sim-tips/26490 */
  
//STARTUP(cellular_credentials_set("mptnet", "mptnet", "mptnet", NULL));    //if MPT sim card
//STARTUP(cellular_credentials_set("internet", "", "", NULL));                // if telenor simcard



/*For analyzing the GPS signals, the TinyGPS is used, make sure that the files 
  for this library are in your directory. This is easily done in the Particle 
  Build environment by "adding" the AssetTrackerRK library. More infor on the 
  library on https://github.com/mikalhart/TinyGPSPlus*/
#include <AssetTrackerRK.h>

/*add the serial4 library to be able to use pins C3(TX) and C2(RX) to 
  communicate with the OpenLog SD logger. */ 
#include "Serial4/Serial4.h"

//Change FALSE to TRUE to receive debugging messages on the standard serial.
#define DEBUGSERIAL FALSE


/****************
 * Object declerations
 * **************/
 
//create an object from the TinyGPSPlus class.
TinyGPSPlus gps;

// Forward function declaration (see end of code)
void publish2Cloud(char buf[]);

//constants
const int sleepTime = 10; //in seconds between GPS measurements.
const int nrOfSamplesBeforeUpload = 6; // nr of sleepTime intervals before uploading to Cloud
const int nrOfSamplesOnFailedUpload = 5; // nr of sleepTime intervals before uploading to Cloud when the previous upload attempt failed.
const int connectCloudTime = 60000; //in milliseconds, time allowed to connect to Cloud.
const int serialTime = 2000; //in milliseconds, time after startup to gather GPS signals
const float lowBatteryThreshold = 5.0; //if the battery drops below this level, the electron goes into prolonged deepsleep
const float okBatteryThreshold = 10.0; //if the electron is in prolonged deepsleep it will wake up again when the battery is above this level.
const int lowBatterySleepTime = 1800; //in seconds: nr of seconds the device should go to deepsleep in case of low battery.

/*retained variabels are retained in SRAM during deepsleep. So they are only 
  reset when the electron is completly removed from power. GPSSampleCounter counts
  how often a sample was recorded and compares to nrOfSamplesBeforeUpload. The 4 
  arrays are to store the measurements. The boolean is a flag for the status of 
  the battery. Note that if nrOfSamplesBEforeUpload is  too large, the SRAM 
  available (4kB) may not be enough to store the retained variables. */
retained int GPSSampleCounter=0;
retained float latBuffer[nrOfSamplesBeforeUpload];
retained float lngBuffer[nrOfSamplesBeforeUpload];
retained int timeBuffer[nrOfSamplesBeforeUpload];
retained int dayBuffer[nrOfSamplesBeforeUpload];
retained bool lowBatWarning = FALSE;

//global variables
long startSerial; //used to store time of startup, to compare to serialTime.



/****************
 * main Code
 * 
 * normally, when battery levels are sufficient, the setup is called once, than the loop is called
 * In the loop, a second is waited for the GPS unit to send a signal to the Electron. Once
 * this signal is received, the "writeInfo" function is called. In this function, the measurement 
 * is added to the SD card, to the retained buffer and if this is the 15th measurement, cellular
 * communication is started to transmit all these messages. The writeInfo always calls the deepsleep 
 * command, to force the unit to go to sleep and start with the setup again once it wakes up.
 * **************/

void setup() {
  //only initiate everything when not in low power mode.
  if (!lowBatWarning){
    /* The GPS module on the AssetTracker is connected to Serial1 and D6. 
    However, D6 is connected to ground (see wiring, so the GPS is always on. */
    Serial1.begin(9600);
    //record the time the Serial1 port was started
    startSerial = millis();

    //start communication with OpenLog
    Serial4.begin(9600);

    //use the PMIC library to set the Charging current tot he maximum.
    PMIC pmic; //Initalize the PMIC class so you can call the Power Management functions below. 
    pmic.setChargeCurrent(0,0,1,0,0,0); //Set charging current to 1024mA (512 + 512 offset)
    pmic.setInputVoltageLimit(4840);   //Set the lowest input voltage to 4.84 volts. This keeps our 6v solar panel from operating below 4.84 volts. Hopefully this limits the nr of brownouts
    
    }

  //if debugging, start Serial communication
  if (DEBUGSERIAL) Serial.begin(9600);

}

void loop(){

  /*check if we are in low power warning. If so: check if battery full enough
    again. */
  if (lowBatWarning){
    //initiate the Fuelgauge and wait 250 ms for it to stabalize
    FuelGauge().quickStart();
    delay(250);
    //if battery is back in ok range: turn lowBatWarning off and reset.
    if (FuelGauge().getSoC() > okBatteryThreshold){
      lowBatWarning = FALSE;
      System.reset();
    }
    else { //otherwise: go back to deepsleep.
      System.sleep(SLEEP_MODE_DEEP, lowBatterySleepTime);
    }
  }
  else {
    /*if not in low power warning, keep processing input from the GPS (Serial1
      untill serialTime milliseconds have passed (and the gps object has 
      processed at least one measurement from the GPS unit. */
    while (Serial1.available() > 0) {
      if (gps.encode(Serial1.read())) {
	    if (millis() - startSerial >= serialTime) {
	        /*The writeInfo writes the location to the SD, checks of it needs to 
	        start cellular, does so if needed, and than goes into deepsleep. It
	        does not return from the function. */
	        writeInfo();
	    }
      }
    }
  }
}


/****************
 * additional functions called in main code
 * **************/

void writeInfo()
{
  /*get the measurements from the GPS and store in the retained arrays. Note 
    that GPSSampleCounter%nrOfSamplesBeforeUpload in the index of the array 
    means only the latest nrOfSamplesBeforeUpload samples are stored. In case
    when there is no cell signal, old measurements will not be uploaded. 
    However: all measurements are written to the SD-logger.
    
    Only if the time and location from the gps and are valid will they be 
    stored. If not valid a value of "-1" will be stored. Note that this is a 
    valid point on the earth, but in the middle of the Atlantic ocean.*/
    if (gps.time.isValid()) {
        dayBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload]=gps.date.value();
        timeBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload]=gps.time.value()/100;
    }
    else {
        dayBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload]=-1;
        timeBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload]=-1;
    }

    if (gps.location.isValid()) {
        latBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload] = gps.location.lat();
        lngBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload] = gps.location.lng();
    }
    else {
        latBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload] = -1.0;
        lngBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload] = -1.0;
    }

    
    //if debugging, print the measurement to serial
    if (DEBUGSERIAL) Serial.print(dayBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload]);
    if (DEBUGSERIAL) Serial.print(",");
    if (DEBUGSERIAL) Serial.print(timeBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload]);
    if (DEBUGSERIAL) Serial.print(",");
    if (DEBUGSERIAL) Serial.print(String(latBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload],8));
    if (DEBUGSERIAL) Serial.print(",");
    if (DEBUGSERIAL) Serial.println(String(lngBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload],8));
    
    /*print the measurement to the SD card (OpenLog). The timeStamp will be written 
      as ddmmyyHHMMSS. */
    Serial4.print(dayBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload],6);
    Serial4.print(timeBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload],6);
    Serial4.print(",");
    Serial4.print(String(latBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload] ,8));
    Serial4.print(",");
    Serial4.println(String(lngBuffer[GPSSampleCounter%nrOfSamplesBeforeUpload],8));
    Serial4.flush();
    Serial4.end();

    //increase the nr of samples since last cellular upload
    GPSSampleCounter=GPSSampleCounter+1;
    
    /*if enought times have passed, call publish2Cloud() which connects online
      and publish the last known locations. */
    if ((GPSSampleCounter > nrOfSamplesBeforeUpload) &
	(((GPSSampleCounter-nrOfSamplesBeforeUpload)%nrOfSamplesOnFailedUpload)==1) ){
      publish2Cloud();
    }
    
    //allow time to shut down the Serial communication to the OpenLog
    System.sleep(SLEEP_MODE_DEEP, sleepTime);
}	    

void publish2Cloud(){
  /*This function turns on the cellular, waits for connection and sends out a 
  burst of messages to completly send all measurements stored in the retained 
  memory. */
    
    //turn GPS serial off, to not interrupt cellular communication.
    Serial1.end();
	    
    //Turn the cellular on and try to connect to the Particle cloud. Since we
    // enabled system threads, these functions will not block the code. The wait
    // statement below will check if connection has been established.
    Cellular.on();
    Particle.connect();

    /*turn on fuelGauge on to measure battery level. Normally we have to wait 
      250 ms before we can read the battery level, but the wait statement below
      takes more than 250 ms anyway.*/       
    FuelGauge().quickStart();
    
    //wait for connection to be esthablised. timeout after connectCloudTime
    waitFor(Particle.connected, connectCloudTime);
    
    //only proceed with publihsing if connected to the Particle Cloud.
    if (Particle.connected()){

        /*we add multiple messages in one string to limit the amount of calls to
    	the Particle Cloud. A message has a maximum length of 255 characters.
    	
    	Since we can only upload 4 messages per second, we collect them, process
    	them in one burst (by calling Particle.process() ) and than wait for a 
    	second. */
            
        int pubCounter = 0;
        String pubString = "";
    
        for (int i=0;i<nrOfSamplesBeforeUpload;i++){
    	    //create the String to be send for a single measurement. This needs zero-padding
    	    String dayString = ("00" + (String) dayBuffer[i]);
    	    int len = dayString.length();
    	    dayString = dayString.substring(len-6);
    	    String timeString = ("00" + (String) timeBuffer[i]);
    	    len = timeString.length();
    	    timeString = timeString.substring(len-6);
    	
    	    String addString = dayString + timeString  + "," + 
    	    String(latBuffer[i],8) + "," + 
    	    String(lngBuffer[i],8) + ",";
    	    
    	    /*check if addString still fits in pubString. If yes, add them 
    	    together. If not, publish, then empty pubString and add addString.*/
    	    if ((pubString.length() + addString.length()) > 255){
    	        Particle.publish("location",pubString);
    	        pubString = addString;
    	        pubCounter = pubCounter + 1;
    	    } else {
    	        pubString.concat(addString);
    	    }
    
    	    /*if 4 messages are qeued, send them to the cloud. Wait one second 
    	    because there is a rate limit of 4 messages per second to the cloud. */
    	    if (pubCounter == 4){
    	        Particle.process();
    	        delay(1000);
    	        pubCounter = 0;
    	    }
        }
    
        //sent out the remaining message per definition less than 4.
        Particle.publish("location",pubString);
    
        //publish the battery level to the cloud for online monitoring. 
        Particle.publish("batLvL",(String) FuelGauge().getSoC());
        Particle.process();
    
        //wait a second to allow the data to be sent.
        delay(1000);
    
        //reset counter because we have succesfully sent data to the cloud.
        GPSSampleCounter=0;
    }
    
    else{ //we did not connect to the cloud! 
        if (DEBUGSERIAL) Serial.println("did not connect to Particle Cloud.");
    }

    //check if battery is less than threshold, if so, go into lowpower mode
    if (FuelGauge().getSoC() < lowBatteryThreshold){
        lowBatWarning = TRUE;
    }

    //turn the cellular unit off. 
    Cellular.off();
    /*wait ons second to allow proper shutdown of the cellular unit before going
      to deepsleep when returning from this function. */
    delay(1000);
}