

// #include <Arduino.h>
// #include <WiFi.h>
// #include "time.h"

// void printLocalTime();
// unsigned long Get_Epoch_Time();

// const char* ssid       = "SFR_EC58";
// const char* password   = "96wwza4yfz24qhtc4mxq";

// // to set the static IP address to 192, 168, 1, 184
// IPAddress local_IP(192, 168, 1, 24);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress dns(192, 168, 1, 1);     //needed to go on internet
// IPAddress subnet(255, 255, 255, 0);

// const char* ntpServer = "pool.ntp.org";
// const long  gmtOffset_sec = 3600;
// const int   daylightOffset_sec = 3600;

// const long uS_TO_S_FACTOR = 1000000; /* Conversion factor for micro seconds to seconds */
// const int TIME_TO_SLEEP_DAY = 1 * 10;    /* Time ESP32 will go to sleep (in seconds) */

// unsigned long Epoch_Time;

// void setup()
// {
//   Serial.begin(9600);
//   esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_DAY * uS_TO_S_FACTOR);
//   WiFi.mode(WIFI_STA);
//   if (!WiFi.config(local_IP, gateway, subnet, dns)) // the order matters
//   {
//     Serial.println("IP adress could not be set to 192.168.1.24");
//   }

//   //connect to WiFi
//   Serial.printf("Connecting to %s ", ssid);
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//       delay(500);
//       Serial.print(".");
//   }
//   Serial.println(" CONNECTED");

//   //init and get the time
//   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//   //delay(10000);
//   printLocalTime();

//   //disconnect WiFi as it's no longer needed
//   WiFi.disconnect(true);
//   WiFi.mode(WIFI_OFF);
// }

// void loop()
// {
//   //delay(1000);
//   Epoch_Time = Get_Epoch_Time();
//   Serial.print("Epoch Time: ");
//   Serial.println(Epoch_Time);
//   esp_light_sleep_start();
// }

// void printLocalTime()
// {
//   struct tm timeinfo;
//   if(!getLocalTime(&timeinfo)){
//     Serial.println("Failed to obtain time");
//     return;
//   }
//   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
// }

// // Get_Epoch_Time() Function that gets current epoch time
// unsigned long Get_Epoch_Time() {
//   time_t now;
//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo)) {
//     Serial.println("Failed to obtain time");
//     return(0);
//   }
//   time(&now);
//   return now;
// }

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "DHT.h"

// pas encore compris pourquoi je dois initilialiser les fonctions ici.

void printLocalTime();
unsigned long Get_Epoch_Time();

// Connection to wifi

const char *ssid = "SFR_EC58";
const char *password = "96wwza4yfz24qhtc4mxq";

// to set the static IP address to 192, 168, 1, 24
IPAddress local_IP(192, 168, 1, 24);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1); //needed to go on internet

const int ARRAYSIZE = 3100;  // risk overflow
String Data_wifi[ARRAYSIZE]; // a single string of data will be sent by wifi
boolean Trigger_Time_Zero_For_Wifi = false;
unsigned long Time_Wifi_Zero;
unsigned long Time;

WiFiServer server(80);

// parameter for time with NTP

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
unsigned long Epoch_Time;

// parameter light sleep

const long uS_TO_S_FACTOR = 1000000; /* Conversion factor for micro seconds to seconds */
const int TIME_TO_SLEEP_DAY = 1 * 10; /* Time ESP32 will go to sleep (in seconds) */

// parameter DHT11

#define DHTPIN 13     // Digital pin connected to the DHT sensor use pins 3, 4, 5, 12, 13 or 14 --
#define DHTTYPE DHT11 // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// internal parameter

unsigned long it = 0; // iteration to save data in array

void setup()
{
  Serial.begin(9600);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_DAY * uS_TO_S_FACTOR);
  dht.begin();

  //connect to WiFi

  if (!WiFi.config(local_IP, gateway, subnet, dns))
  {
    Serial.println("IP adress could not be set to 192.168.1.24");
  }

  Serial.printf("Connecting to %s ", ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //delay(1000);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  //server.begin();
}

void loop()
{
  Epoch_Time = Get_Epoch_Time();
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  //printLocalTime();

  Data_wifi[it] = String(String(Epoch_Time) + "," + String(h) + "," + String(t));
  Serial.println(Data_wifi[it]);

  it++;

  if (it == 10)
  {

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      Serial.print(".");
    }
    Serial.println(" CONNECTED");
    server.begin();

    while (1)
    {
      WiFiClient client = server.available();
      //Serial.println("Client connected");
      //delay(100);
      if (client)
      {
        Serial.println("Client connected");
        //Trigger_Time_Zero_For_Wifi = false;

        // while (client.connected())
        // { // Attention, Si perds la connection wifi, les temps ne seront plus corrects

          // if ((client.connected()) // && (Trigger_Time_Zero_For_Wifi == false))
          // {
          //   Time_Wifi_Zero = millis();
          //   Trigger_Time_Zero_For_Wifi = true;
          // }
          for (int it2 = 0; it2 < it; it2++)
          {
            Serial.println(Data_wifi[it2]);
            delay(100);
            client.println(Data_wifi[it2]);
            //delay(100);
          }
          it = 0;
          delay(30000);
          client.stop();
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);
          break;

//        }
      }
    }
  }

  esp_light_sleep_start();
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Get_Epoch_Time() Function that gets current epoch time
unsigned long Get_Epoch_Time()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

// #include <Arduino.h>
// #include <WiFi.h>
// #include "DHT.h"

// // Wifi parameter (put your own)

// const char *ssid = "SFR_EC58";
// //const char *password = "uguele2vocuminhonext"; //parents
// const char *password = "96wwza4yfz24qhtc4mxq";

// // to set the static IP address to 192, 168, 1, 184
// IPAddress local_IP(192, 168, 1, 24);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);

// const int ARRAYSIZE = 3100; // risk of overflow
// String Data_wifi[ARRAYSIZE]; // a single string of data will be sent by wifi
// boolean Trigger_Time_Zero_For_Wifi = false;
// unsigned long Time_Wifi_Zero;
// unsigned long Time;

// WiFiServer server(80);

// #define DHTPIN 13 // Digital pin connected to the DHT sensor
// // Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// // Pin 15 can work but DHT must be disconnected during program upload.

// #define DHTTYPE DHT11 // DHT 11

// DHT dht(DHTPIN, DHTTYPE);

// void setup()
// {
//   Serial.begin(9600);
//   Serial.println(F("DHT11 test!"));
//   dht.begin();

//   if (!WiFi.config(local_IP, gateway, subnet))
//   {
//     Serial.println("IP adress could not be set to 192.168.1.24");
//   }

//   WiFi.begin(ssid, password);

//   while (WiFi.status() != WL_CONNECTED)
//   {
//     delay(500);
//     // Serial.print(".");
//   }

//   server.begin();
// }

// void loop()
// {

//   WiFiClient client = server.available();

//   if (client)
//   {
//     Trigger_Time_Zero_For_Wifi = false;

//     while (client.connected())
//     { // Attention, Si perds la connection wifi, les temps ne seront plus corrects

//       if ((client.connected()) && (Trigger_Time_Zero_For_Wifi == false))
//       {
//         Time_Wifi_Zero = millis();
//         Trigger_Time_Zero_For_Wifi = true;
//       }

//       // Wait a few seconds between measurements.
//       delay(2000);

//       // Reading temperature or humidity takes about 250 milliseconds!
//       // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
//       float h = dht.readHumidity();
//       // Read temperature as Celsius (the default)
//       float t = dht.readTemperature();

//       // Check if any reads failed and exit early (to try again).
//       if (isnan(h) || isnan(t))
//       {
//         Serial.println(F("Failed to read from DHT sensor!"));
//         return;
//       }

//       // Compute heat index in Celsius (isFahreheit = false)
//       //float hic = dht.computeHeatIndex(t, h, false);

//       // Serial.print(F("Humidity: "));
//       // Serial.print(h);
//       // Serial.print(",");
//       // Serial.print(F("Temperature: "));
//       // Serial.println(t);

//       Time = millis() - Time_Wifi_Zero;
//       Data_wifi = String(String(Time) + "," + String(h) + "," + String(t));
//       Serial.println(Data_wifi);
//       client.println(Data_wifi); // the main line to send data over wifi

//       // if (Touch_WIFI == false)
//       // {
//       //     client.stop();

//       //     break;
//       // }
//     }
//   }
// }