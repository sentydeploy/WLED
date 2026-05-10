#include "wled.h"
#include <WiFiUdp.h>

class WakeOnLanUsermod : public Usermod {
private:
  char _mac[18]   = "D8:BB:C1:0C:A2:84";
  char _topic[48] = "wled/01896c/wol";
  bool _macParsed = false;
  uint8_t _macBytes[6];

  void parseMac() {
    if (_macParsed) return;
    int v[6];
    if (sscanf(_mac, "%x:%x:%x:%x:%x:%x", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]) == 6) {
      for (int i=0;i<6;i++) _macBytes[i]=(uint8_t)v[i];
      _macParsed = true;
    }
  }

  void sendMagicPacket() {
    parseMac();
    if (!_macParsed) return;
    uint8_t packet[102];
    for (int i=0;i<6;i++) packet[i]=0xFF;
    for (int i=1;i<=16;i++)
      for (int j=0;j<6;j++)
        packet[i*6+j]=_macBytes[j];
    WiFiUDP udp;
    udp.begin(9);
    udp.beginPacket(IPAddress(255,255,255,255),9);
    udp.write(packet,sizeof(packet));
    udp.endPacket();
    udp.stop();
    DEBUG_PRINTLN(F("WoL: Magic packet sent!"));
  }

public:
  void setup() override {}
  void loop() override {}

  bool onMqttMessage(char* topic, char* payload) override {
    if (strcmp(topic, _topic) == 0) {
      sendMagicPacket();
      return true;
    }
    return false;
  }

  void onMqttConnect(bool sessionPresent) override {
    if (WLED_MQTT_CONNECTED) {
      mqtt->subscribe(_topic, 0);
      DEBUG_PRINTLN(F("WoL: Subscribed to MQTT topic"));
    }
  }

  void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject("WakeOnLan");
    top["mac"]   = _mac;
    top["topic"] = _topic;
  }

  bool readFromConfig(JsonObject& root) override {
    JsonObject top = root["WakeOnLan"];
    if (top.isNull()) return false;
    if (top["mac"].as<String>().length() > 0)   strlcpy(_mac,   top["mac"],   sizeof(_mac));
    if (top["topic"].as<String>().length() > 0) strlcpy(_topic, top["topic"], sizeof(_topic));
    _macParsed = false;
    return true;
  }

  uint16_t getId() override { return USERMOD_ID_UNSPECIFIED; }
};

static WakeOnLanUsermod wol_usermod;
REGISTER_USERMOD(wol_usermod);
