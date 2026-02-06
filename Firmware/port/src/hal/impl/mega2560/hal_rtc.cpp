#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include "hal/hal_rtc.h"

namespace hal {

  static RTC_DS1307 rtc1307;
  static RTC_DS3231 rtc3231;

  static bool iniciado = false;
  static RtcTipo tipo = RtcTipo::Ninguno;

  static FechaHora convertir(const DateTime& dt) {
    FechaHora fh;
    fh.anio = (uint16_t)dt.year();
    fh.mes  = (uint8_t)dt.month();
    fh.dia  = (uint8_t)dt.day();
    fh.hora = (uint8_t)dt.hour();
    fh.minuto = (uint8_t)dt.minute();
    fh.segundo = (uint8_t)dt.second();
    return fh;
  }

  static DateTime convertir(const FechaHora& fh) {
    return DateTime(fh.anio, fh.mes, fh.dia, fh.hora, fh.minuto, fh.segundo);
  }

  // Heurística para distinguir DS3231 de DS1307 cuando RTC_CHIP=AUTO
  // DS3231 tiene temperatura en 0x11/0x12; el LSB (0x12) usa solo bits 7..6 (cuartos de grado)
  // => en DS3231: (reg0x12 & 0x3F) == 0 siempre.
  static bool pareceDS3231() {
    const uint8_t addr = 0x68;

    Wire.beginTransmission(addr);
    Wire.write((uint8_t)0x12); // temp LSB en DS3231
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom((int)addr, 1) != 1) return false;
    uint8_t t_lsb = Wire.read();

    // Si los 6 bits bajos son 0, es MUY probable DS3231 (en DS1307 sería RAM aleatoria)
    return ((t_lsb & 0x3F) == 0);
  }

  bool rtcIniciar() {
    if (iniciado) return (tipo != RtcTipo::Ninguno);

    Wire.begin();

#if (RTC_CHIP == RTC_CHIP_DS1307)

    if (rtc1307.begin()) {
      tipo = RtcTipo::DS1307;
      iniciado = true;
      return true;
    }

#elif (RTC_CHIP == RTC_CHIP_DS3231)

    if (rtc3231.begin()) {
      tipo = RtcTipo::DS3231;
      iniciado = true;
      return true;
    }

#elif (RTC_CHIP == RTC_CHIP_AUTO)

    // AUTO: intentamos identificar DS3231 por heurística; si no, asumimos DS1307
    if (pareceDS3231()) {
      if (rtc3231.begin()) {
        tipo = RtcTipo::DS3231;
        iniciado = true;
        return true;
      }
    }

    if (rtc1307.begin()) {
      tipo = RtcTipo::DS1307;
      iniciado = true;
      return true;
    }

#else
  #error "RTC_CHIP invalido"
#endif

    tipo = RtcTipo::Ninguno;
    iniciado = true;
    return false;
  }

  RtcTipo rtcTipo() {
    rtcIniciar();
    return tipo;
  }

  bool rtcHoraValida() {
    if (!rtcIniciar()) return false;

    if (tipo == RtcTipo::DS1307) {
      return rtc1307.isrunning();
    }
    if (tipo == RtcTipo::DS3231) {
      return !rtc3231.lostPower();
    }
    return false;
  }

  bool rtcLeer(FechaHora& out) {
    if (!rtcIniciar()) return false;

    if (tipo == RtcTipo::DS1307) {
      out = convertir(rtc1307.now());
      return true;
    }
    if (tipo == RtcTipo::DS3231) {
      out = convertir(rtc3231.now());
      return true;
    }
    return false;
  }

  bool rtcAjustar(const FechaHora& fh) {
    if (!rtcIniciar()) return false;
    const DateTime dt = convertir(fh);

    if (tipo == RtcTipo::DS1307) {
      rtc1307.adjust(dt);
      return true;
    }
    if (tipo == RtcTipo::DS3231) {
      rtc3231.adjust(dt);
      return true;
    }
    return false;
  }

}
