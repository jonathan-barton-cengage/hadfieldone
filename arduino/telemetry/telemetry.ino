//includes
#include <TinyGPS++.h>
#include <Wire.h>
#include <SD.h> //SD Logger
#include <Adafruit_Sensor.h>  //bmp180
#include <Adafruit_BMP085_U.h>  //bmp180 (formerly 085)
#include <SoftwareSerial.h>
#include <util/crc16.h>
//pin defines
#define LOGPIN 10 //10 is sd logger chipset for adafruit sd logger shield
#define RADIOPIN 9
#define BATTERY_ADC A0
//constants
//const int sdLoggerChipSelect = 10; //adafruit sd logger shield chipset define
const float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
const char logFileName[] = "hadfield.log";
const char sentenceIdFileName[] = "sentence.dat";
const char callSign[] = "KIHAD";
static const int GPS_ARDUINO_RX = 4;
static const int GPS_ARDUINO_TX = 3;
static const uint32_t GPSBaud = 4800;
const uint32_t serialBaud = 115200;
const uint32_t RADIO_LOW = 100; //105
const uint32_t RADIO_HIGH = 10; //120
//sensors
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085); 
//RTC_DS1307 RTC;
//gps
TinyGPSPlus gps;
//emulated serial connection to the GPS
SoftwareSerial ss(GPS_ARDUINO_RX, GPS_ARDUINO_TX);
//logging related
uint16_t sentence_id = 0;
//debug
const int debug = 1;

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

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

// $$CALLSIGN,sentence_id,time,latitude,longitude,altitude,optional speed,optional bearing,optional internal temperature,*CHECKSUM\n 
char * getSensorMessage(){  
  char timestamp[9]="--------";
  char temp_buffer[7];
  char pre_buffer[9];
  char alt_buffer[9];
  float temperature;
  float pressure;
  float altitude; 
  char lat[12]="-----------";
  char lon[12]="-----------";
  char gps_altitude[10] = "---------";
  uint16_t crc;
  char txstring[100] = "";

  //gps read
  if (gps.location.isUpdated() && gps.location.isValid() && gps.altitude.isValid())
  {
    dtostrf(gps.location.lat(),5, 6, lat);
    dtostrf(gps.location.lng(),5, 6, lon);
    dtostrf(gps.altitude.meters(), 5, 2, gps_altitude);
    sprintf(timestamp, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    sentence_id = gps.sentencesWithFix();
    smartDelay(0);
  } else {
    sentence_id = (sentence_id > 0) ? sentence_id+1:sentence_id; 
  }
        
  bmp.getTemperature(&temperature); 
  dtostrf(temperature, 4, 2, temp_buffer);
  bmp.getPressure(&pressure); //in hPa
  if(pressure) pressure /= 100;
  dtostrf(pressure, 6, 2, pre_buffer);
  sprintf(txstring,"$$%s,%i,%s,%s,%s,%s,%s", callSign, sentence_id, timestamp, lat, lon, pre_buffer, temp_buffer);
  crc = gps_CRC16_checksum(txstring);
  sprintf(txstring,"%s*%04X",txstring,crc);
  
  //calc crc

  return txstring;
  
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
      safeSerialPrintLn("1. Failed to load SD card.");
      return;
    }
    safeSerialPrintLn("2. Loading last sentence_id...");
}

void loop(void){
   File hadfieldDataLog = SD.open(logFileName, FILE_WRITE); 
   char * message;
   message = getSensorMessage();
   if(hadfieldDataLog){
     safeSerialPrintLn("4. Opened data log.");
     hadfieldDataLog.println(message);
     hadfieldDataLog.close();
   }
   rtty_txstring (message);
   safeSerialPrintLn(message);
  // Dispatch incoming characters
  
  smartDelay(1000);
}
