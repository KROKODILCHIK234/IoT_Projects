const bool WIFI_START_MODE_CLIENT = true;
const bool WIFI_START_MODE_AP = false;  

String SSID_AP = "esp8266";
String PASSWORD_AP = "12345678";

int led = LED_BUILTIN;

String SSID_CLI = "kent"; 
String SSID_PASSWORD = "530909";

char* mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;