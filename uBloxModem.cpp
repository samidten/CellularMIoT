/*
  uBloxModem.cpp - Modem driver for uBlox SARA N200 NB-IoT modem
  Tested with uBlox N200 firmware V100R100C10B657SP2
  Samid Tennakoon <samid.tennakoon@ericsson.com>

  TODO:
    - During any of the stages if network fails go into recovery mode where we try to reconnect to the network and restart the state machine
      In recovery mode we should only retry in exponentional interval
    - Enable downlink messaging
    - Add sending cell id as metadata
    - Add payload encryption
    - Optimize code for smaller footprint (ie: Adafruit Feather?)
*/

#include "uBloxModem.h"

uBloxModem::uBloxModem()
{

}

void uBloxModem::on()
{
  bool done = false;

  DEBUG_STREAM.begin(9600);
  MODEM_STREAM.begin(9600);

  pinMode(powerPin, OUTPUT); // POWER PIN
  digitalWrite(powerPin, HIGH);

  writeData(1000, "AT+CPSMS=0"); // disable PSM for now 
  writeData(1000, "AT+CEDRXS=0,5,\"0101\""); // disable eDRX for now
  writeData(1000, "AT+NPSMR=1"); // enable PSM status reporting

  while(!done) {
    if (writeData(1000, "AT+NCONFIG=\"AUTOCONNECT\",\"TRUE\"") != SUCCESS) break;
    if (writeData(1000, "AT+NCONFIG=\"CR_0354_0338_SCRAMBLING\",\"TRUE\"") != SUCCESS) break;
    if (writeData(1000, "AT+NCONFIG=\"CR_0859_SI_AVOID\",\"TRUE\"") != SUCCESS) break;
    if (writeData(1000, "AT+NCONFIG=\"COMBINE_ATTACH\",\"TRUE\"") != SUCCESS) break;
    if (writeData(1000, "AT+NCONFIG=\"CELL_RESELECTION\",\"TRUE\"") != SUCCESS) break;
    if (writeData(1000, "AT+NCONFIG=\"ENABLE_BIP\",\"FALSE\"") != SUCCESS) break;
    done = true;
  }
  if (!done) {
    if (DEBUG==1) _debugStream->println(F("ERROR: Something went wrong during on"));
    if (DEBUG==1) _debugStream->println(replybuffer);
  }
  restartWarm();
  setup();
  prepStage1();
}


// fetch static attributes
void uBloxModem::prepStage1()
{
  bool done = false;

  while(!done) {
    if (writeData(1000, "AT+CGSN=1") != SUCCESS) break; // IMEI
    modemInfo1 = middle("+CGSN: ", "OK");
    modemInfo1 += ";";
    if (writeData(1000, "AT+CIMI") != SUCCESS) break; // IMSI
    modemInfo1 += middle("\n", "OK");
    modemInfo1 += ";";
    modemInfo1 += "NB-IoT;";
    if (DEBUG==2) _debugStream->println(modemInfo1);
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during prepStage1"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
  }

}

// fetch semi-static attributes in attach mode
void uBloxModem::prepStage2()
{
  bool done = false;
  String tmp;

  modemInfo = modemInfo1;

  while(!done) {
    if (writeData(1000, "AT+CSQ") != SUCCESS) break;
    // +CSQ: 26,99
    // rssi, ber
    // 0: -113 dBm or less
    // 1: -111 dBm
    // 2..30: from -109 to -53 dBm with 2 dBm steps
    // 31: -51 dBm or greater
    // 99: not known or not detectable or currently not available
    tmp = middle("+CSQ:", "OK");
    tmp = splitString(tmp, ',', 0); // RSSI
    //_debugStream->println(tmp);
 //   if (tmp.toInt() == 99) {
 //     restartWarm();
  //  }
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
  }

}

void uBloxModem::test1()
{

  if (writeData(2000, "AT+CSQ") == SUCCESS) {
    _debugStream->println("### AT is OK!");
    _debugStream->println(replybuffer);
  }
  else {
    _debugStream->println("### AT is NOK!");
  }
}


void uBloxModem::setup()
{
  bool done = false;

  while(!done) {
    if (writeData(5000, "AT+CFUN=0") != SUCCESS) break;
    delay(1000);
    sprintf(cmd, "AT+CGDCONT=0,\"IP\",\"%s\"", _apn);
    if (writeData(1000, cmd) != SUCCESS) break;
//    if (writeData(5000, "AT+CFUN=1") != SUCCESS) break;
    done = true;
  }
  if (!done) {
    if (DEBUG>=1) _debugStream->println(F("ERROR: Something went wrong during setup"));
    if (DEBUG>=1) _debugStream->println(replybuffer);
      restartWarm();
  }
  else {
    //restartWarm();
    //delay(2000); // give some space for modem setup to settle
    checkConnection();
  }
}

void uBloxModem::publish(char* data)
{
  bool done = false;
  char tmp[255];
  char buf[255];

 prepStage2();
 modemInfo.toCharArray(tmp, modemInfo.length());
 sprintf(tmp, "%s;%s", tmp, data);
 _debugStream->println(tmp);
 size_t len = strlen(tmp);

 for (size_t i = 0; i < len; i++) {
   sprintf(buf + i * 2, "%02X", tmp[i]);
 }

 len = strlen(buf);
 sprintf(tmp, "AT+NSOST=0,\"%s\",5121,%d,\"%s\"", _server, len / 2, buf);
 //_debugStream->println(tmp);

  while(!done) {
    //AT+NSOCR=DGRAM,17,42000,1
    //AT+NSOCL=0
    //AT+NSOST=0,10.1.1.1,5121,4,ABCDEF
    if (writeData(1000, "AT+NSOCR=\"DGRAM\",17,42000,1") != SUCCESS) break;
    if (writeData(3000, tmp) != SUCCESS) break;
    if (writeData(1000, "AT+NSOCL=0") != SUCCESS) break;
    done = true;
  }
  if (!done) {
    if (DEBUG==1) _debugStream->println(F("ERROR: Something went wrong during publish"));
    if (DEBUG==1) _debugStream->println(replybuffer);
    restartWarm();
    setup();
  }
}

// check for attach status
// TODO: maybe check IP as well?
void uBloxModem::checkConnection()
{
  digitalWrite(LED_BUILTIN, HIGH);
  int retry = 10; // lets try 1min to attach before bailing
  while(retry--) {
    writeData(1000, "AT+CSQ");
    if (writeData(1000, "AT+CGATT?", "+CGATT: 1\r\n") == CUSTOM) break;
    if (retry == 0) break;
    delay(5000);
  }
  if (retry == 0 && DEBUG >= 1) {
    _debugStream->println("ERROR: Something wrong with connection");
    restartWarm();
    setup();
  }
  digitalWrite(LED_BUILTIN, LOW);
  delay(100); // let the serial settle
  
  //writeData(1000, "AT+CPSMS=1,,,\"10100101\",\"00000001\""); // enable PSM; active period only 1s

}

void uBloxModem::restartWarm()
{
  //writeData(5000, "AT+CFUN=0");
  //writeData(5000, "AT+CFUN=1");
   // takes ~5 seconds to restart this modem
  if(writeData(10000, "AT+NRB") == SUCCESS) {
    if (DEBUG>=1) _debugStream->println("Restart OK!");
  }
  else {
    if (DEBUG>=1) _debugStream->println("ERROR: Restart failed!");
  }

}
