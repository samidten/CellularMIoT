/*
  CellularMIoT.ino - Example app for Massive IoT
  Samid Tennakoon <samid.tennakoon@ericsson.com>
  Updated: 26 Sep 2017
*/

//#include "GemaltoModem.h"
//#include "uBloxModem.h"
#include "SimcomModem.h"
//#include <SimpleDHT.h>

//GemaltoModem modem;
//uBloxModem modem;
SimcomModem modem;

//lab SIM = "stm-iot", production SIM = "m2minternet"
//char apn[] = "m2minternet";
//char apn[] = "e-ideas";
char apn[] = "hicard";
char server[] = "203.126.155.248"; // iotlab.zmalloc.org
int once = 1;
int counter = 0;

//const int trigPin = 8;
//const int echoPin = 9;
//int pinDHT22 = 8;
//SimpleDHT22 dht22;

void setup() {
  modem.init(apn, server);
  //Serial.begin(9600);
}

void loop() {
  char v1[10];
  byte temperature;
  byte humidity;
  int err;
  //if (!once) modem.reinit();

  //  err = dht22.read(pinDHT22, &temperature, &humidity, NULL);
  //  sprintf(v1, "%d", temperature); // combine var1, var2, etc to a string
  //Serial.println(v1);



  //float value = analogRead(A0);
  //int a1 = (int) (value / 1023*200); // scale the sensor reading 0 - 100
  counter = counter + 1;
  sprintf(v1, "%d", counter); // combine var1, var2, etc to a string


  /*
    long duration;
    int distance;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    distance= duration*0.034/2;
    sprintf(v1, "%d", 27);
  */

  modem.publish(v1);

  //modem.off();
  delay(2000);
  once = 0;

}
