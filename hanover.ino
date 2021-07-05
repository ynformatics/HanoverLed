#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <SPI.h> 

#include "Z:\secure\include\Credentials.h" // WIFI_SSID_2, WIFI_PASSWORD_2

// using a Wemos D1 Mini (ESP8266)
// MOSI is D7
// MISO is D6 (unused)
// SCK  is D5
const int MS =  D1; //meta CS
const int DS =  D2; //pixel CS
const int EOT = D3; //EOT
const int BRIGHT = D4; //Brightness

WiFiClient espClient;
PubSubClient client(espClient);
char sendBuffer[256];
byte display[64];

void setup() 
{
  pinMode(MS, OUTPUT);
  pinMode(DS, OUTPUT);
  pinMode(EOT, OUTPUT);
  pinMode(BRIGHT, OUTPUT);
  
  digitalWrite(MS, LOW);
  digitalWrite(DS, HIGH);
  digitalWrite(EOT, LOW);
  analogWrite(BRIGHT, 0);
  
  Serial.begin(9600);
  SPI.begin();
  WiFi.begin(WIFI_SSID_2, WIFI_PASSWORD_2 );
 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
  }
 
  client.setServer(MQTT_SERVER_IP, 1883);
  client.setCallback(MqttCallback);
 
  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("HanoverLed" )) 
    {
      MqttDebug("connected");   
    } 
    else 
    { 
      Serial.print("failed with state: ");
      Serial.println(client.state());
      delay(500); 
    }
  }

  MqttDebug("hanover led IP: %s", WiFi.localIP().toString().c_str());
 
  client.subscribe("hanoverled/display"); 
  client.subscribe("hanoverled/brightness"); 

  InitDisplay();
}

void MqttDebug( const char* fmt, ...)
{
   va_list args;         
   va_start(args, fmt); 

   vsnprintf(sendBuffer, sizeof(sendBuffer),fmt, args);
   client.publish("hanoverled/debug", sendBuffer);
}

void MqttCallback(char* topic, byte* payload, unsigned int length) 
{
  if(strcmp(topic, "hanoverled/display") == 0)
  { 
    int maxlen = length > 64 ? 64 : length;
   
    for (int i = 0; i < maxlen; i++) 
    {
      display[i] = payload[i];
    } 
    for (int i = maxlen; i < 64; i++) 
    {
      display[i] = 0;
    } 

    UpdateDisplay(display);
  }
  else if(strcmp(topic, "hanoverled/brightness") == 0)
  { 
    char buf[length+1];
 
    for (int i = 0; i < length; i++) 
    {
       buf[i] = payload[i];
    }
    buf[length] = '\0';

    int bright = atoi(buf);
   
    analogWrite(BRIGHT, bright);
  }
}

void loop() 
{
   if(!client.loop())
   {
      ESP.restart();
   }
}

void InitDisplay()
{
  byte data[64] = {0};
  data[0] = 0x80;
  UpdateDisplay(data);
}

void UpdateDisplay(byte data[64])
{
  SPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE2));

  // meta data row 1
  digitalWrite(MS, HIGH);
  SPI.transfer(0x3E);
  digitalWrite(MS, LOW);

  // pixel data row 1
  for(int i = 0; i < 32; i++)
  {
    digitalWrite(DS, LOW);
    SPI.transfer(data[i]);
    digitalWrite(DS, HIGH);
  }

  // meta data row 2
  digitalWrite(MS, HIGH);
  SPI.transfer(0xBE);
  digitalWrite(MS, LOW);

  // pixel data row 2
  for(int i = 32; i < 64; i++)
  {
    digitalWrite(DS, LOW);
    SPI.transfer(data[i]);
    digitalWrite(DS, HIGH);
  }
  
  SPI.endTransaction();
   
  // pulse EOT line 
  delay(15);
  digitalWrite(EOT, HIGH);
  digitalWrite(EOT, LOW);
}
