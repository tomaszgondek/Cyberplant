//INCLUDES
#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <WiFiClientSecure.h>
#include "PubSubClient.h"

//PINDEF
#define ON_LED D1
#define SIG_LED D2
#define ERR_LED D3
#define RELAY D6
#define READ_PIN A0

//WIFI DATA
const char* ssid     = "***";
const char* password = "***";

//MQTT DATA
const char* mqtt_server = "re97f066.ala.eu-central-1.emqxsl.com";
const char* mqtt_username = "***";
const char* mqtt_password = "***";
const int mqtt_port = 8883;
String msgbuffer = "";

//VARIABLES
int reading[10];
float meanReading = 0;
const long int interval = 86400000;   //24H
const long int interval_m = 1800000;  //0.5H
unsigned long previousMillis = 86500000;
unsigned long previousMillis2 = 1810000;
float moisture = 0;
bool override = false;

//MALFUNCTION LOGIC
bool M1, M2, M3 = false;
bool globalError = false;

//MQTT SETUP
WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

//FUNCTIONS
void wifi_setup()
{
  WiFi.mode(WIFI_STA);
  if(!WiFi.begin(ssid, password))
  {
    M2 = true;
    Serial.println("WiFi initialisation has failed!");
  }
  else 
  {
    Serial.print("WiFi setup \nConnecting to: ");
    Serial.print(ssid);
    Serial.print("\n");
    while(WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("\nConnection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  }
  randomSeed(micros());
}

void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message published ["+String(topic)+"]: "+payload);
}

float takeRead()
{
  float temp = 0;
  Serial.println("Taking soil moisture measurments: ");
  for(int i = 0; i < 10; i++)
  {
    digitalWrite(SIG_LED, HIGH);
    reading[i] = analogRead(READ_PIN);
    Serial.printf("Reading %i: %i \n", i+1, reading[i]);
    delay(750);
    digitalWrite(SIG_LED, LOW);
    delay(250);
  }
  for(int i = 0; i < 10; i++)
  {
    temp += reading[i];
  }
  meanReading = temp / 10;
  Serial.printf("Final mean reading: %f\n", meanReading);
  return meanReading;
}

void pump_action(float mp)
{
  Serial.printf("\nMoisture in %%: %f\n", mp);
  if(mp < 45 || override)
  {
    Serial.println("Pump on");
    digitalWrite(RELAY, HIGH);
    digitalWrite(SIG_LED, HIGH);
    delay(3000);
    digitalWrite(RELAY, LOW);
    digitalWrite(SIG_LED, LOW);
    Serial.println("Pump off");
  }
  else
  {
    Serial.println("Moisture ok.");
  }
  if(override)
  {
    Serial.println("Watered with MQTT override");
    override = false;
    Serial.print("Current override state: ");
    Serial.print(override);
    Serial.print("\n");
  } 
  if(mp < 0 || mp > 100)
  {
    M1 = true;
  }
}

void malfunction_handler()
{
  Serial.println("Malfunctions occured during operation: ");
  if(M1) Serial.println("MALFUNCTION #1 - impossible reading");
  if(M2) Serial.println("MALFUNCTION #2 - WiFi has not initialised properly");
  Serial.println("---");
}

void reconnect() 
{
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "CYBERPLANT-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) 
    {
      Serial.println("connected");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  msgbuffer = "";
  Serial.print("Message arrived at [");
  Serial.print(topic);
  Serial.print("], payload: ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    msgbuffer += (char)payload[i];
  }
  if(msgbuffer == "ON")
  {
    override = true;
    pump_action(moisture);
  }
  Serial.println();
}

void setup() 
{
  delay(3000);
  Serial.println();
  Serial.begin(115200);
  Serial.println("\nStarting the device...");
  wifi_setup();
  espClient.setInsecure();
  pinMode(ON_LED, OUTPUT);
  pinMode(SIG_LED, OUTPUT);
  pinMode(ERR_LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(READ_PIN, INPUT);
  digitalWrite(ON_LED, HIGH);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  if(!client.connected()) reconnect();
  if(client.subscribe("CYBERPLANT/DEBUG/WATERING"))
  {
    Serial.println("Subscribed succsefully to:\n[CYBERPLANT/DEBUG/WATERING]");
  }
  else
  {
    Serial.println("Subscriptions failed");
  }
  Serial.println("Startup completed!");
}

void loop() 
{
  if(!client.connected()) reconnect();
  client.loop();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis2 >= interval_m)
  {
    moisture = map(takeRead(), 255, 700, 100, 0);
    char mqtt_message[128];
    sprintf(mqtt_message, "%f", moisture);
    publishMessage("CYBERPLANT/HUMIDITY", mqtt_message, true);
    malfunction_handler();
    previousMillis2 = currentMillis;
  }
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    pump_action(moisture);
  }
}
