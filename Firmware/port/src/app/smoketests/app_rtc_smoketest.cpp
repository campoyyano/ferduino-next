#include <Arduino.h>
#include "hal/hal_time.h"
#include "hal/hal_rtc.h"
#include "hal/hal_serial.h"
#include "board/board_debug.h"

static void imprimir2(uint8_t v) {
  if (v < 10) hal::serialEscribir(board::debugUart, "0");
  char buf[4];
  itoa(v, buf, 10);
  hal::serialEscribir(board::debugUart, buf);
}

static const char* rtcNombre(hal::RtcTipo t) {
  switch (t) {
    case hal::RtcTipo::DS1307: return "DS1307";
    case hal::RtcTipo::DS3231: return "DS3231";
    default: return "Ninguno";
  }
}

void app_rtc_smoketest_run() {
  static bool init = false;
  if (!init) {
    hal::serialIniciar(board::debugUart, 115200);

    const bool ok = hal::rtcIniciar();
    hal::serialEscribir(board::debugUart, "[RTC] Detectado: ");
    hal::serialEscribirLinea(board::debugUart, ok ? rtcNombre(hal::rtcTipo()) : "NO");

    hal::serialEscribir(board::debugUart, "[RTC] Hora valida: ");
    hal::serialEscribirLinea(board::debugUart, hal::rtcHoraValida() ? "SI" : "NO");

    init = true;
  }

  hal::FechaHora fh;
  if (hal::rtcLeer(fh)) {
    // YYYY-MM-DD HH:MM:SS
    char buf[8];
    ultoa((unsigned long)fh.anio, buf, 10);
    hal::serialEscribir(board::debugUart, buf);
    hal::serialEscribir(board::debugUart, "-");
    imprimir2(fh.mes);
    hal::serialEscribir(board::debugUart, "-");
    imprimir2(fh.dia);
    hal::serialEscribir(board::debugUart, " ");
    imprimir2(fh.hora);
    hal::serialEscribir(board::debugUart, ":");
    imprimir2(fh.minuto);
    hal::serialEscribir(board::debugUart, ":");
    imprimir2(fh.segundo);
    hal::serialEscribirLinea(board::debugUart, "");
  } else {
    hal::serialEscribirLinea(board::debugUart, "[RTC] No se pudo leer (sin HW o no responde)");
  }

  hal::delayMs(1000);
}

