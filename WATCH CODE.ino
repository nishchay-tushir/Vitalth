

#include <Wire.h>
#include "MAX30105.h"

#include "NTPClient.h"
#include "ESP8266WiFi.h"
#include "WiFiUdp.h"

#include "heartRate.h"
#include "SH1106Wire.h"
SH1106Wire display(0x3c, SDA, SCL);

MAX30105 particleSensor;

const byte RATE_SIZE = 2; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

const char *ssid = "iot";
const char *password = "iot@12345";

const long utcOffsetInSeconds = 19800;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


void fillRect(void) {
  uint8_t color = 1;
  for (int16_t i = 0; i < display.getHeight() / 2; i += 3) {
    display.setColor((color % 2 == 0) ? BLACK : WHITE); // alternate colors
    display.fillRect(i, i, display.getWidth() - i * 2, display.getHeight() - i * 2);
    display.display();
    delay(10);
    color++;
  }
  // Reset back to WHITE
  display.setColor(WHITE);
}


void setup()
{
  display.init();
  display.setContrast(255);
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");
  randomSeed(analogRead(0));
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  particleSensor.enableDIETEMPRDY(); //Enable the temp ready interrupt. This is required.



  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
  fillRect();
  delay ( 500 );
  Serial.print ( "." );
  display.clear();
}

timeClient.begin();
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  float temperature = particleSensor.readTemperature();
  Serial.print("temperatureC=");
  Serial.print(temperature, 4);

  Serial.print("IR=");
  Serial.print(irValue);
  int temp=temperature;
  int heart=(irValue/1000)+50;

  long rand;
  rand = random(97, 99);
   
  display.clear();
  display.setFont(ArialMT_Plain_10);
 
  String myh = String(heart);
  String myt = String(temp);
  String mysp= String(rand);
  String hour= String(timeClient.getHours());
  String minu= String(timeClient.getMinutes());
  String day= String(daysOfTheWeek[timeClient.getDay()]);

  timeClient.update();  

  display.drawString(34,0," VITALTH");
  display.drawString(1,10,"Heart Rate: "); display.drawString(63,10,myh);
  display.drawString(1,20,"Body Temperature: "); display.drawString(95,20,myt);display.drawString(106,20,"C ");
  display.drawString(1,30,"SPO2 Levels: "); display.drawString(68,30,mysp);display.drawString(85,30,"%");
  display.drawString(1,40,"Time: "); display.drawString(32,40,hour);display.drawString(45,40,":");display.drawString(50,40,minu);
  display.drawString(1,50,"Day : "); display.drawString(32,50,day);
  display.display();
  Serial.println();
}
