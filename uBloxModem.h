/*
  uBloxModem.h - Modem driver for uBlox SARA N200 NB-IoT modem
  Tested with uBlox N200 firmware V100R100C10B656
  Samid Tennakoon <samid.tennakoon@ericsson.com>
*/

#ifndef _uBloxModem_H_
#define _uBloxModem_H_

#include "CellularModem.h"

class uBloxModem : public CellularModem  {
private:

public:
  uBloxModem();

  void on();
  void test1();
  void prepStage1();
  void prepStage2();
  void setup();
  void publish(char* data);
  void checkConnection();
  void restartWarm();

};

#endif
