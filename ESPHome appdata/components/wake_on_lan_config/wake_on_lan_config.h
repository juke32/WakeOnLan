
// Minimal Wake-on-LAN helper (simplified): builds & sends magic packets with optional LED feedback.
#pragma once
#include "esphome.h"
#include <vector>
#include <array>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <functional>
#include <errno.h>

namespace esphome {
namespace wake_on_lan_config {

namespace internal {
  static const uint16_t LED_ON_MS = 70;
  static const uint16_t REPEAT_GAP_MS = 25;
  static const uint16_t MAC_GAP_MS = 50;
  static const int WOL_PACKET_LEN = 102; // 6*FF + 16 * 6-byte MAC
  enum Phase { PHASE_IDLE, PHASE_SEND_AND_LED_ON, PHASE_LED_OFF_OR_NEXT, PHASE_LED_ONLY_OFF };
  static Phase phase = PHASE_IDLE;
  static std::vector<std::array<uint8_t,102>> packets;
  static int repeats_target = 1;
  static int current_mac_index = 0;
  static int current_repeat = 0;
  static bool led_on = false;
  static bool active = false;
  static unsigned long next_action_ms = 0;
  static std::function<void(bool)> led_cb;
  static int sock_fd = -1;
  static struct sockaddr_in addr;
  // Destination parameters (broadcast IP fixed, port configurable)
  static int dest_port = 9;               // default WOL port
  static const char *BCAST_IP = "255.255.255.255";
  // No skip list in minimal version

  static void stop_sequence() {
    if (sock_fd >= 0) { lwip_close(sock_fd); sock_fd = -1; }
    if (led_on && led_cb) { led_cb(false); }
    active = false; led_on = false; phase = PHASE_IDLE; packets.clear();
  }

  inline void configure_addr() {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dest_port);
    addr.sin_addr.s_addr = inet_addr(BCAST_IP);
  }

  // Diagnostics removed for minimal build
}

// Dynamic port/broadcast removed for minimal build

inline void send_magic_packet(const std::string &mac) {
  int v[6]; if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]) != 6) return;
  uint8_t pkt[102]; memset(pkt,0xFF,6); uint8_t mb[6]; for(int i=0;i<6;i++) mb[i]=(uint8_t)v[i]; for(int i=0;i<16;i++) memcpy(pkt+6+i*6, mb,6);
  int sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); if (sock < 0) return; int b=1; lwip_setsockopt(sock,SOL_SOCKET,SO_BROADCAST,&b,sizeof(b));
  struct sockaddr_in a; memset(&a,0,sizeof(a)); a.sin_family=AF_INET; a.sin_port=htons(internal::dest_port); a.sin_addr.s_addr=inet_addr(internal::BCAST_IP);
  lwip_sendto(sock,(const char*)pkt,sizeof(pkt),0,(struct sockaddr*)&a,sizeof(a)); lwip_close(sock);
}

inline void wol_start(const std::vector<std::string> &macs, int repeats, const std::function<void(bool)> &led_cb) {
  using namespace internal; stop_sequence(); if (macs.empty()) { led_cb(true); led_on=true; active=true; phase=PHASE_LED_ONLY_OFF; next_action_ms=millis()+LED_ON_MS; internal::led_cb=led_cb; return; }
  if (repeats <1) repeats=1; if (repeats>10) repeats=10;
  repeats_target=repeats; current_mac_index=0; current_repeat=0; internal::led_cb=led_cb; led_on=false; active=false; packets.clear(); packets.reserve(macs.size());
  for (auto &m: macs) {
    int v[6];
    if (sscanf(m.c_str(), "%x:%x:%x:%x:%x:%x", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5])!=6) continue;
    bool all_zero=true; for(int i=0;i<6;i++) if (v[i]!=0){all_zero=false;break;}
    if(all_zero) continue;
    std::array<uint8_t,102> pkt; memset(pkt.data(),0xFF,6);
    uint8_t mb[6]; for(int i=0;i<6;i++) mb[i]=(uint8_t)v[i];
    for(int i=0;i<16;i++) memcpy(pkt.data()+6+i*6, mb,6);
    packets.push_back(pkt);
  }
  if (packets.empty()) { led_cb(true); led_on=true; active=true; phase=PHASE_LED_ONLY_OFF; next_action_ms=millis()+LED_ON_MS; return; }
  sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); if (sock_fd <0) return; int b=1; lwip_setsockopt(sock_fd,SOL_SOCKET,SO_BROADCAST,&b,sizeof(b));
  configure_addr();
  active=true; phase=PHASE_SEND_AND_LED_ON; next_action_ms=millis();
}

inline void set_wol_port(int port) {
  using namespace internal;
  if (port < 1 || port > 65535) port = 9;  // clamp / default
  if (dest_port != port) {
    dest_port = port;
    // Reconfigure address for any future batch sends (active sequence keeps old socket/addr)
    if (sock_fd >= 0) {
      configure_addr();
    }
  }
}

inline int get_wol_port() {
  return internal::dest_port;
}

inline void wol_process() {
  using namespace internal; if(!active) return; unsigned long now=millis(); if((long)(now - next_action_ms) < 0) return; switch(phase){
    case PHASE_SEND_AND_LED_ON: { if (led_cb) { led_cb(true); led_on=true; } lwip_sendto(sock_fd,(const char*)packets[current_mac_index].data(), packets[current_mac_index].size(),0,(struct sockaddr*)&addr,sizeof(addr)); next_action_ms=now+LED_ON_MS; phase=PHASE_LED_OFF_OR_NEXT; break; }
    case PHASE_LED_OFF_OR_NEXT: { if (led_on && led_cb) { led_cb(false); led_on=false; } current_repeat++; if (current_repeat < repeats_target) { phase=PHASE_SEND_AND_LED_ON; next_action_ms=now+REPEAT_GAP_MS; } else { current_repeat=0; current_mac_index++; if (current_mac_index < (int)packets.size()) { phase=PHASE_SEND_AND_LED_ON; next_action_ms=now+MAC_GAP_MS; } else { stop_sequence(); } } break; }
    case PHASE_LED_ONLY_OFF: { if (led_on && led_cb) { led_cb(false); led_on=false; } stop_sequence(); break; }
    case PHASE_IDLE: default: break; }
}

// Dump full hex of first queued packet (if any) for diagnostics
// Dump function removed in minimal version

inline bool validate_mac(const std::string &in, std::string &out_norm) {
  std::string hex; hex.reserve(12); for(char c: in){ if((c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f')) hex.push_back(toupper((unsigned char)c)); else if (c==':'||c=='-'||c==' '||c=='\t') continue; }
  if (hex.size()!=12) return false; out_norm.clear(); out_norm.reserve(17); for(int i=0;i<12;i+=2){ if(i) out_norm.push_back(':'); out_norm.push_back(hex[i]); out_norm.push_back(hex[i+1]); }
  if (out_norm == "00:00:00:00:00:00") return false; return true;
}

inline bool is_valid_wol_target(const std::string &mac) { std::string norm; return validate_mac(mac, norm); }

} // namespace wake_on_lan_config
} // namespace esphome
