/*
  GemaltoModem.h - Modem driver for Gemalto ELS61-E CAT-1 modem
  Tested with Cinterion ELS61-E REVISION 01.000
  Samid Tennakoon <samid.tennakoon@ericsson.com>
*/

#ifndef _GemaltoModem_H_
#define _GemaltoModem_H_

#include "CellularModem.h"

class GemaltoModem : public CellularModem  {
private:

public:
  GemaltoModem();

  void on();
  void off();
  void reinit();
  void prepStage1();
  void prepStage2();
  void setup();
  void checkConnection();
  void publish(char* data);
  void test1();
  void restartWarm();
  void restartCold();
};

#endif
