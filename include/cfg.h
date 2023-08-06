#ifndef CFG_H
#define CFG_H

#define EEPROM_SIZE 4096
#define OPMODE_WIFI_ACCESSPOINT 0
#define OPMODE_WIFI_STATION 1
#define OPMODE_ETH_CLIENT 2

#define cfg_ver_num 0x2

typedef struct {
  byte version;
  char wifi_ssid[33];
  char wifi_secret[65];
  char wifi_hostname[256];
  uint8_t wifi_opmode;
  bool wifi_powersave;
  char ota_path[256];
  int wifi_ap_fallback;

  char ip_addr[16];
  char ip_gw[16];
  char ip_netmask[16];
  char ip_dns[16];

  long tx_frequency; 
  long bandwidth;
  int sf;
  int tx_power;
  int syncword;
  int tally_id;
  int num_pixels;
  unsigned long tally_timeout;
  unsigned long display_timeout;
  unsigned long inactivity_timeout;
  unsigned long status_interval;
  unsigned long command_interval;
  char mqtt_host[256];
  char atem_host[256];
  int tsl_port;
  char tsl_host[64];
  int led_max_brightness;
  int tally_screen; 
  int atem_channel_offset;
} settings_t;

extern settings_t cfg;

void config_setup(void);
void write_config(void);
void read_config(void);
String get_settings(void);
boolean parse_settings(DynamicJsonDocument json);
void factory_reset(int state);

#endif
