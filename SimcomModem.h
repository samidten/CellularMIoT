/*
  SimcomModem.h - Modem driver for SIM7000C modem
  Tested with SIM7000C with 1351B01SIM7000 firmware (MDM9206_SIM7000C_P1.02_20170707)
  Samid Tennakoon <samid.tennakoon@ericsson.com>

*/


#ifndef _SimcomModem_H_
#define _SimcomModem_H_

#include "CellularModem.h"

class SimcomModem : public CellularModem  {
private:

public:
  SimcomModem();

  void on();
  void prepStage1();
  void prepStage2();
  void setup();
  void publish(char* data);
  void checkConnection();
  void checkConnection2();
  void restartWarm();
  void reinit();

};

#endif
