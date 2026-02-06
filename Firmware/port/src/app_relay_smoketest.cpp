#include <Arduino.h>

#include "hal_time.h"
#include "hal_relay.h"
#include "hal_serial.h"
#include "board_debug.h"

static const char* nombreRelay(hal::Relay r) {
  switch (r) {
    case hal::Relay::Ozonizador:    return "Ozonizador";
    case hal::Relay::Reator:        return "Reator";
    case hal::Relay::Bomba1:        return "Bomba1";
    case hal::Relay::Bomba2:        return "Bomba2";
    case hal::Relay::Bomba3:        return "Bomba3";
    case hal::Relay::Temporizador1: return "Temporizador1";
    case hal::Relay::Temporizador2: return "Temporizador2";
    case hal::Relay::Temporizador3: return "Temporizador3";
    case hal::Relay::Temporizador4: return "Temporizador4";
    case hal::Relay::Temporizador5: return "Temporizador5";
    case hal::Relay::Solenoide1:    return "Solenoide1";
    case hal::Relay::Alimentador:   return "Alimentador";
    case hal::Relay::Skimmer:       return "Skimmer";
    default:                        return "?";
  }
}

void app_relay_smoketest_run() {
  static bool init = false;

  if (!init) {
    hal::serialIniciar(board::debugUart, 115200);
    hal::serialEscribirLinea(board::debugUart, "[RELAY] smoketest start");

    if (!hal::relayInit()) {
      hal::serialEscribirLinea(board::debugUart, "[RELAY] relayInit() fallo (I2C no responde)");
    } else {
      hal::serialEscribirLinea(board::debugUart, "[RELAY] relayInit() OK");
    }

    hal::relayAllOff();
    init = true;
  }

  for (uint8_t i = 0; i < static_cast<uint8_t>(hal::Relay::_Count); i++) {
    const hal::Relay r = static_cast<hal::Relay>(i);

    hal::relayAllOff();

    hal::serialEscribir(board::debugUart, "[RELAY] ON: ");
    hal::serialEscribirLinea(board::debugUart, nombreRelay(r));

    hal::relaySet(r, true);
    hal::delayMs(700);

    hal::relaySet(r, false);
    hal::delayMs(250);
  }

  hal::delayMs(1000);
}
