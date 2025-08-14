
#pragma once
#include "esphome.h"
#include <vector>
#include <array>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <functional>
#include <array>

// Non-blocking batch WOL sender with LED feedback.

namespace esphome {
namespace wake_on_lan_config {

// Static state machine (no Component registration needed).

namespace internal {
  // Timing constants (ms)
  static const uint16_t LED_ON_MS = 70;
  static const uint16_t REPEAT_GAP_MS = 25;
  static const uint16_t MAC_GAP_MS = 50;
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

  static void stop_sequence() {
    if (sock_fd >= 0) { lwip_close(sock_fd); sock_fd = -1; }
    if (led_on && led_cb) { led_cb(false); }
    active = false; led_on = false; phase = PHASE_IDLE; packets.clear();
  }
}

inline void send_magic_packet(const std::string &mac) {
  int values[6];
  if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) return;
  int sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) return;
  int broadcastEnable = 1; lwip_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
  struct sockaddr_in addr; memset(&addr,0,sizeof(addr)); addr.sin_family=AF_INET; addr.sin_port=htons(9); addr.sin_addr.s_addr=inet_addr("255.255.255.255");
  uint8_t packet[102]; memset(packet,0xFF,6); uint8_t mac_bytes[6]; for(int i=0;i<6;i++) mac_bytes[i]=(uint8_t)values[i]; for(int i=0;i<16;i++) memcpy(packet+6+i*6,mac_bytes,6);
  lwip_sendto(sock,(const char*)packet,sizeof(packet),0,(struct sockaddr*)&addr,sizeof(addr)); lwip_close(sock);
}

inline void wol_start(const std::vector<std::string> &macs, int repeats, const std::function<void(bool)> &led_cb) {
  using namespace internal;
  stop_sequence();
  if (macs.empty()) {
    led_cb(true); led_on = true; active = true; phase = PHASE_LED_ONLY_OFF; next_action_ms = millis() + LED_ON_MS; internal::led_cb = led_cb; return;
  }
  if (repeats < 1) repeats = 1; if (repeats > 10) repeats = 10;
  repeats_target = repeats; current_mac_index = 0; current_repeat = 0; internal::led_cb = led_cb; led_on = false; active = false; packets.clear(); packets.reserve(macs.size());
  for (auto &m : macs) {
    int v[6]; if (sscanf(m.c_str(), "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) continue;
    std::array<uint8_t,102> pkt; memset(pkt.data(),0xFF,6); uint8_t mb[6]; for(int i=0;i<6;i++) mb[i]=(uint8_t)v[i]; for(int i=0;i<16;i++) memcpy(pkt.data()+6+i*6,mb,6); packets.push_back(pkt);
  }
  if (packets.empty()) { // invalid only
    led_cb(true); led_on = true; active = true; phase = PHASE_LED_ONLY_OFF; next_action_ms = millis() + LED_ON_MS; return; }
  sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); if (sock_fd < 0) return; int b=1; lwip_setsockopt(sock_fd,SOL_SOCKET,SO_BROADCAST,&b,sizeof(b)); memset(&addr,0,sizeof(addr)); addr.sin_family=AF_INET; addr.sin_port=htons(9); addr.sin_addr.s_addr=inet_addr("255.255.255.255");
  active = true; phase = PHASE_SEND_AND_LED_ON; next_action_ms = millis();
}

inline void wol_process() {
  using namespace internal;
  if (!active) return; unsigned long now = millis(); if ((long)(now - next_action_ms) < 0) return;
  switch (phase) {
    case PHASE_SEND_AND_LED_ON: {
      if (led_cb) { led_cb(true); led_on = true; }
      lwip_sendto(sock_fd,(const char*)packets[current_mac_index].data(), packets[current_mac_index].size(),0,(struct sockaddr*)&addr,sizeof(addr));
      next_action_ms = now + LED_ON_MS; phase = PHASE_LED_OFF_OR_NEXT; break;
    }
    case PHASE_LED_OFF_OR_NEXT: {
      if (led_on && led_cb) { led_cb(false); led_on = false; }
      current_repeat++;
      if (current_repeat < repeats_target) { phase = PHASE_SEND_AND_LED_ON; next_action_ms = now + REPEAT_GAP_MS; }
      else { current_repeat = 0; current_mac_index++; if (current_mac_index < (int)packets.size()) { phase = PHASE_SEND_AND_LED_ON; next_action_ms = now + MAC_GAP_MS; } else { stop_sequence(); } }
      break;
    }
    case PHASE_LED_ONLY_OFF: {
      if (led_on && led_cb) { led_cb(false); led_on = false; }
      stop_sequence(); break;
    }
    case PHASE_IDLE: default: break;
  }
}


}  // namespace wake_on_lan_config
}  // namespace esphome
