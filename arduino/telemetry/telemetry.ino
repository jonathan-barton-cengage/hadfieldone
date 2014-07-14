//includes
#include <TinyGPS++.h>
#include <Wire.h>
#include <RTClib.h>
#include <SD.h> //SD Logger
#include <Adafruit_Sensor.h>  //bmp180
#include <Adafruit_BMP085_U.h>  //bmp180 (formerly 085)
#include <util/crc16.h>
#include <SoftwareSerial.h>
//pin defines
#define LOGPIN 10 //10 is sd logger chipset for adafruit sd logger shield
#define RADIOPIN 9
#define GPS_ARDUINO_RX 4
#define GPS_ARDUINO_TX 3
#define BATTERY_ADC A0
//constants
//const int sdLoggerChipSelect = 10; //adafruit sd logger shield chipset define
const float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
const char logFileName[] = "hadfield.log";
const char sentenceIdFileName[] = "sentence.dat";
const char logHeader[] = "KIHAD";
const char callSign[] = "KIHAD";
const char infoHeader[] = "[INFO]";
const char dataHeader[] = "[DATA]";
const char errorHeader[] = "[ERROR]";
const uint32_t GPSBaud = 4800;
const uint32_t serialBaud = 115200;
const uint32_t RADIO_LOW = 100; //105
const uint32_t RADIO_HIGH = 10; //120
//sensors
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085); 
RTC_DS1307 RTC;
//gps
TinyGPSPlus gps;
//emulated serial connection to the GPS
SoftwareSerial ss(GPS_ARDUINO_RX, GPS_ARDUINO_TX);
//logging related
uint16_t sentence_id = 1;
char txstring[100];
//debug
const int debug = 1;

uint16_t calculateNmeaXor(String theseChars) {
  int check = 0;
  for (int c = 0; c < theseChars.length(); c++) {
    check = int(byte(check) ^ byte(theseChars.charAt(c)));
  } 
  return check;
}

uint16_t gps_CRC16_checksum (char *string)
{
   size_t i;
   uint16_t crc;
   uint8_t c;
   
  crc = 0xFFFF;
   
  // Calculate checksum ignoring the first two $s
   for (i = 2; i < strlen(string); i++)
   {
   c = string[i];
   crc = _crc_xmodem_update (crc, c);
   }
   
  return crc;
}

// $$CALLSIGN,sentence_id,time,latitude,longitude,altitude,optional speed,optional bearing,optional internal temperature,*CHECKSUM\n 
void setSensorMessage(char * message){
  DateTime now = RTC.now(); 
  char *temp_buffer="", *pre_buffer="", *alt_buffer="", *timestamp="";
  float temperature, pressure, altitude; 
  char *lat="", *lon="", *gps_altitude="";
  uint16_t batteryadc_v, crc;
 //lat read
  dtostrf(gps.location.lat(),11, 6, lat);
  //lon read
  dtostrf(gps.location.lng(),12, 6, lon);
  //gps alt read
  dtostrf(gps.altitude.meters(), 7, 2, gps_altitude);
  //temperature read
  bmp.getTemperature(&temperature); 
  dtostrf(temperature, 2, 2, temp_buffer); 
  //pressure read
  bmp.getPressure(&pressure);
  dtostrf(pressure, 8, 2, pre_buffer);
  //altitude calculate
  altitude = bmp.seaLevelForAltitude(seaLevelPressure, pressure, temperature);
  dtostrf(altitude, 8, 2, alt_buffer); 
  //read battery voltage
  batteryadc_v=analogRead(BATTERY_ADC)*4.8; 
  //create timestamp
  TinyGPSTime &t = gps.time; 
  snprintf(timestamp, 9, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  safeSerialPrintLn(timestamp);

  //create stub
  //snprintf(txstring,100,"%s,%i,%s,%s,%s,%s,%s,%s,%i",callSign,sentence_id,timestamp,lat,lon,alt_buffer,temp_buffer,pre_buffer,batteryadc_v);
  
  //calc crc
  crc = calculateNmeaXor(txstring); //gps_CRC16_checksum(txstring);
  //finalize
  //snprintf(txstring,100, "$$%s*%i",txstring,crc);
  //message = txstring;
  
}

void rtty_txstring (char * string)
{
  char c;
  c = *string++;  
  while ( c != '\0') {
     rtty_txbyte (c);
     c = *string++;
  }
}

void rtty_txbyte (char c) {
  int i;
  rtty_txbit (0); // Start bit
  // Send bits for for char LSB first
  for (i=0;i<7;i++) // Change this here 7 or 8 for ASCII-7 / ASCII-8
   {
     if (c & 1) rtty_txbit(1);
     else rtty_txbit(0);
     c = c >> 1;
  }
    rtty_txbit (1); // Stop bit
    rtty_txbit (1); // Stop bit
}
 
void rtty_txbit (int bit) {
 if (bit) {
   // high
   analogWrite(RADIOPIN,RADIO_HIGH);
 }
 else {
   // low
   analogWrite(RADIOPIN,RADIO_LOW);
}
 
// delayMicroseconds(3370); // 300 baud
 delayMicroseconds(10000); // For 50 Baud uncomment this and the line below.
 delayMicroseconds(10150); // You can't do 20150 it just doesn't work as the
 // largest value that will produce an accurate delay is 16383
 // See : http://arduino.cc/en/Reference/DelayMicroseconds
 
}

void setPwmFrequency(int pin, int divisor) {
 byte mode;
 if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
   switch(divisor) {
   case 1:
     mode = 0x01;
   break;
   case 8:
     mode = 0x02;
   break;
   case 64:
     mode = 0x03;
   break;
   case 256:
     mode = 0x04;
   break;
   case 1024:
     mode = 0x05;
   break;
   default:
     return;
   }
   if(pin == 5 || pin == 6) {
     TCCR0B = TCCR0B & 0b11111000 | mode;
   }
   else {
     TCCR1B = TCCR1B & 0b11111000 | mode;
   }
 } else if(pin == 3 || pin == 11) {
     switch(divisor) {
       case 1:
         mode = 0x01;
       break;
       case 8:
         mode = 0x02;
       break;
       case 32:
         mode = 0x03;
       break;
       case 64:
         mode = 0x04;
       break;
       case 128:
         mode = 0x05;
       break;
       case 256:
         mode = 0x06;
       break;
       case 1024:
         mode = 0x7;
       break;
       default:
         return;
     }
     TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

void incrementSentenceId(){
 sentence_id += 1;
 File sentenceIdFile = SD.open(sentenceIdFileName, FILE_WRITE);
 if(sentenceIdFile){
   sentenceIdFile.seek(0);
   sentenceIdFile.println("                                 ");
   sentenceIdFile.seek(0);
   sentenceIdFile.println(sentence_id);
   sentenceIdFile.close(); 
 }
}

void initSentenceId(){
  File sentenceIdFile = SD.open(sentenceIdFileName, FILE_READ);
  String lastSentenceId;
  if(sentenceIdFile.available()){
    safeSerialPrintLn("3. Reading sentence_id file...");
    sentence_id = 1;
     //lastSentenceId = sentenceIdFile.read();
  }
  sentenceIdFile.close();
 // sentence_id = lastSentenceId.toInt();
}

void safeSerialPrintLn(char * message){
  if(debug == 1){
    Serial.println(message);
  }  
}

void setup(void){
    //debug setup
    if(debug == 1){ 
      Serial.begin(serialBaud); 
    }
    //sensor setup
    Wire.begin();
    if(!bmp.begin()){
      safeSerialPrintLn("0. Sensor is not functioning. ");       
    }
    //rtc setup
    RTC.begin();
    if (! RTC.isrunning()) { 
      safeSerialPrintLn("0. Realtime Clock is not Functioning. ");
      RTC.adjust(DateTime(__DATE__, __TIME__)); 
    }

    //gps setup    
    ss.begin(GPSBaud);
    
    //radio setup
    pinMode(RADIOPIN, OUTPUT);
    setPwmFrequency(RADIOPIN, 1);
    //log setup
    pinMode(LOGPIN, OUTPUT);
    if(SD.begin(LOGPIN)){
      safeSerialPrintLn("1. SD card initialized.");
    }else{
      safeSerialPrintLn("Failed to load SD card");
      return;
    }
    safeSerialPrintLn("2. Loading last sentence_id...");
    initSentenceId();
}

void loop(void){
   safeSerialPrintLn("4. Opening data log...");
   File hadfieldDataLog = SD.open(logFileName, FILE_WRITE); 
   char * message;
   setSensorMessage(message);
   if(hadfieldDataLog){
     hadfieldDataLog.println(message);
     hadfieldDataLog.close();
   }
   rtty_txstring (message);
   safeSerialPrintLn(message);
   incrementSentenceId();
 delay(2000);
}
