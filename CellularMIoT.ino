/*
  main.ino - Example app for Massive IoT
  Samid Tennakoon <samid.tennakoon@ericsson.com>
  Updated: 18 Jan 2018
*/

//#include "GemaltoModem.h"
#include "uBloxModem.h"
//#include "SimcomModem.h"
#include <Wire.h>
#include <Sodaq_HTS221.h>

//GemaltoModem modem;
uBloxModem modem;
//SimcomModem modem;

//lab SIM = "stm-iot", production SIM = "m2minternet"
//char apn[] = "m2minternet";
//char apn[] = "stm-iot";
char apn[] = "hicard";
char server[] = "203.126.155.248"; // iotlab.zmalloc.org

int counter = 0;
Sodaq_HTS221 s1;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin();
  if (s1.init()) { // enable HTS221 sensor (temp/humidity)
    s1.enableSensor();
  }
  modem.init(apn, server);
}

void loop() {
  char v1[15];
 
  counter = counter + 1;
  sprintf(v1, "%d;%f;%f", counter, s1.readHumidity(), s1.readTemperature()); // combine var1, var2, etc to a string
  digitalWrite(LED_BUILTIN, HIGH);
  modem.publish(v1);
  digitalWrite(LED_BUILTIN, LOW);
  delay(30000);
}


