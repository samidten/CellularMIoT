/*
  GemaltoModem.cpp - Modem driver for Gemalto ELS61-E CAT-1 modem
  Samid Tennakoon <samid.tennakoon@ericsson.com>
*/

#include "GemaltoModem.h"

GemaltoModem::GemaltoModem()
{

}

void GemaltoModem::on()
{
  DEBUG_STREAM.begin(115200);
  MODEM_STREAM.begin(115200);
  while(DEBUG_STREAM.available());

  // keep the module ON state
  pinMode(9, INPUT); // POWER PIN (needs to be low impedance to power on)
  pinMode(2, INPUT_PULLUP); // RTS
  pinMode(3, OUTPUT); // DTS

  // TODO: force band and RAT before restart

  restartCold();
  setup();
  prepStage1();
}

void GemaltoModem::off()
{
  // quick shutdown
  if (writeData(5000, "AT^SMSO", "^SHUTDOWN") == CUSTOM) {
    if (DEBUG>=1) _debugStream->println(F("Shutdown Successful!"));
  }
  else {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Shutdown failed!"));
  }
}

void GemaltoModem::reinit()
{
  restartCold();
  setup();
}



// fetch static attributes
void GemaltoModem::prepStage1()
{
  bool done = false;

  while(!done) {
    if (writeData(1000, "AT+CGSN") != SUCCESS) break; // IMEI
    modemInfo1 = middle("AT+CGSN", "OK");
    modemInfo1 += ";";
    if (writeData(1000, "AT+CIMI") != SUCCESS) break; // IMSI
    modemInfo1 += middle("AT+CIMI", "OK");
    modemInfo1 += ";";
    modemInfo1 += "CAT-1;";
    if (DEBUG==2) _debugStream->println(modemInfo1);
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during prepStage1"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
  }

}

// fetch semi-static attributes
void GemaltoModem::prepStage2()
{
  bool done = false;
  String tmp;

  modemInfo = modemInfo1;

  while(!done) {
    if (writeData(1000, "AT^SMONI") != SUCCESS) break;
    tmp = middle("AT^SMONI", "OK");
    // ^SMONI: 4G,3725,8,5,5,FDD,525,01,02C5,B5F9309,396,37,-82,-12.5,NOCONN
    // RAT,EARFCN,BAND,DL_BW,UL_BW,MCC,MNC,TAC,Global_Cell_Id,Phy_Cell_Id,Srxlev,RSRP,RSRQ,Conn_State
    // We want: Global_Cell_Id, RSRP, RSRQ
    modemInfo += splitString(tmp, ',', 9); // Global_Cell_Id
    modemInfo += ";";
    modemInfo += splitString(tmp, ',', 12); // RSRP
    modemInfo += ";";
    modemInfo += splitString(tmp, ',', 13); // RSRQ
    modemInfo += ";";
    if (writeData(1000, "AT^SBV") != SUCCESS) break;
    modemInfo += middle("^SBV: ", "OK"); // Power reading mV
    modemInfo += ";";
    if (DEBUG==2) _debugStream->println(modemInfo);
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during prepStage2"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
  }

}

void GemaltoModem::setup()
{
  bool done = false;
  checkConnection();
  delay(2000); // give some space for modem setup to settle

  while(!done) {
    if (writeData(1000, "AT^SICS=0,\"conType\",\"GPRS0\"") != SUCCESS) break;
    sprintf(cmd, "AT^SICS=0,\"apn\",\"%s\"", _apn);
    if (writeData(1000, cmd) != SUCCESS) break;
    if (writeData(1000, "AT^SISS=0,\"srvType\",\"Socket\"") != SUCCESS) break;
    if (writeData(1000, "AT^SISS=0,\"conId\",0") != SUCCESS) break;
    sprintf(cmd, "AT^SISS=0,\"address\",\"sockudp://%s:5121\"", _server);
    if (writeData(1000, cmd) != SUCCESS) break;
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during setup"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
  }
}

// this is connectionless (UDP) transaction. we do not expect an answer from the server
void GemaltoModem::publish(char* data)
{
  bool done = false;
  char buf[255];


  prepStage2();
  modemInfo.toCharArray(buf, modemInfo.length());
  sprintf(buf, "%s;%s", buf, data);

  size_t len = strlen(buf);

  sprintf(cmd, "AT^SISW=0,%d", len);
  sprintf(resp, "^SISW: 0,%d,0", len);

  while(!done) {
    writeData(2000, "AT^SISC=0");
    if (writeData(5000, "AT^SISO=0", "^SISW: 0,1") != CUSTOM) break;
    if (writeData(5000, cmd, resp) != CUSTOM) break;
    writeData(2000, buf);
    if (writeData(2000, "AT^SISC=0") != SUCCESS) break;
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during publish"));
  }
}

void GemaltoModem::test1()
{
  if (writeData(2000, "AT^SMONI", "4G") == CUSTOM) {
    _debugStream->println("### AT is OK!");
    _debugStream->println(replybuffer);
  }
  else {
    _debugStream->println("### AT is NOK!");
  }
}

// check for attach status
// TODO: maybe check IP as well?
void GemaltoModem::checkConnection() {
  int retry = 12; // lets try 1 min to attach before bailing
  while(retry--) {
    if (writeData(1000, "AT+CGATT?", "+CGATT: 1\r\n") == CUSTOM) break;
    if (retry == 0) break;
    delay(5000);
  }
  if (retry == 0 && DEBUG == 1) _debugStream->println("ERROR: Something wrong with connection");
}

void GemaltoModem::restartWarm()
{
  writeData(1000, "AT+CFUN=1,1"); // takes ~32 seconds to restart this modem
  if(readBuffer(30000, "+PBREADY\r\n") == CUSTOM) {
    if (DEBUG>=1) _debugStream->println("Restart OK!");
  }
  else {
    if (DEBUG>=1) _debugStream->println("ERROR: Restart failed!");
  }

}

void GemaltoModem::restartCold()
{
  if (DEBUG>=1) _debugStream->println(F("Restarting Cold.."));
  // workaround to restart the modem; not sure what Gemalto done here....
  pinMode(9,OUTPUT);
  delay(3000);
  pinMode(9, INPUT);
  delay(1000);
  pinMode(9, OUTPUT);
  delay(500);
  pinMode(9, INPUT);
  delay(1000);
  if(readBuffer(30000, "+PBREADY\r\n") == CUSTOM) {
    if (DEBUG>=1) _debugStream->println("Restart OK!");
  }
  else {
    if (DEBUG>=1) _debugStream->println("ERROR: Restart failed!");
  }

}
