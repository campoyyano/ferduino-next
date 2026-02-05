#include <Arduino.h>
#include "hal_time.h"
#include "hal_ioexpander.h"
#include "board_hw_config.h"

static hal::IoExpander iox(FERDUINO_PCF8575_ADDR);

static void apagarP0aP12() {
  for (uint8_t p = 0; p <= 12; p++) {
    iox.writePin(p, false);
  }
}

static void activarSolo(uint8_t p) {
  apagarP0aP12();
  iox.writePin(p, true);
}

void app_ioexpander_smoketest_run() {
  static bool init = false;
  if (!init) {
    iox.begin();
    apagarP0aP12();
    init = true;
  }

  for (uint8_t p = 0; p <= 12; p++) {
    activarSolo(p);
    hal::delayMs(400);
  }

  apagarP0aP12();
  hal::delayMs(800);
}
