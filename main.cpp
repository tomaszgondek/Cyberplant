#include <Arduino.h>

//PINDEF
#define ON_LED D1
#define SIG_LED D2
#define ERR_LED D3
#define RELAY D6
#define READ_PIN A0

int reading[10];
float meanReading = 0;
const long int interval = 86400000; 
unsigned long previousMillis = 0;

bool M1 = false;
bool globalError = false;
bool firststart = true;
float takeRead()
{
  float temp = 0;
  Serial.println("Taking measurments: ");
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

void setup() 
{
  delay(500);
  Serial.begin(115200);
  Serial.println("Starting the device...");
  pinMode(ON_LED, OUTPUT);
  pinMode(SIG_LED, OUTPUT);
  pinMode(ERR_LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(READ_PIN, INPUT);
  digitalWrite(ON_LED, HIGH);
  Serial.println("Startup completed!");
}

void loop() 
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval || firststart == true)
  {
    previousMillis = currentMillis;
    float moisture = map(takeRead(), 255, 700, 100, 0);
    Serial.printf("Moisture in %%: %f\n", moisture);
    if(moisture < 45 || firststart == true)
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
    if(moisture < 0 || moisture > 100)
    {
      M1 = true;
    }
    Serial.println("Malfunctions occured during operation: ");
    if(M1) Serial.println("MALFUNCTION #1 - impossible reading");
    Serial.println("Refer to the guide if any");
    if(firststart == true) firststart = false;
  }
  if(M1 == true)
  {
    globalError = true;
  }
  if(globalError == true) digitalWrite(ERR_LED, HIGH);
  else digitalWrite(ERR_LED, LOW);
}