#pragma once
#include <stdint.h>

namespace hal {

  enum class RtcTipo : uint8_t {
    Ninguno = 0,
    DS1307  = 1,
    DS3231  = 2
  };

  struct FechaHora {
    uint16_t anio;
    uint8_t  mes;
    uint8_t  dia;
    uint8_t  hora;
    uint8_t  minuto;
    uint8_t  segundo;
  };

  // Inicia y detecta RTC. Devuelve true si detecta DS1307 o DS3231.
  bool rtcIniciar();

  // Devuelve qué RTC se detectó.
  RtcTipo rtcTipo();

  // Devuelve true si la hora parece válida según el chip detectado.
  bool rtcHoraValida();

  // Lee fecha/hora.
  bool rtcLeer(FechaHora& out);

  // Ajusta fecha/hora.
  bool rtcAjustar(const FechaHora& fh);

} // namespace hal
