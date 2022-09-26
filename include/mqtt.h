#ifndef MQTT_H
#define MQTT_H
boolean mqtt_setup();
boolean mqtt_publish(const char *topic, const char *payload);
void mqtt_loop();
#endif