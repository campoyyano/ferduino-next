#pragma once

#include <stdint.h>

namespace app::cfg {

// Registry keys (TLV) para AppConfig.
// Rango recomendado:
// - 0..199 reservado para migración legacy (temperaturas, etc.)
// - 200..299 AppConfig/network/mqtt

static constexpr uint16_t KEY_BACKEND_MODE = 205; // u32: 0=Legacy,1=HA

static constexpr uint16_t KEY_MQTT_HOST    = 200; // string
static constexpr uint16_t KEY_MQTT_PORT    = 201; // u32
static constexpr uint16_t KEY_DEVICE_ID    = 204; // string

static constexpr uint16_t KEY_NET_DHCP     = 210; // bool
static constexpr uint16_t KEY_NET_IP       = 211; // u32 packed a|b<<8|c<<16|d<<24
static constexpr uint16_t KEY_NET_GW       = 212; // u32 packed
static constexpr uint16_t KEY_NET_SUBNET   = 213; // u32 packed
static constexpr uint16_t KEY_NET_DNS      = 214; // u32 packed

// MAC: string "02:FD:00:00:00:01" (17 chars)
static constexpr uint16_t KEY_NET_MAC      = 215; // string

} // namespace app::cfg
