/***************************************************

Rural Remote Monitoring software to transmit sensor data back to
a base station for analysis.
Fragments of code have been borrowed from https://github.com/adafruit/Adafruit_FONA

25-11-2016

v1.0 - May this be the only, bug-free version ever :|

Numaan Chaudhry
numaan@plansterling.co.uk

 ****************************************************/

/* Import underlying functionality from the C/C++ library by Adafruit */
#include "Adafruit_FONA.h"

/* This gets set to a unique ID in the build process and linked to the hardware */
char* MODULE_ID = "sl_001";

/* Software version */
uint8_t VERSION = 1;

/* Configuration options */
char* _smsDestination         = "+44_YOUR_NUMBER_HERE";
char* _dataDestinationUrl     = "__YOUR_URL_HERE.bluemix.net";
uint8_t BATT_CHARGE_THRESHOLD = 80;

/* Custom APN to override default for use by GSM module */
char * _customApnName = "";
char * _customApnUser = "";
char * _customApnPwd  = "";

/* Status Constants */
uint8_t GPRS_CODE_SUCCESS       = 200;
uint8_t GPRS_CODE_ENABLE_FAIL   = 888;
uint8_t GPRS_CODE_POST_FAIL     = 999;
char * ERROR_GPS_ENABLE_FAILED  = "G_F";
char * ERROR_BATT_LEVEL_FAILED  = "B_F";
const uint8_t ERROR_SZ          = 8;


/* Reset is tied to pin 4 */
#define FONA_RST 4

/* Enable this #define to speed-up time (for testing) */
#define RUNTIME_TEST

/* Enable this to 
    speed-up time, 
    write logging to Serial monitor,
    mock receiving data from sensors,
    disable transmitting data (to prevent running out of phone data/SMS'!) */
#define DEBUG

#ifdef DEBUG
  #define RM_LOG2(x,y)  \
      Serial.print (x);\
      Serial.print (":");\
      Serial.println (y);
      
   #define RM_LOG(x) \
      Serial.println(x);
#else
  #define RM_LOG2(x,y)
  #define RM_LOG(x)
#endif

/* Connection between Intel board and SIM808 FONA module */
HardwareSerial *fonaSerial = &Serial1;

/* C++ SIM808 module */
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

/* Replies from fona module are kept here */
char replybuffer[255];

/* Holds GPS data */
struct GpsData{
  boolean success   = false;
  boolean is3DFix   = false;
  uint8_t rawSz     = 0;
  char* raw         = NULL;
  char* gsmLoc      = NULL;
  uint8_t gsmLocSz  = 0;
  uint16_t gsmErrCode;
};

/* Holds data about electrical measurements */
struct SensorData{
  float VBatt   = 0;
  float VPV     = 0;
  float Current = 0;
  float Temp    = 0;

  uint8_t NoOfBattReadings  = 0;
  uint8_t NoOfPVReadings    = 0;
  uint8_t NoOfCurrReadings  = 0;
  uint8_t NoOfTempReadings  = 0;
};


/* Measurements storage for current cycle */
SensorData* currSensorData;
SensorData sensorDataArr[12];  //Store a day's worth of readings, one ~ every other hour
uint8_t currSensorDataIdx=0; //Index of where we are in the above
GpsData gpsData;

/*****************/
/* Timing fields */
/*****************/

/* These are latching fields. Once set, remain set until the end of the current cycle */
boolean _has1MinElapsed;  //Gets set and latches to true after 1 min
boolean _has5MinElapsed;
boolean _has15MinElapsed; //Gets set and latches to true after 15 mins

/* These are true only ONCE in the FIRST loop in 1 particular cycle */
boolean _isAtCycleStart;
boolean _isDailyCycle;
boolean _isWeeklyCycle;
boolean _at1Min;

/* These are set for 1 loop - at regular intervals - in any given cycle */
boolean _is1SecInterval;
boolean _is30SecInterval;

boolean _gpsFetchInProgress;
boolean _chargingInProgress;
boolean _waitForTransmitInProgress;

boolean __is1MinTriggered;
unsigned long __last1SecInterval;
unsigned long __last30SecInterval;
unsigned long previousMillis  = 0;
unsigned long currCycleStart  = 0; //Milliseconds when this cycle started
unsigned long currCycleNo     = 0; //Cycle-Number of current cycle
unsigned long _runtimeTestSpeedUpFactor = 1L * 40L * 60L * 1000L;
unsigned long _delayBetweenCycles = 
#ifdef DEBUG
  100;
#elif defined(RUNTIME_TEST)
  300;
#else
  3 * 1000;
#endif


unsigned long _cycleInterval =
#ifdef DEBUG
  1L * 60L * 1000L; //Every few secs
//#elif defined(RUNTIME_TEST)
  //1L*1*1000;
#else
  1L * 60L * 60 * 1000L; //Every hour-3,600,000
#endif


/* Forward declarations */
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
uint8_t type;

/* One-time Arduino setup */
void setup() {
  
  #ifdef DEBUG
    while (!Serial) { } ;
    Serial.begin(115200);
   #endif

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {

                                                    RM_LOG(F("Failed to Initialise !"));
    return;
  }
  
  type = fona.type();
}

/*
 * See comments for loop() for how this function fits into the bigger picture
 * Returns true if the cycle should continue as some sub-component needs more time
 */
boolean loopCycle() {
  
  boolean doContinueCycle = false;

  //Read sensors every second for 1 minute to obtain a cumulative moving average
  if (!_has1MinElapsed) {

      if (_is1SecInterval) {
        
          readSensorsAsync();
      }

      doContinueCycle |= true;
  }

  //Read the GPS value every week. It's unlikely to change so this is sufficient.
  if (_isWeeklyCycle) {
    
      triggerGpsRefreshAsync();
      
      gpsData = GpsData();
      _gpsFetchInProgress = true;
      
      doContinueCycle |= true;
  }

  //Wait 5 minutes for the GPS fetch to complete
  if (_gpsFetchInProgress) {

      if (_has5MinElapsed) {
  
            gpsData = getGpsSensorData();

            onGpsComplete();
                                                    RM_LOG2(F("GSM RawData"),gpsData.raw); //1 way of keeping log statements out of the way !
            execTransmitGps();
          
            _gpsFetchInProgress = false;
            doContinueCycle |=    false;
      }
      else {
        
          doContinueCycle |= true;
      }
  }

  //Once a day, send sensor data
  if (_isDailyCycle) {
                                                    RM_LOG(F("Transmitting reading..."));
      execTransmitReadings();
  }

  //Make sure the LiPo battery level doesn't drop too much, preventing us sending data
  if (_isAtCycleStart) {
    
      bool needsCharging = ensureBatteryLevel();
      _chargingInProgress = needsCharging;

      doContinueCycle |= needsCharging;
  }

  //Keep checking battery level every 30 seconds until it's high enough we can stop this cycle and shut down module
  if (_chargingInProgress) {

      if (_is30SecInterval) {
        
          bool needsCharging = ensureBatteryLevel();
          bool doneCharging = !needsCharging || _has15MinElapsed;

          _chargingInProgress =   !doneCharging;
          doContinueCycle |=      !doneCharging;
      }
      else {
        
          doContinueCycle |= true;
      }
  }
  
  return doContinueCycle;
}

unsigned long getCycleIntervalInHours() {

  return _cycleInterval / (1000L * 60L * 60L);
}
/* 
 * This is the main loop that runs continuously whilst the module is running.
 * It's responsible for determining when to run and for how long. 
 * It doesn't determine WHAT runs. The loopCycle() function decides what to run.
 * 
 * This loop runs as follows :-
 *    > Determine if we need to run a cycle. A cycle runs every _cycleInterval (e.g. once an hour etc.)
 *    > When a cycle begins,
 *        > This loop begins tracking time and setting various timing-related variables (like _has1MinElapsed)
 *        > It then calls loopCycle() continuously. 
 *        > loopCycle() then runs sensor measurements, algorithms, transmissions etc. The loopCycle() will return true if it needs more time
 *        > When the loopCycle() returns false, this cycle finishes and the loop waits for the _cycleInterval to elapse before doing the same
 * 
 */
void loop() {
  
  unsigned long currentMillis = millis();

#ifdef RUNTIME_TEST
  //Speed up time in DEBUG mode ! // Each second => x minutes
  unsigned long secsFromStart = currentMillis/1000;
  currentMillis = _runtimeTestSpeedUpFactor * secsFromStart;
#endif

  boolean isRunningCycle = currCycleStart != 0;

  //If board being powered up for the first time, start timer before running 1st cycle
  if (previousMillis == 0) {
      previousMillis = currentMillis;
  }

  //Kick off new cycle if interval has elapsed
  boolean intervalElapsed = (unsigned long)(currentMillis - previousMillis) >= _cycleInterval;
  boolean runNewCycle = !isRunningCycle && intervalElapsed;

  boolean doContinue = false;

  if (isRunningCycle || runNewCycle) {

      //Flag new cycle if started
      if (runNewCycle) {
        
                                              RM_LOG2(F("Start of new cycle - idx"),currCycleNo);
        currCycleNo++;
        currCycleStart = currentMillis;
        
        //Set flags
        _isAtCycleStart = true;
        if (currCycleNo % ((1000L * 60L * 60L * 24L) / _delayBetweenCycles) == 0 )
          _isDailyCycle = true;
        if (currCycleNo % ((1000L * 60L * 60L * 24L * 7L) / _delayBetweenCycles) == 0)
          _isWeeklyCycle = true; 

#ifdef DEBUG
//  Simulate a weekly cycle for GPS
//  if (currCycleNo==1)_isWeeklyCycle=true;
#endif

        sensorDataArr[currSensorDataIdx] = SensorData();
        currSensorData = &sensorDataArr[currSensorDataIdx];
        uint8_t arrLength=sizeof(sensorDataArr)/sizeof(SensorData);      
        currSensorDataIdx = (currSensorDataIdx+1) % arrLength;

        //Reset all latching variables
        _has5MinElapsed = false;
        _has1MinElapsed = false;
        _has15MinElapsed = false;
      }
  
      //How long has this cycle been running for ?
      unsigned long currCycleDuration = currentMillis - currCycleStart;
                                               RM_LOG2(F("Current Cycle Duration"), currCycleDuration);
  
      //1 minute
      if (currCycleDuration >= 1L*60*1000) {
          _has1MinElapsed = true;
  
          //One-Time @1-Min triggere
          if (!__is1MinTriggered) {
            
            _at1Min = true;
            __is1MinTriggered = true;
          }
      }
  
      //5 mins
      if (currCycleDuration >= 5L*60*1000)
          _has5MinElapsed=true;
  
      //15 mins
      if (currCycleDuration >= 15L*60*1000)
          _has15MinElapsed=true;
  
      //1 sec interval
      if ( (currentMillis - __last1SecInterval) >= 1L*1000) {
          _is1SecInterval = true;
          __last1SecInterval = currentMillis;
      }
  
      //30 sec interval
      if ( (currentMillis - __last30SecInterval) >= 30L*1000) {
          _is30SecInterval = true;
          __last30SecInterval = currentMillis;
      }    
  
      //Run the cycle !
      doContinue = loopCycle();
  
      //Some don't latch, so reset them
      _isAtCycleStart = false;
      _at1Min = false;
      _is30SecInterval = false;
      _is1SecInterval = false;
      _isDailyCycle = false;
      _isWeeklyCycle = false;
        
        
    //If cycle complete, wait for interval to elapse again
    if (!doContinue) {
                                            RM_LOG(F("END CYCLE"));

      currCycleStart = 0;
      previousMillis = currentMillis;
    }
  }
  

  delay(_delayBetweenCycles);
}

/* Initialises the provided buffer with data to send */
char* initBufferForSend(char* buffer, uint16_t errorCode) {
  
  //Get network diagnostics
  uint8_t networkStatus = fona.getNetworkStatus();
  uint8_t rssi = fona.getRSSI();

  char* curr = buffer;

                                             // RM_LOG2(F("GSM RawData4"),gpsData.raw);
  //Software Version
  sprintf(curr++, "%d", VERSION);

                                              //RM_LOG2(F("GSM RawData5"),gpsData.raw);
  //Module id
  sprintf(curr, "%s", MODULE_ID);
  curr += 6;

                                             // RM_LOG2(F("GSM RawData6"),gpsData.raw);
  //Network status
  sprintf(curr++, "%d", networkStatus);

                                             // RM_LOG2(F("GSM RawData7"),gpsData.raw);
  //Get network signal
  sprintf(curr, "%02d", rssi);
  curr+=2;

                                              //RM_LOG2(F("GSM RawData8"),gpsData.raw);
  //GPRS error code, if any, padded to 3 chars (for html status codes :))
  sprintf(curr, "%03d", errorCode);
  curr+=3;

                                              //RM_LOG2(F("GSM RawData9"),gpsData.raw);
                                              //RM_LOG2(F("BUFFER"),buffer);
  return curr;
}

/* Transmits GPS readings via GPRS or SMS (fallback) */
void execTransmitGps() {

                                              
    char netBuffer[120+12]; //MAX{GPS-Size, Loc-Size} + Header
    char* currPos = initBufferForSend(netBuffer, 0);
    loadDataForGps(currPos, sizeof(netBuffer)-(currPos-netBuffer));
                                                      RM_LOG2(F("Sending GPS via GPRS"), netBuffer);
    //Try GPRS, else SMS
    uint16_t fCode = sendViaGprs(netBuffer);
    
    if (fCode != GPRS_CODE_SUCCESS) {
                                                      RM_LOG2(F("GPRS Failed - Trying SMS..."),fCode);
      char smsBuffer[140]; //SMS limit
      
      currPos = initBufferForSend(smsBuffer, fCode);
      loadDataForGps(currPos, sizeof(smsBuffer)-(currPos-smsBuffer));
      
      if (!sendViaSms(netBuffer)) {
                                                      RM_LOG(F("SMS Send Failed !"));
      }
    }
}

/* Transmits sensor measurements via GPRS or SMS (fallback) */
void execTransmitReadings() {
  
      char netBuffer[(15*12)+20]; //12 readings+header

      //Load header data
      char* currPos = initBufferForSend(netBuffer, 0);

      //Load data in GPRS format
      loadDataForReadings(currPos, sizeof(netBuffer)-(currPos-netBuffer)); //Get in GPRS format
                                                          
      //If GPRS failure, try SMS

      uint16_t fCode = sendViaGprs(netBuffer);
      
      if (fCode != GPRS_CODE_SUCCESS) {
                                                      //RM_LOG2(F("GPRS Failed-Trying SMS..."),fCode);

        char smsBuffer[140]; //SMS limit

        //Load header data
        currPos = initBufferForSend(smsBuffer, fCode);
        
        loadDataForReadings(currPos, sizeof(smsBuffer)-(currPos-smsBuffer));
                                                      //RM_LOG2(F("Sending Sensors Cmpt"),smsBuffer);
        
        if (!sendViaSms(smsBuffer)) {
          //If even SMS if failing, can't do much
                                                      RM_LOG(F("SMS Send Failed !!"));
        }
      }
}


/* Transmits the given data via GPRS. Returns a status-code corresponding to the HTML code received from server. */
uint16_t sendViaGprs(char* data) {
  
                                                      RM_LOG2(F("Sending via GPRS"),data);
#ifdef DEBUG
  return GPRS_CODE_SUCCESS;
#elif defined(RUNTIME_TEST)
  return GPRS_CODE_SUCCESS;
#endif

      uint16_t ret;
      if (!fona.enableGPRS(true)) {
      
        ret = GPRS_CODE_ENABLE_FAIL; 
      }
      else {
          // Post data to website
          uint16_t statuscode;
          int16_t length;
  

          //First try with the default APN settings
          boolean succ = fona.HTTP_POST_start(_dataDestinationUrl, F("text/plain"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length);

          //On failure, try custom APN settings
          if (!succ && _customApnName != "") {
            
            fona.setGPRSNetworkSettings(F(_customApnName), F(_customApnUser), F(_customApnPwd));
            succ = fona.HTTP_POST_start(_dataDestinationUrl, F("text/plain"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length);
          }

          if (!succ) {
            
             ret = GPRS_CODE_POST_FAIL;
          }
          else{
                                                          RM_LOG2(F("GPRS Status:"), statuscode);
            char dataReceived[16];
            uint8_t currRecIdx = 0;
            
            while (length > 0) {
              while (fona.available()) {
                char c = fona.read();
                
                dataReceived[currRecIdx++] = c;

                length--;
                if (! length) break;
              }
            }
            
                                                          RM_LOG2(F("GPRS POST Response:"), currRecIdx);
            fona.HTTP_POST_end();
    
            //200 => Success
            ret = statuscode;
         }

         //Shut down GPRS as consumes battery
          fona.enableGPRS(false);
      }
      
      return ret;
}

/* Transmits the given data via SMS. Returns 0 on success, 1 on failure. */
uint8_t sendViaSms(char* data) {

                                                      RM_LOG2(F("Sending via SMS"),data);
#ifdef DEBUG
  return 0; //Success
#else
     if (!fona.sendSMS(_smsDestination, data)) {
      
        return 1;
     } else {
      
          return 0;
     }
#endif
}

/* Reads analog sensors and stores in current cycle measurements */
void readSensorsAsync() {
  //Collect sensor data for 1 minute, calc average, remove outliers

                                                      RM_LOG(F("Reading Sensors"));

    //Cumulative average
    int batt =  analogRead(A0);
    if (batt > 0) {
      
      float currReading = currSensorData->VBatt * currSensorData->NoOfBattReadings;
      currSensorData->NoOfBattReadings++;
      currSensorData->VBatt = (currReading + batt)/currSensorData->NoOfBattReadings; //Will be >0 denom because of above
    }
    
    int curr = analogRead(A1);
    if (curr > 0) {
      
      float currReading = currSensorData->Current * currSensorData->NoOfCurrReadings;
      currSensorData->NoOfCurrReadings++;
      currSensorData->Current = (currReading + curr)/currSensorData->NoOfCurrReadings; //Will be >0 denom because of above
    }
    
    int pv = analogRead(A2);
    if (pv > 0) {
      
      float currReading = currSensorData->VPV * currSensorData->NoOfPVReadings;
      currSensorData->NoOfPVReadings++;
      currSensorData->VPV = (currReading + pv)/currSensorData->NoOfPVReadings; //Will be >0 denom because of above
    }
    
    int temp = analogRead(A5);
    if (temp > 0) {
      
      float currReading = currSensorData->Temp * currSensorData->NoOfTempReadings;
      currSensorData->NoOfTempReadings++;
      currSensorData->Temp = (currReading + temp)/currSensorData->NoOfTempReadings; //Will be >0 denom because of above
    }
                                                      //RM_LOG2(F("Sensors-VPV"), cData.VPV);
    
                                                     // RM_LOG2(F("Sensors-Curr"), cData.Current);

#ifdef DEBUG
    currSensorData->VBatt=currCycleNo;
    currSensorData->VPV=currCycleNo+1;
    currSensorData->Current=currCycleNo+2;
#endif

}

void addError(char* err) {  
      RM_LOG2(F("Adding Error"), err);
}

/* Populates the supplied buffer with GPS data from current cycle */
void loadDataForGps(char* buffer, int maxSize) {
    char* curr = buffer;

                                                  //  RM_LOG2(F("Buffer"), buffer);
                                                    RM_LOG2(F("Raw Data"), gpsData.raw);
                                                 //   RM_LOG2(F("Max Sz"), maxSize);
                                                  //  RM_LOG2(F("Loc Data"), gpsData.gsmLoc);
                                                  //  RM_LOG2(F("Raw Sz"), gpsData.rawSz);
                                                   // RM_LOG2(F("Loc Sz"), gpsData.gsmLocSz);
                                                    
    //Header to indicate data type
    if (gpsData.gsmLocSz == 0 && gpsData.rawSz == 0) {
      sprintf(curr++, "%s", "X");
    }
    else if (gpsData.gsmLocSz == 0) {
      sprintf(curr++, "%s", "P"); //gPs format
      
      strncpy(curr,gpsData.raw,gpsData.rawSz);
      curr += gpsData.rawSz;
    }
    else if (gpsData.rawSz == 0) {
      sprintf(curr++, "%s", "S");  //gSm format
      
      strncpy(curr,gpsData.gsmLoc,gpsData.gsmLocSz);
      curr += gpsData.gsmLocSz;
    }
    else{
      //Both valid, choose one randomly
                
      if (millis()%2==0) {
       // sprintf(curr++, "%s", "T"); 
      
        char local[gpsData.rawSz+1];
        strncpy(local, gpsData.raw, gpsData.rawSz);
        local[gpsData.rawSz]=0;
        
        strncpy(curr, local, gpsData.rawSz);
      }
      else {
     //   sprintf(curr++, "%s", "U"); 
      
        char local[gpsData.gsmLocSz+1];
        strncpy(local, gpsData.gsmLoc, gpsData.gsmLocSz);
        local[gpsData.gsmLocSz]=0;
        
        strncpy(curr, local, gpsData.gsmLocSz);
      }
    }

    //This gets truncated for some reason
                                                    //RM_LOG2(F("Buffer2"), buffer);
                                                    RM_LOG2(F("Raw Data22"), gpsData.raw);
                                                   // RM_LOG2(F("Loc Data22"), gpsData.gsmLoc);
                                                   // RM_LOG2(F("Raw Sz22"), gpsData.rawSz);
                                                    //RM_LOG2(F("Loc Sz22"), gpsData.gsmLocSz);
}


/* Populates the supplied buffer with sensor readings data from current cycle */
void loadDataForReadings(char* buffer, int maxSize) {

    char* curr = buffer;
    
    //Header to indicate data type
    sprintf(curr, "%s", "R");
    curr+=1;

    maxSize-=3;

    uint8_t readingSize = 7*3;
    uint8_t maxNoOfReadings = maxSize/readingSize;
    uint8_t totalNoOfReadings = sizeof(sensorDataArr)/sizeof(SensorData);

                                                    RM_LOG2(F("Max Sz"), maxSize);
                                                    RM_LOG2(F("maxNoOfReadings"), maxNoOfReadings);
                                                    RM_LOG2(F("totalNoOfReadings"), totalNoOfReadings);
                                                    
    if (maxNoOfReadings == 0) //No space for any readings :|
      return;

    uint8_t jumpValue = totalNoOfReadings/maxNoOfReadings;
    if (jumpValue==0) //i.e. more space than available readings
        jumpValue=1;
    
    for(uint8_t readingIdx=0 ; readingIdx<totalNoOfReadings ; readingIdx += jumpValue) {

                                                    RM_LOG2(F("Reading Idx"), readingIdx);
      SensorData reading = sensorDataArr[readingIdx];
      
      //Copy Sensor data in
      //char battChar[8], pvChar[8], currChar[8];

      sprintf(curr, "this: %07.3f", reading.VBatt); //102.345 Volts, for example
      //dtostrf (reading.VBatt,7,3, battChar);
      //strncpy(curr,battChar,7);

      sprintf(curr+=7, "this: %07.3f", reading.VPV);
      //dtostrf (reading.VPV,7,3, pvChar);
     // strncpy(curr+=7,pvChar,7);

      sprintf(curr+=7, "this: %07.3f", reading.Current);
      //dtostrf (reading.Current,7,3, currChar);
      //strncpy(curr+=7,currChar,7);
      
      curr+=7;
      //If buffers change, modify readingSize variable too
    }
}

/* Checks battery level and returns true if needs more time to charge the battery */
boolean ensureBatteryLevel() {

     uint16_t vbat;
     if (!fona.getBattPercent(&vbat)) {
                                                    RM_LOG(F("BatteryLevel - Failed To Retrieve"));
      
        addError(ERROR_BATT_LEVEL_FAILED);
        return false;
    } else {
                                                    RM_LOG2(F("BatteryLevel Retrieved"), vbat);

        //Require charging if less than threshold
        return vbat <= BATT_CHARGE_THRESHOLD;
    }
}


/* Kicks off the GPS-Refresh process */
void triggerGpsRefreshAsync() {

                                                    RM_LOG(F("GPS - Switching On"));
  if (!fona.enableGPS(true))
      addError(ERROR_GPS_ENABLE_FAILED);
  
  //if (!fona.enableGPRS(true)) //We can use GSM-LOC to give us location data via GSM network!
   //   addError("P-EB");
}

/* Retrieves GPS data from GPS sub-module and stores in current cycle data*/
GpsData getGpsSensorData() {

    GpsData ret;

                                                    RM_LOG(F("GPS Getting Loc Data"));
    // check for GSMLOC (requires GPRS)
    boolean gsmLocSuccess = false;
    {
        uint16_t returncode;
  
        if (fona.getGSMLoc(&returncode, replybuffer, 250)) {
            if (returncode == 0) {

              //TODO: Can I even do this - storing local to global?
              char gsmLocData[50];
              strncpy(gsmLocData,replybuffer, 50);
              ret.gsmLoc = gsmLocData;
              ret.gsmLocSz = sizeof(gsmLocData);
                                              RM_LOG2(F("GSM Orig.LocData"),ret.gsmLoc);
                                              RM_LOG2(F("GSM Orig.LocDataSz"),ret.gsmLocSz);
              gsmLocSuccess = true;
            }
        }
        else
          returncode=9; //our own :)
        
        ret.gsmErrCode = returncode;
    }
        
    // check GPS fix
    int8_t stat;
    stat = fona.GPSstatus();
    boolean isGpsSuccess = false;
    
    if (stat < 0)
      addError(new char[3]{'G','S',stat}); //test this concat
    else {    
          
        if (stat > 1){ //If we have a fix
          isGpsSuccess = true;
          ret.is3DFix = stat == 3;

          char gpsdataArr[120];
          fona.getGPS(0, gpsdataArr, 120);

          //TODO: Can I even do this - storing local to global?
          ret.raw = gpsdataArr;
          ret.rawSz = sizeof(gpsdataArr);
          
                                              RM_LOG2(F("GPS Retrieved-Full"),ret.raw);
      }
    }

    ret.success = gsmLocSuccess || isGpsSuccess;

    return ret;
}

/* Ends the GPS retrieval process */
void onGpsComplete() {
                                                    RM_LOG(F("GPS Shutting down"));
  fona.enableGPS(false);
  //fona.enableGPRS(false);
}



