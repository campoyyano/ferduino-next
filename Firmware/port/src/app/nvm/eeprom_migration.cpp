#include "app/nvm/eeprom_migration.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "app/nvm/eeprom_registry.h"
#include "app/nvm/eeprom_layout.h"
#include "hal/hal_storage.h"

namespace app::nvm {

// =========================
// Registry Keys (v1)
// =========================
// Temperatura agua (legacy: struct tempSettings @ 482)
static constexpr uint16_t KEY_TEMP_SET_10   = 100;
static constexpr uint16_t KEY_TEMP_OFF_10   = 101;
static constexpr uint16_t KEY_TEMP_ALARM_10 = 102;

// Outlets (legacy: salvar_outlets_EEPROM)
static constexpr uint16_t KEY_OUTLET_BLOCK_VALID = 310;       // bool
static constexpr uint16_t KEY_OUTLET_1_STATE     = 311;       // u32 (0/1)
static constexpr uint16_t KEY_OUTLET_9_STATE     = 319;       // u32 (0/1)

// Dosificadora / schedules (legacy: Salvar_dosadora_EEPROM)
static constexpr uint16_t KEY_DOSING_BLOCK_VALID   = 500;     // bool
static constexpr uint16_t KEY_DOSING_CH1_DOSE_X10  = 510;     // i32 (x10)
static constexpr uint16_t KEY_DOSING_CH6_DOSE_X10  = 515;

static constexpr uint16_t KEY_DOSING_CH1_START_H   = 520;     // u32 [0..23]
static constexpr uint16_t KEY_DOSING_CH6_START_H   = 525;

static constexpr uint16_t KEY_DOSING_CH1_START_M   = 530;     // u32 [0..59]
static constexpr uint16_t KEY_DOSING_CH6_START_M   = 535;

static constexpr uint16_t KEY_DOSING_CH1_DAYS_MASK = 540;     // u32 bits (Mon..Sun)
static constexpr uint16_t KEY_DOSING_CH6_DAYS_MASK = 545;

static constexpr uint16_t KEY_DOSING_CH1_QTY       = 550;     // u32
static constexpr uint16_t KEY_DOSING_CH6_QTY       = 555;

static constexpr uint16_t KEY_DOSING_CH1_ENABLED   = 560;     // bool
static constexpr uint16_t KEY_DOSING_CH6_ENABLED   = 565;

static constexpr uint16_t KEY_DOSING_CH1_END_H     = 570;     // u32
static constexpr uint16_t KEY_DOSING_CH6_END_H     = 575;

static constexpr uint16_t KEY_DOSING_CH1_END_M     = 580;     // u32
static constexpr uint16_t KEY_DOSING_CH6_END_M     = 585;

// =========================
// Offsets legacy (Ferduino original - Funcoes_EEPROM.h)
// =========================
// TempSettings: EEPROM_writeAnything(482, tempSettings)
// struct config_t { int tempset; byte tempoff; byte tempalarm; }
static constexpr uint16_t LEGACY_TEMP_SET_ADDR   = 482; // int16
static constexpr uint16_t LEGACY_TEMP_OFF_ADDR   = 484; // u8
static constexpr uint16_t LEGACY_TEMP_ALARM_ADDR = 485; // u8

// Outlets:
// EEPROM.write(840, 66); then for i=0..8 EEPROM.write(841+i, outlets[i])
static constexpr uint16_t LEGACY_OUTLETS_SENTINEL_ADDR = 840; // u8 (66)
static constexpr uint16_t LEGACY_OUTLETS_DATA_ADDR     = 841; // 9 bytes

// Dosificadora / schedule:
// EEPROM.write(706, 66);
// EEPROM_writeAnything(545, dosaconfig[6]) where struct { int dose_personalizada; } (int16 on AVR)
// plus per-channel bytes in various addresses.
static constexpr uint16_t LEGACY_DOSING_SENTINEL_ADDR   = 706; // u8 (66)
static constexpr uint16_t LEGACY_DOSING_DOSE_STRUCTS    = 545; // 6 * int16 = 12 bytes

static constexpr uint16_t LEGACY_DOSING_START_H_ADDR    = 557; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_START_M_ADDR    = 583; // 6 bytes

// Days: segunda..domingo arrays are written per channel:
// segunda: 589+i, terca:595+i, quarta:601+i, quinta:607+i, sexta:613+i, sabado:619+i, domingo:625+i
static constexpr uint16_t LEGACY_DOSING_MON_ADDR        = 589; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_TUE_ADDR        = 595; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_WED_ADDR        = 601; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_THU_ADDR        = 607; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_FRI_ADDR        = 613; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_SAT_ADDR        = 619; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_SUN_ADDR        = 625; // 6 bytes

static constexpr uint16_t LEGACY_DOSING_QTY_ADDR        = 631; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_ENABLED_ADDR    = 637; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_END_H_ADDR      = 643; // 6 bytes
static constexpr uint16_t LEGACY_DOSING_END_M_ADDR      = 649; // 6 bytes

static bool readLegacy(uint16_t addr, void* dst, size_t len) {
  return hal::storage().read(hal::StorageRegion::Config, addr, dst, len);
}

static uint32_t buildDaysMask(uint8_t mon, uint8_t tue, uint8_t wed, uint8_t thu,
                             uint8_t fri, uint8_t sat, uint8_t sun) {
  // Convención propuesta:
  // bit0=Mon(segunda), bit1=Tue(terca), bit2=Wed(quarta), bit3=Thu(quinta),
  // bit4=Fri(sexta), bit5=Sat(sabado), bit6=Sun(domingo)
  uint32_t mask = 0;
  if (mon) mask |= (1u << 0);
  if (tue) mask |= (1u << 1);
  if (wed) mask |= (1u << 2);
  if (thu) mask |= (1u << 3);
  if (fri) mask |= (1u << 4);
  if (sat) mask |= (1u << 5);
  if (sun) mask |= (1u << 6);
  return mask;
}

bool migrateLegacyIfNeeded() {
  auto& reg = registry();

  // Si el registry no es válido aún, lo inicializamos.
  if (!reg.isValid()) {
    if (!reg.format()) {
      return false;
    }
  }

  // Ya migrado
  if (reg.flags() & REGF_MIGRATED) {
    return true;
  }

  // ============================================================
  // 1) Temperatura agua (x10)
  // ============================================================
  int16_t set10 = 0;
  uint8_t off10 = 0;
  uint8_t alarm10 = 0;

  (void)readLegacy(LEGACY_TEMP_SET_ADDR, &set10, sizeof(set10));
  (void)readLegacy(LEGACY_TEMP_OFF_ADDR, &off10, sizeof(off10));
  (void)readLegacy(LEGACY_TEMP_ALARM_ADDR, &alarm10, sizeof(alarm10));

  (void)reg.setI32(KEY_TEMP_SET_10, (int32_t)set10);
  (void)reg.setI32(KEY_TEMP_OFF_10, (int32_t)off10);
  (void)reg.setI32(KEY_TEMP_ALARM_10, (int32_t)alarm10);

  // ============================================================
  // 2) Outlets (bloque 840..849)
  // ============================================================
  uint8_t outSent = 0;
  if (readLegacy(LEGACY_OUTLETS_SENTINEL_ADDR, &outSent, sizeof(outSent)) && outSent == 66) {
    uint8_t outs[9] = {0};
    (void)readLegacy(LEGACY_OUTLETS_DATA_ADDR, outs, sizeof(outs));

    (void)reg.setBool(KEY_OUTLET_BLOCK_VALID, true);

    for (uint8_t i = 0; i < 9; ++i) {
      const uint16_t key = (uint16_t)(KEY_OUTLET_1_STATE + i);
      // Guardamos como U32 0/1 (o valores legacy si fueran >1)
      (void)reg.setU32(key, (uint32_t)outs[i]);
    }
  } else {
    // Si no hay bloque válido, dejamos explícito que no migramos outlets.
    (void)reg.setBool(KEY_OUTLET_BLOCK_VALID, false);
  }

  // ============================================================
  // 3) Dosificadora + schedule (bloque 706 + tablas)
  // ============================================================
  uint8_t dosSent = 0;
  if (readLegacy(LEGACY_DOSING_SENTINEL_ADDR, &dosSent, sizeof(dosSent)) && dosSent == 66) {
    (void)reg.setBool(KEY_DOSING_BLOCK_VALID, true);

    // 3.1 dosis personalizada (6x int16 x10)
    int16_t doseX10[6] = {0};
    (void)readLegacy(LEGACY_DOSING_DOSE_STRUCTS, doseX10, sizeof(doseX10));
    for (uint8_t ch = 0; ch < 6; ++ch) {
      (void)reg.setI32((uint16_t)(KEY_DOSING_CH1_DOSE_X10 + ch), (int32_t)doseX10[ch]);
    }

    // 3.2 start/end, qty, enabled (bytes)
    uint8_t startH[6] = {0}, startM[6] = {0};
    uint8_t endH[6]   = {0}, endM[6]   = {0};
    uint8_t qty[6]    = {0};
    uint8_t en[6]     = {0};

    (void)readLegacy(LEGACY_DOSING_START_H_ADDR, startH, sizeof(startH));
    (void)readLegacy(LEGACY_DOSING_START_M_ADDR, startM, sizeof(startM));
    (void)readLegacy(LEGACY_DOSING_END_H_ADDR,   endH,   sizeof(endH));
    (void)readLegacy(LEGACY_DOSING_END_M_ADDR,   endM,   sizeof(endM));
    (void)readLegacy(LEGACY_DOSING_QTY_ADDR,     qty,    sizeof(qty));
    (void)readLegacy(LEGACY_DOSING_ENABLED_ADDR, en,     sizeof(en));

    for (uint8_t ch = 0; ch < 6; ++ch) {
      (void)reg.setU32((uint16_t)(KEY_DOSING_CH1_START_H + ch), (uint32_t)startH[ch]);
      (void)reg.setU32((uint16_t)(KEY_DOSING_CH1_START_M + ch), (uint32_t)startM[ch]);
      (void)reg.setU32((uint16_t)(KEY_DOSING_CH1_END_H   + ch), (uint32_t)endH[ch]);
      (void)reg.setU32((uint16_t)(KEY_DOSING_CH1_END_M   + ch), (uint32_t)endM[ch]);
      (void)reg.setU32((uint16_t)(KEY_DOSING_CH1_QTY     + ch), (uint32_t)qty[ch]);
      (void)reg.setBool((uint16_t)(KEY_DOSING_CH1_ENABLED + ch), en[ch] != 0);
    }

    // 3.3 días activos -> máscara (Mon..Sun)
    uint8_t mon[6] = {0}, tue[6] = {0}, wed[6] = {0}, thu[6] = {0}, fri[6] = {0}, sat[6] = {0}, sun[6] = {0};
    (void)readLegacy(LEGACY_DOSING_MON_ADDR, mon, sizeof(mon));
    (void)readLegacy(LEGACY_DOSING_TUE_ADDR, tue, sizeof(tue));
    (void)readLegacy(LEGACY_DOSING_WED_ADDR, wed, sizeof(wed));
    (void)readLegacy(LEGACY_DOSING_THU_ADDR, thu, sizeof(thu));
    (void)readLegacy(LEGACY_DOSING_FRI_ADDR, fri, sizeof(fri));
    (void)readLegacy(LEGACY_DOSING_SAT_ADDR, sat, sizeof(sat));
    (void)readLegacy(LEGACY_DOSING_SUN_ADDR, sun, sizeof(sun));

    for (uint8_t ch = 0; ch < 6; ++ch) {
      const uint32_t mask = buildDaysMask(mon[ch], tue[ch], wed[ch], thu[ch], fri[ch], sat[ch], sun[ch]);
      (void)reg.setU32((uint16_t)(KEY_DOSING_CH1_DAYS_MASK + ch), mask);
    }
  } else {
    (void)reg.setBool(KEY_DOSING_BLOCK_VALID, false);
  }

  // ============================================================
  // Final: marca migración completada
  // ============================================================
  return reg.setFlags((uint16_t)(reg.flags() | REGF_MIGRATED));
}

} // namespace app::nvm
