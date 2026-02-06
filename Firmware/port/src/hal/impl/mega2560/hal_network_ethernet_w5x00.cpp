#include "hal/hal_network.h"

#if defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_ARCH_AVR)

  // IMPORTANTE: solo aquí incluimos la librería de plataforma.
  #include <SPI.h>
  #include <Ethernet.h>

  // Si en tu board config quieres definir el CS del shield, puedes hacerlo así:
  //   -D HAL_ETH_CS_PIN=10
  // o en un header del port.
  #ifndef HAL_ETH_CS_PIN
    #define HAL_ETH_CS_PIN 10
  #endif

namespace hal {

static inline IPAddress toIpAddress(const Ip4& ip) {
  return IPAddress(ip.a, ip.b, ip.c, ip.d);
}

static inline Ip4 fromIpAddress(const IPAddress& ip) {
  return Ip4(ip[0], ip[1], ip[2], ip[3]);
}

// Adaptador EthernetClient -> ITcpClient
class EthernetTcpClientAdapter final : public ITcpClient {
public:
  bool connect(const char* host, uint16_t port) override {
    if (!host) return false;
    return _client.connect(host, port);
  }

  void stop() override {
    _client.stop();
  }

  bool connected() const override {
    return _client.connected();
  }

  int available() const override {
    return _client.available();
  }

  int read() override {
    return _client.read();
  }

  int read(uint8_t* dst, size_t len) override {
    if (!dst || len == 0) return 0;
    // EthernetClient::read(uint8_t*, size_t) devuelve int (n bytes) en la mayoría de cores.
    return _client.read(dst, len);
  }

  size_t write(const uint8_t* src, size_t len) override {
    if (!src || len == 0) return 0;
    return _client.write(src, len);
  }

  void flush() override {
    _client.flush();
  }

  // Acceso interno por si más adelante MQTT necesita el Client real
  EthernetClient& raw() { return _client; }

private:
  EthernetClient _client;
};

class NetworkHalEthernetW5x00 final : public INetworkHal {
public:
  NetError begin(const NetworkConfig& cfg) override {
    // Config mínima
    if (isZeroMac(cfg.mac)) return NetError::InvalidConfig;

    Ethernet.init(HAL_ETH_CS_PIN);

    // Nota: algunas variantes de Ethernet lib exponen Ethernet.hardwareStatus()
    // y linkStatus(). Lo dejamos defensivo.
    if (hardwareMissing()) {
      _initialized = false;
      return NetError::NoHardware;
    }

    int ok = 1;

    if (cfg.useDhcp) {
      ok = Ethernet.begin(const_cast<uint8_t*>(cfg.mac)); // API no-const en Arduino
      if (ok == 0) {
        _initialized = false;
        return NetError::DhcpFailed;
      }
    } else {
      // Estático
      const IPAddress ip      = toIpAddress(cfg.ip);
      const IPAddress dns     = toIpAddress(cfg.dns);
      const IPAddress gateway = toIpAddress(cfg.gateway);
      const IPAddress subnet  = toIpAddress(cfg.subnet);

      // Overload habitual: Ethernet.begin(mac, ip, dns, gateway, subnet)
      Ethernet.begin(const_cast<uint8_t*>(cfg.mac), ip, dns, gateway, subnet);
    }

    _initialized = true;

    // Si la lib soporta link status y está down, lo reflejamos
    if (linkStatus() == LinkStatus::Down) {
      // No lo consideramos fatal porque hay setups donde no se detecta bien
      // pero devolvemos estado correcto en linkStatus().
    }

    return NetError::Ok;
  }

  void loop() override {
    // En W5x00 normalmente es no-op.
  }

  bool isInitialized() const override {
    return _initialized;
  }

  LinkStatus linkStatus() const override {
    // Ethernet.linkStatus() no existe en todas las versiones.
    // Cuando no existe, devolvemos Unknown.
    #if defined(ETHERNET_LINK_STATUS)
      const auto ls = Ethernet.linkStatus();
      if (ls == LinkON)  return LinkStatus::Up;
      if (ls == LinkOFF) return LinkStatus::Down;
      return LinkStatus::Unknown;
    #else
      return LinkStatus::Unknown;
    #endif
  }

  Ip4 localIp() const override {
    if (!_initialized) return Ip4();
    return fromIpAddress(Ethernet.localIP());
  }

  Ip4 gatewayIp() const override {
    if (!_initialized) return Ip4();
    return fromIpAddress(Ethernet.gatewayIP());
  }

  Ip4 subnetMask() const override {
    if (!_initialized) return Ip4();
    return fromIpAddress(Ethernet.subnetMask());
  }

  Ip4 dnsServer() const override {
    if (!_initialized) return Ip4();
    return fromIpAddress(Ethernet.dnsServerIP());
  }

  ITcpClient& tcp() override {
    return _tcp;
  }

private:
  static bool isZeroMac(const uint8_t mac[6]) {
    for (int i = 0; i < 6; ++i) {
      if (mac[i] != 0) return false;
    }
    return true;
  }

  static bool hardwareMissing() {
    // Si existe hardwareStatus(), úsalo
    #if defined(ETHERNET_HARDWARE_STATUS)
      const auto hs = Ethernet.hardwareStatus();
      return (hs == EthernetNoHardware);
    #else
      return false; // no podemos detectarlo
    #endif
  }

  bool _initialized = false;
  EthernetTcpClientAdapter _tcp;
};

// Instancia única (sin heap, friendly AVR)
static NetworkHalEthernetW5x00 g_net;

INetworkHal& network() {
  return g_net;
}

} // namespace hal

#endif // AVR Mega
