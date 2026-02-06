#pragma once

#include "board/board_config.h"

namespace hal {

  // HAL de placa: expone capacidades de la plataforma/board sin que
  // el resto del código tenga que tocar macros directamente.

  inline bool esPlaca5V() {
    return boardEs5V;
  }

  inline bool esMega2560() {
    return (BOARD_TARGET == BOARD_MEGA2560);
  }

  inline bool esDue() {
    return (BOARD_TARGET == BOARD_DUE);
  }

  inline bool esGiga() {
    return (BOARD_TARGET == BOARD_GIGA);
  }

  inline bool perfilLegacy() {
    return (PINS_PROFILE == PINS_PROFILE_LEGACY);
  }

  inline bool perfilFase2() {
    return (PINS_PROFILE == PINS_PROFILE_FASE2);
  }

}
