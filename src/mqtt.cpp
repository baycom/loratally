
#include "main.h"

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);
static unsigned long mqttLast = 0;

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    DynamicJsonDocument json(256);
    DeserializationError error = deserializeJson(json, payload);
    int r = 0;
    int g = 0;
    int b = 0;
    int addr = 0;
    int state = -1;
    int brightness = 3;
    char text[100] = {0};

#ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
#endif
    if (!error) {
        if (json.containsKey("r")) r = json["r"];
        if (json.containsKey("g")) g = json["g"];
        if (json.containsKey("b")) b = json["b"];
        if (json.containsKey("addr")) addr = json["addr"];
        if (json.containsKey("state")) state = json["state"];
        if (json.containsKey("brightness")) brightness = json["brightness"];
        if (json.containsKey("text")) strncpy(text, json["text"], sizeof(text)-1);
        if (state != -1) {
            if(setTallyState(addr, state, brightness, text)) {
                LoRaBCTS();
            }
        }
        if (addr == cfg.tally_id) {
            rgbFromTSL(r, g, b, state, 3);
            setTallyLight(r, g, b);
        }
        LoRaSend(addr, r, g, b);
    }
}

boolean mqtt_setup() {
    if (cfg.mqtt_host[0]) {
        if (mqttClient.connected()) {
            return true;
        }
        mqttClient.setServer(cfg.mqtt_host, 1883);
        mqttClient.setCallback(mqttCallback);
        String client_id = "tally-client-";
        client_id += String(WiFi.macAddress());
        if (mqttClient.connect(client_id.c_str())) {
            String topic = "tally/control/" + String(cfg.tally_id);
            boolean ret = mqttClient.subscribe(topic.c_str());
#ifdef DEBUG
            printf("CId: %s subscribe: %s -> %d conn: %d\n", client_id.c_str(),
                   topic.c_str(), ret, mqttClient.connected());
#endif
            return true;
        }
    }
    return false;
}

boolean mqtt_publish(const char *topic, const char *payload) {
    if (mqtt_setup()) {
        return mqttClient.publish(topic, payload);
    }
    return false;
}

void mqtt_loop() {
    if (cfg.mqtt_host[0] && eth_connected) {
        if (millis() - mqttLast > 10000) {
            boolean ret = mqtt_setup();
            mqttLast = millis();
        }
        mqttClient.loop();
    }
}

