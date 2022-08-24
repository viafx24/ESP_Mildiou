

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "DHT.h"

// parameter about Time_To_Send_Data: it is ABSOLUTE time.
unsigned long Next_Time;
int Day_To_Send = 0;
int Hour_To_Send = 1;
int Minute_To_Send = 15;
int Second_To_Send = 0;

//int Minute_Relative_To_Send = 2;

// pas encore compris pourquoi je dois initilialiser les fonctions ici.

void printLocalTime();
unsigned long Get_Epoch_Time();
unsigned long Set_Next_Time(int Day, int Hour, int Minute, int Second);
unsigned long Set_Next_Time_Relative(int Day, int Hour, int Minute_Relative, int Second);
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

boolean Data_Sent = false;

WiFiServer server(80);

// parameter for time with NTP

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
unsigned long Epoch_Time;

unsigned long Time_Limit;
int Time_To_Wait = 5;

long int t1;
long int t2;
// parameter light sleep

const long uS_TO_S_FACTOR = 1000000;  /* Conversion factor for micro seconds to seconds */
const int TIME_TO_SLEEP_DAY = 1 * 30; /* Time ESP32 will go to sleep (in seconds) */

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

  printLocalTime();
  Epoch_Time = Get_Epoch_Time();
  Serial.println(Epoch_Time);
  Next_Time = Set_Next_Time(Day_To_Send, Hour_To_Send, Minute_To_Send, Second_To_Send);
  // for relative in place of absolute
  //Next_Time = Set_Next_Time_Relative(Day_To_Send, Hour_To_Send, Minute_Relative_To_Send, Second_To_Send);
  Serial.println(Next_Time);
  delay(100);

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

  

  Data_wifi[it] = String(String(Epoch_Time) + "," + String(h) + "," + String(t));

  // FOR DEBUGGING
  printLocalTime();
  Serial.println(Data_wifi[it]);
  delay(100);

  it++;

  // FOR DEBUGGING
  //
  // delay(100);

  if (Epoch_Time > Next_Time)
  {
    printLocalTime();
    t1= millis();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      Serial.print(".");
    }
    Serial.println(" CONNECTED");
    printLocalTime();
    server.begin();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Epoch_Time = Get_Epoch_Time();
    //Serial.println(Epoch_Time);
    Time_Limit = Epoch_Time + Time_To_Wait;

    while (Get_Epoch_Time() < Time_Limit)
    {
      WiFiClient client = server.available();

      if (client)
      {
        Serial.println("Client connected");

        // while (client.connected())
        // {
          
        for (int it2 = 0; it2 < it; it2++)
        {
          //Serial.println(Data_wifi[it2]);
          
          client.println(Data_wifi[it2]);
          //delay(3);
          //delay(100);
        }
        // }
        
        delay(1000);
        // delay(it*5);
        // Serial.println(it);
        // Serial.println(it*5);
        
        it = 0;
        Data_Sent = true;
        Serial.println("Data Sent");
        client.stop();
        //printLocalTime();
        break;
      }
    }
    if (Data_Sent == false)
    {
      Serial.println("Time out: no client");
    }

    Next_Time =Set_Next_Time(Day_To_Send, Hour_To_Send, Minute_To_Send, Second_To_Send)  ;
    //Next_Time = Set_Next_Time_Relative(Day_To_Send, Hour_To_Send, Minute_Relative_To_Send, Second_To_Send);
    
    // DEBUG
    // Serial.println(Next_Time);
    // delay(100);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    t2= millis();
    Serial.println(t2-t1);
    Data_Sent = false;
    //printLocalTime();

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

unsigned long Set_Next_Time(int Day, int Hour, int Minute, int Second)
{
  time_t next_time;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return (0);
  }

  timeinfo.tm_hour = timeinfo.tm_hour + Hour;
  timeinfo.tm_min = Minute;
  timeinfo.tm_sec = Second;

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  next_time = mktime(&timeinfo);
  return next_time;
}

unsigned long Set_Next_Time_Relative(int Day, int Hour, int Minute_Relative, int Second)
{
  time_t next_time;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return (0);
  }

  timeinfo.tm_hour = timeinfo.tm_hour + Hour;
  timeinfo.tm_min = timeinfo.tm_min + Minute_Relative;
  timeinfo.tm_sec = Second;

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  next_time = mktime(&timeinfo);
  return next_time;
}
