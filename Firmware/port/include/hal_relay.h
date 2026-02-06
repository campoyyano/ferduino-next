#pragma once
#include <stdint.h>

namespace hal {

  // Relés / salidas a través del PCF8575 (P0..P15)
  // Basado en el pinout original:
  // P0  ozonizador
  // P1  reator (CO2)
  // P2  bomba1
  // P3  bomba2
  // P4  bomba3
  // P5  temporizador1
  // P6  temporizador2
  // P7  temporizador3
  // P8  temporizador4
  // P9  temporizador5
  // P10 solenoide1
  // P11 alimentador
  // P12 skimmer

  enum class Relay : uint8_t {
    Ozonizador = 0,
    Reator,
    Bomba1,
    Bomba2,
    Bomba3,
    Temporizador1,
    Temporizador2,
    Temporizador3,
    Temporizador4,
    Temporizador5,
    Solenoide1,
    Alimentador,
    Skimmer,

    _Count
  };

  // Inicializa el backend (PCF8575). Seguro llamar varias veces.
  bool relayInit();

  // Set lógico: on=true significa “activar relé”, la polaridad la gestiona IoExpander (ACTIVE_LOW)
  bool relaySet(Relay r, bool on);

  // Lectura del nivel físico (HIGH/LOW) (útil para debug; no siempre representa on/off lógico si ACTIVE_LOW)
  bool relayGetNivelFisico(Relay r, bool& nivel);

  // Apaga todos los relés definidos (0.._Count-1)
  void relayAllOff();

}
