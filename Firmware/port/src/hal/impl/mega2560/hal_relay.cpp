#include "hal/hal_relay.h"

#include "hal/hal_ioexpander.h"
#include "board/board_hw_config.h"

namespace hal {

  static bool g_init = false;
  static IoExpander g_iox(FERDUINO_PCF8575_ADDR);

  static uint8_t relayToPcfPin(Relay r) {
    // Por diseño, el enum empieza en 0 y es contiguo.
    return static_cast<uint8_t>(r);
  }

  bool relayInit() {
    if (g_init) return true;
    g_init = g_iox.begin();
    return g_init;
  }

  bool relaySet(Relay r, bool on) {
    if (!relayInit()) return false;
    const uint8_t p = relayToPcfPin(r);
    return g_iox.writePin(p, on);
  }

  bool relayGetNivelFisico(Relay r, bool& nivel) {
    if (!relayInit()) return false;
    const uint8_t p = relayToPcfPin(r);
    return g_iox.readPin(p, nivel);
  }

  void relayAllOff() {
    // Aunque init falle (sin HW), intentamos no romper.
    if (!relayInit()) return;

    for (uint8_t i = 0; i < static_cast<uint8_t>(Relay::_Count); i++) {
      g_iox.writePin(i, false);
    }
  }

}
