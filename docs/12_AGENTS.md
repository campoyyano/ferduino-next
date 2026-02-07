# ferduino-next – Reglas para Codex

## Alcance
- Trabaja SOLO en: Firmware/port
- NO modificar nunca: Firmware/Original (read-only, solo referencia)

## Estilo de cambios
- Cambios mínimos e incrementales.
- No refactors masivos sin pedirlo explícitamente.
- Mantén compatibilidad AVR (sin heap si se puede, evitar String).

## Organización
- Headers en include/ con rutas coherentes.
- Sources en src/ respetando la estructura actual.
- No introducir dependencias nuevas sin justificar.

## Compilación y validación
- Si cambias código, ejecuta build del env mega2560_port.
- No introducir warnings nuevos (si aparecen, justificar o corregir).

## NVM/EEPROM
- No romper TLV/CRC existentes.
- Migración legacy -> registry: idempotente, con flag REGF_MIGRATED.

## Commits
- Propón mensaje de commit (con scope), lista de archivos y razón.

## promt para codex minimo

  -Trabaja solo en Firmware/port

  -Firmware/Original es read-only: no modificar

  -Cambios mínimos, incremental, con commit plan

  -Si no estás seguro, pregunta en el PR/diff (no inventes)