
#include "esphome.h"
#include <vector>
#include <array>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

namespace esphome {
namespace wake_on_lan_config {

class WakeOnLanConfig : public Component {
 public:
  void send_magic_packet(const std::string &mac) {
    // Parse MAC string to bytes
    std::array<uint8_t, 6> mac_bytes;
    int values[6];
    if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6) {
      for (int i = 0; i < 6; ++i) mac_bytes[i] = (uint8_t)values[i];
      // Build magic packet
      uint8_t packet[102];
      memset(packet, 0xFF, 6);
      for (int i = 0; i < 16; ++i) {
        memcpy(packet + 6 + i * 6, mac_bytes.data(), 6);
      }
      // Send via UDP broadcast using ESP-IDF sockets
      int sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (sock < 0) return;
      int broadcastEnable = 1;
      lwip_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
      struct sockaddr_in addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(9);
      addr.sin_addr.s_addr = inet_addr("255.255.255.255");
      lwip_sendto(sock, (const char*)packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr));
      lwip_close(sock);
    }
  }
};

}  // namespace wake_on_lan_config
}  // namespace esphome
