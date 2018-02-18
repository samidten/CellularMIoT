/*
  SimcomModem.cpp - Modem driver for SIM7000C modem
  Tested with SIM7000C with 1351B01SIM7000 firmware (MDM9206_SIM7000C_P1.02_20170707)
  Samid Tennakoon <samid.tennakoon@ericsson.com>

  NOTE: This is still experimental code; use with care
  
*/

#include "SimcomModem.h"

SimcomModem::SimcomModem()
{

}

void SimcomModem::on()
{
  bool done = false;

  DEBUG_STREAM.begin(57600);
  MODEM_STREAM.begin(115200);
  while(!MODEM_STREAM.available());
  MODEM_STREAM.println("AT+IPR=57600");
  delay(1000);
  MODEM_STREAM.begin(57600);
  //while(DEBUG_STREAM.available());

  //restartWarm();
  setup();
  prepStage1();
}


// re-init the modem if something goes wrong
void SimcomModem::reinit()
{
    restartWarm();
    setup();
    prepStage1();
}

// fetch static attributes
void SimcomModem::prepStage1()
{
  bool done = false;

  while(!done) {
    if (writeData(1000, "AT+CGSN") != SUCCESS) break; // IMEI
    modemInfo1 = middle("AT+CGSN", "\r\nOK");
    modemInfo1 += ";";
    if (writeData(1000, "AT+CIMI") != SUCCESS) break; // IMSI
    modemInfo1 += middle("AT+CIMI", "\r\nOK");
    modemInfo1 += ";";
    modemInfo1 += "CAT-M1;";
    if (DEBUG==2) _debugStream->println(modemInfo1);
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during prepStage1"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
    modemInfo1 = "ERROR";
  }

}

// fetch semi-static attributes
void SimcomModem::prepStage2()
{
  bool done = false;
  String tmp;

  modemInfo = modemInfo1;

  while(!done) {
    if (writeData(5000, "AT+CSQ") != SUCCESS) break;
    // +CSQ: 26,99
    // rssi, ber
    // 0: -113 dBm or less
    // 1: -111 dBm
    // 2..30: from -109 to -53 dBm with 2 dBm steps
    // 31: -51 dBm or greater
    // 99: not known or not detectable or currently not available
    tmp = middle("+CSQ: ", "OK");
    tmp = splitString(tmp, ',', 0); // RSSI
    //_debugStream->println(tmp);
    int rssi = -113 + 2 * tmp.toInt(); // map CSQ to RSSI
    _debugStream->println(rssi);
    tmp = String(rssi);
    modemInfo += tmp;
    modemInfo += ";";
    if (DEBUG==2) _debugStream->println(modemInfo);
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during prepStage2"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
    reinit();
  }

}


void SimcomModem::setup()
{
  bool done = false;
  
  checkConnection2();

  while(!done) {
    if (writeData(1000, "AT+CIPSHUT") != SUCCESS) break;
    sprintf(cmd, "AT+CSTT=\"%s\"", _apn);
    if (writeData(1000, cmd) != SUCCESS) break;
    checkConnection();
    if (writeData(5000, "AT+CIICR") != SUCCESS) break;
    if (writeData(5000, "AT+CIFSR") == FAILURE) break;
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during setup"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
    reinit();
  }
}

void SimcomModem::publish(char* data)
{
  bool done = false;
  char tmp[255];
  char buf[255];
  size_t len;

 prepStage2();  
 modemInfo.toCharArray(tmp, modemInfo.length());
 sprintf(tmp, "%s;%s", tmp, data);
 len = strlen(tmp);
 //_debugStream->println("!!!!!!!!!!!!!!");
 _debugStream->println(tmp);
 //_debugStream->println("!!!!!!!!!!!!!!");

 sprintf(buf, "AT+CIPSTART=\"UDP\",\"%s\",5121", _server);
 //_debugStream->println(tmp);

  while(!done) {
    if (writeData(3000, buf) != SUCCESS) break;
     sprintf(buf, "AT+CIPSEND=%d", len);
    if (writeData(1000, buf, ">") != CUSTOM) break;
    if (writeData(1000, tmp, "SEND OK") != CUSTOM) break;
    if (writeData(1000, "AT+CIPCLOSE", "CLOSE OK") != CUSTOM) break;
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during publish"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
    writeData(1000, "AT+CIPCLOSE", "CLOSE OK"); // socket cleanup
    //reinit();
  }
}

// check for attach status
// TODO: maybe check IP as well?
void SimcomModem::checkConnection()
{
  int retry = 12; // lets try 1 min to attach before bailing
  while(retry--) {
    writeData(1000, "AT+CSQ");
    if (writeData(1000, "AT+CGATT?", "+CGATT: 1\r\n") == CUSTOM) break;
    if (retry == 0) break;
    delay(5000);
  }
  if (retry == 0 && DEBUG == 1) _debugStream->println("ERROR: Something wrong with connection");
}


// check for signal level
void SimcomModem::checkConnection2()
{
  int retry = 6; // lets try 1 min to attach before bailing
  while(retry--) {
    if (writeData(1000, "AT+CSQ", "+CSQ: 99,99") != CUSTOM) break;
    //if (writeData(1000, "AT+CGATT?", "+CGATT: 1\r\n") == CUSTOM) break;
    if (retry == 0) break;
    delay(5000);
  }
  if (retry == 0 && DEBUG == 1) _debugStream->println("ERROR: Something wrong with connection2");
}

//perform hardware reset
void SimcomModem::restartWarm()
{

  MODEM_STREAM.println("AT+CFUN=1,1");
  delay(5000);   // takes ~5 seconds to restart this 
  MODEM_STREAM.begin(115200);
  while(!MODEM_STREAM.available());
  MODEM_STREAM.println("AT+IPR=57600");
  delay(1000);
  MODEM_STREAM.begin(57600);   

  if(writeData(10000, "AT") == SUCCESS) {
    if (DEBUG>=1) _debugStream->println("Restart OK!");
  }
  else {
    if (DEBUG>=1) _debugStream->println("ERROR: Restart failed!");
  }

}
