/*
  CellularModem.cpp - Modem driver for Massive IoT developer kits
  Samid Tennakoon <samid.tennakoon@ericsson.com>
*/

#include "CellularModem.h"


// Constructor
CellularModem::CellularModem()
{
}

void CellularModem::init(const char* apn, const char* server)
{
   _debugStream = &DEBUG_STREAM;
   _modemStream = &MODEM_STREAM;
   _apn = apn;
   _server = server;

   on();
}

String CellularModem::middle(String a, String b)
{
  String str(replybuffer);

  int locStart = str.indexOf(a);
  if (locStart==-1) return "-";
  locStart += a.length();
  int locFinish = str.indexOf(b, locStart);
  if (locFinish==-1) return "+";
  str = str.substring(locStart, locFinish);
  str.trim();
  return str;
}

String CellularModem::splitString(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void CellularModem::flushBuffer()
{
  while(_modemStream->available()) {
    _modemStream->read();
  }
}

eModemState CellularModem::writeData(uint16_t timeout, String data)
{
  char dummy[] = ""; // custom resposne not expected
  return writeData(timeout, data, dummy);
}

eModemState CellularModem::writeData(uint16_t timeout, String data, const char* resp)
{
  if (DEBUG>=1) _debugStream->println(data);
  flushBuffer(); // clear the modem buffers before we start issuing commands
  _modemStream->println(data);
  return readBuffer(timeout, resp);
}


eModemState CellularModem::readBuffer(uint16_t timeout, const char* resp)
{
  uint16_t idx = 0;
  eModemState ret = INIT;
  replybuffer[0] = '\0';

  while (timeout--) {
    if (idx >= 254) {  // if string exceeds buffer size
      break;
    }
    while (_modemStream->available()) {
      char c =  _modemStream->read();
      //DEBUG_PRINT(c);
      if (c == '\0') continue; // ignore nulls incase modem spitting garbage chars on reboot
      replybuffer[idx] = c;
      idx++;
    }

    replybuffer[idx] = '\0'; // insert dummy null char before we start comparing


    if (resp[0] == '\0' && strstr(replybuffer, "OK\r\n")) {
      if (DEBUG==2) DEBUG_PRINTLN(F("RECVD OK!"));
      if (DEBUG==2) DEBUG_PRINTLN(replybuffer);
      if (DEBUG==2) DEBUG_PRINTLN("-----");
      ret = SUCCESS;
      break;
    }
    else if(resp[0] != '\0' &&strstr(replybuffer, resp)) {
      if (DEBUG==2) DEBUG_PRINTLN(F("RECVD CUSTOM!"));
      if (DEBUG==2) DEBUG_PRINTLN(replybuffer);
      if (DEBUG==2) DEBUG_PRINTLN("-----");
      ret = CUSTOM;
      break;
    }
    else if (strstr(replybuffer, "ERROR\r\n")) {
      if (DEBUG==2) DEBUG_PRINTLN(F("RECVD ERROR!"));
      if (DEBUG==2) DEBUG_PRINTLN(replybuffer);
      if (DEBUG==2) DEBUG_PRINTLN("-----");
      ret = FAILURE;
      break;
    }
    else if (timeout == 0) {
      if (DEBUG==2) DEBUG_PRINTLN(F("TIMEOUT"));
      if (DEBUG==2) DEBUG_PRINTLN(replybuffer);
      if (DEBUG==2) DEBUG_PRINTLN("-----");
      ret = TIMED_OUT;
      break;
    }
    delay(1);
  }
  return ret;
}
