//Modem selection
#define TINY_GSM_MODEM_SIM7000SSL
// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

//#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]  = "airtelgprs.com";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
#include <Ticker.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define uS_TO_S_FACTOR      1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP       60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD           9600
#define PIN_DTR             25
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4

#define SD_MISO             2
#define SD_MOSI             15
#define SD_SCLK             14
#define SD_CS               13
#define LED_PIN             12

const char server[]   = "script.google.com";
//char resource[] = "/macros/s/<your_sheets_id_goes_here>/exec?";
String resourceStrMain = "/macros/s/<your_sheets_id_goes_here>/exec?";
const int  port       = 443;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsmClientSecure client(modem);
HttpClient          http(client, server, port);

void enableGPS(void)
{
    // Set SIM7000G GPIO4 LOW ,turn on GPS power
    // CMD:AT+SGPIO=0,4,1,1
    // Only in version 20200415 is there a function to control GPS power
    modem.sendAT("+SGPIO=0,4,1,1");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" SGPIO=0,4,1,1 false ");
    }
    modem.enableGPS();


}

void disableGPS(void)
{
    // Set SIM7000G GPIO4 LOW ,turn off GPS power
    // CMD:AT+SGPIO=0,4,1,0
    // Only in version 20200415 is there a function to control GPS power
    modem.sendAT("+SGPIO=0,4,1,0");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" SGPIO=0,4,1,0 false ");
    }
    modem.disableGPS();
}

void modemPowerOn()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1000);    //Datasheet Ton mintues = 1S
    digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1500);    //Datasheet Ton mintues = 1.2S
    digitalWrite(PWR_PIN, HIGH);
}


void modemRestart()
{
    modemPowerOff();
    delay(1000);
    modemPowerOn();
}

void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);

    delay(10);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    modemPowerOn();

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    Serial.println("/**********************************************************/");
    Serial.println("/**********************************************************/\n\n");

    delay(10000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(2000);
    digitalWrite(PWR_PIN, LOW);

    SerialMon.println("Wait...");

  // Set GSM module baud rate
  //TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  SerialAT.begin(9600, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  
  Serial.println("Initializing modem...");
  int retryCount = 5;
  while(retryCount>0){
    if(modem.init()){
      break;
    }else{
      Serial.println("Failed to init modem, attempting to retry...");
      retryCount--;
      delay(1000);
    }
    if(retryCount==0){
      ESP.restart();
    }
  }
  /*
  if (!modem.init()) {
        Serial.println("Failed to init modem, attempting to continue without restarting");
  }
  */
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

}


void sendDataToServer(float lat2,float lon2,float accuracy2,float speed2,float alt2,
                        int hour2,int min2,int sec2,int year2,int month2,int day2){
  
  SerialMon.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive();  // Currently, this is needed for HTTPS
  
  SerialMon.println("Framing resource line:");


  String resourceStr = "";
  String finalResourceStr = "";
  resourceStr += "param1=";
  
  resourceStr += String(lat2,4);
  resourceStr += "&";

  resourceStr += "param2=";
  resourceStr += String(lon2,4);
  resourceStr += "&";

  resourceStr += "param3=";
  resourceStr += String(speed2,4);
  resourceStr += "&";

  resourceStr += "param4=";
  resourceStr += String(alt2,4);
  resourceStr += "&";

  resourceStr += "param5=";
  resourceStr += String(hour2);
  resourceStr += "&";
  
  resourceStr += "param6=";
  resourceStr += String(min2);

  finalResourceStr = resourceStrMain + resourceStr;
  int str_len = finalResourceStr.length() + 1; 
  char resource[str_len];
  finalResourceStr.toCharArray(resource, str_len);

  SerialMon.println(resource);
  
  SerialMon.println("Framing resource line completed");
  SerialMon.println(resource);

  int err = http.get(resource);
  if (err != 0) {
    SerialMon.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  SerialMon.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName  = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    SerialMon.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    SerialMon.print(F("Content length is: "));
    SerialMon.println(length);
  }
  if (http.isResponseChunked()) {
    SerialMon.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  SerialMon.println(F("Response:"));
  SerialMon.println(body);

  SerialMon.print(F("Body length is: "));
  SerialMon.println(body.length());

  // Shutdown

  http.stop();
  SerialMon.println(F("Server disconnected"));
}


void loop()
{
    if (!modem.testAT()) {
        Serial.println("Failed to restart modem, attempting to continue without restarting");
        modemRestart();
        return;
    }

    Serial.println("Start positioning . Make sure to locate outdoors.");
    Serial.println("The blue indicator light flashes to indicate positioning.");

    enableGPS();

    SerialMon.print("Waiting for GSM network...");
    if (!modem.waitForNetwork()) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    SerialMon.println(" success");
    if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }

    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    SerialMon.println(" success");

    if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }
    //float lat,  lon;
    while (1) {
        /*
        if (modem.getGPS(&lat, &lon)) {
            Serial.println("The location has been locked, the latitude and longitude are:");
            Serial.print("latitude:"); Serial.println(lat);
            Serial.print("longitude:"); Serial.println(lon);
            break;            
        }*/
        //digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        float lat2      = 0;
        float lon2      = 0;
        float speed2    = 0;
        float alt2      = 0;
        int   vsat2     = 0;
        int   usat2     = 0;
        float accuracy2 = 0;
        int   year2     = 0;
        int   month2    = 0;
        int   day2      = 0;
        int   hour2     = 0;
        int   min2      = 0;
        int   sec2      = 0;

        while(1){  
        if (modem.getGPS(&lat2, &lon2, &speed2, &alt2, &vsat2, &usat2, &accuracy2,
                     &year2, &month2, &day2, &hour2, &min2, &sec2)) {
          Serial.print("latitude:"); Serial.println(lat2,4);
          Serial.print("longitude:"); Serial.println(lon2,4);
          Serial.print("Accuracy:"); Serial.println(accuracy2);
          
          Serial.print("Speed:"); Serial.println(speed2);
          Serial.print("Altitue:"); Serial.println(alt2);
          

          Serial.print("Hour:"); Serial.println(hour2);
          Serial.print("Min:"); Serial.println(min2);
          Serial.print("Sec:"); Serial.println(sec2);
          
          Serial.print("Year:"); Serial.println(year2);
          Serial.print("Month:"); Serial.println(month2);
          Serial.print("Day:"); Serial.println(day2); 
          break;
        } else {
          Serial.println("Couldn't get GPS/GNSS/GLONASS location, retrying in 15s.");
          delay(15000);
        }
        }        
        sendDataToServer(lat2,lon2,accuracy2,speed2,alt2,hour2,min2,sec2,year2,month2,day2);
        delay(5000);
        int flickerCount=0;
        while(flickerCount<10){
          digitalWrite(LED_PIN, HIGH);
          delay(200);
          digitalWrite(LED_PIN, LOW);
          delay(200); 
          flickerCount++;
        }
        
    }
}
