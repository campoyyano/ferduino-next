# Migration Playbook — Ferduino HAL (procedimiento seguro)

Este playbook define el procedimiento para migrar módulos del firmware legacy a la arquitectura HAL sin romper compilación ni rediseñar a mitad.

## Principios

1. **API estable por módulo** (App layer):
   - Define tipos y funciones públicas en `include/app/<modulo>/...`.
   - El resto de la app solo consume esta API.

2. **Backend seleccionable por flag**:
   - `APP_<MODULO>_USE_<BACKEND>=0/1`
   - `0` = FAKE/stub, `1` = HW/real
   - Defaults centralizados en `include/app/app_build_flags.h`.

3. **FAKE primero, HW después**:
   - El fake debe simular suficiente comportamiento para validar integración.
   - El HW se añade detrás de HAL/hook sin cambiar la API.

4. **Cambios mínimos y trazables**:
   - Un commit por unidad lógica (módulo / bugfix / docs).
   - Actualizar siempre `07_CONTEXT.md` con el cambio y motivación.

## Checklist por módulo (template)

1) **Definir API pública**
- `include/app/<modulo>/<modulo>.h`
- Tipos y funciones mínimas.

2) **Implementar backend FAKE (flag=0)**
- `src/app/<modulo>/<modulo>.cpp`
- Determinista, sin dependencias HW.

3) **Implementar backend HW stub (flag=1)**
- Debe compilar aunque no haya hardware.
- Devuelve `valid=false`/no-op si procede.
- Más adelante se sustituye por HW real sin tocar la API.

4) **Integrar en runtime**
- `app_runtime.cpp`: begin/loop + telemetría mínima si aplica.

5) **Integración MQTT/HA**
- state + discovery coherentes.
- topics estables.

6) **NVM**
- keys nuevas documentadas y, si aplica, migración legacy idempotente.

7) **Smoketests**
- Añadir/actualizar test mínimo si aplica.

8) **Docs**
- Actualizar `docs/NVM_REGISTRY_KEYS_AND_EEPROM_MAP.md` y `07_CONTEXT.md`.

## Política de layout (.h/.cpp)

- Headers públicos → `include/`
- Headers privados → pueden vivir junto al `.cpp` en `src/` **siempre que** no se incluyan fuera del módulo.
- Evitar mover archivos salvo necesidad: si se mueve, hay que actualizar includes y validar build.
