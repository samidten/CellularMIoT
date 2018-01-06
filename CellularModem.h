/*
  CellularModem.h - Generic Modem driver for Massive IoT developer kits
  Samid Tennakoon <samid.tennakoon@ericsson.com>
*/

#ifndef _CellularModem_H_
#define _CellularModem_H_

#include "Arduino.h"

#define DEBUG_PRINT(...)		_debugStream->print(__VA_ARGS__)
#define DEBUG_PRINTLN(...)		_debugStream->println(__VA_ARGS__)

#define DEBUG_STREAM Serial
#define MODEM_STREAM Serial1

#define DEBUG 2 // 1=level 1 (INFO), 2=level 2 (DETAIL)

enum eModemState
{
	FAILURE			= 2,
	SUCCESS			= 1,
	INIT			= 0,
	TIMED_OUT		= -1,
	BUSY	= -2,
  CUSTOM = -3
};

class CellularModem  {

private:
  String customString;

public:
  CellularModem();
  void init(const char*, const char*);
  virtual void on() {};
  virtual void test1() {};
  virtual void test2() {};
  void flushBuffer();
  String middle(String a, String b);
  String splitString(String data, char separator, int index);
  eModemState readBuffer(uint16_t timeout, const char* resp);
  eModemState writeData(uint16_t timeout, String data);
  eModemState writeData(uint16_t timeout, String data, const char* resp);

protected:
  bool _debug = false;
  const char* _apn;
  const char* _server;
  Stream* _debugStream;
  Stream* _modemStream;
  char replybuffer[255];
  char cmd[250];
  char resp[250];
  String modemInfo;
  String modemInfo1;
};

#endif
