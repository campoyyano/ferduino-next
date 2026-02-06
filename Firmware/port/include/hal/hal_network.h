#pragma once

#include <stdint.h>
#include <stddef.h>

// HAL de red (Ethernet / futuros transports).
// Nota: Este header NO debe incluir headers de plataforma (Ethernet.h, WiFi.h, etc.).
// Las implementaciones viven en src/hal/impl/<platform>/.

namespace hal {

struct Ip4 {
  uint8_t a, b, c, d;

  constexpr Ip4() : a(0), b(0), c(0), d(0) {}
  constexpr Ip4(uint8_t a_, uint8_t b_, uint8_t c_, uint8_t d_)
  : a(a_), b(b_), c(c_), d(d_) {}

struct NetworkConfig {
  // MAC obligatoria (en Mega con W5x00 es lo normal). Si no se quiere configurar aún,
  // se puede rellenar desde la implementación por defecto, pero aquí se deja explícita.
  uint8_t mac[6] = {0, 0, 0, 0, 0, 0};

  bool useDhcp = true;

  // Solo usados si useDhcp == false
  Ip4 ip      = Ip4(0, 0, 0, 0);
  Ip4 gateway = Ip4(0, 0, 0, 0);
  Ip4 subnet  = Ip4(255, 255, 255, 0);
  Ip4 dns     = Ip4(0, 0, 0, 0);
};

enum class LinkStatus : uint8_t {
  Unknown = 0,
  Down    = 1,
  Up      = 2,
};

enum class NetError : uint8_t {
  Ok = 0,
  NotInitialized,
  DhcpFailed,
  InvalidConfig,
  NoHardware,
  LinkDown,
  InternalError,
};

// Cliente TCP abstracto para desacoplar MQTT/otros protocolos del stack concreto.
// OJO: no hereda de Arduino Client a propósito.
class ITcpClient {
public:
  virtual ~ITcpClient() = default;

  virtual bool connect(const char* host, uint16_t port) = 0;
  virtual void stop() = 0;

  virtual bool connected() const = 0;

  virtual int available() const = 0;
  virtual int read() = 0;
  virtual int read(uint8_t* dst, size_t len) = 0;

  virtual size_t write(const uint8_t* src, size_t len) = 0;
  virtual void flush() = 0;
};

class INetworkHal {
public:
  virtual ~INetworkHal() = default;

  // Inicializa el stack de red.
  // - cfg.useDhcp == true: intenta DHCP
  // - cfg.useDhcp == false: usa configuración estática (ip/gw/subnet/dns)
  virtual NetError begin(const NetworkConfig& cfg) = 0;

  // Debe llamarse periódicamente si la implementación lo necesita
  // (en W5x00 puede ser no-op, pero se deja por portabilidad).
  virtual void loop() = 0;

  virtual bool isInitialized() const = 0;

  virtual LinkStatus linkStatus() const = 0;

  virtual Ip4 localIp() const = 0;
  virtual Ip4 gatewayIp() const = 0;
  virtual Ip4 subnetMask() const = 0;
  virtual Ip4 dnsServer() const = 0;

  // Acceso a un cliente TCP (reutilizable) para capas superiores (MQTT, etc.).
  // Decisión: devolvemos referencia para evitar alloc dinámica en AVR.
  virtual ITcpClient& tcp() = 0;
};

} // namespace hal
