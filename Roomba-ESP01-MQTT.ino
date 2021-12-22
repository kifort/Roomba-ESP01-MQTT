#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SimpleTimer.h>
#include <Roomba.h>
#include <ESP8266HTTPUpdateServer.h>
//#include <WiFiClient.h>
//#include <WiFiUdp.h>


//USER CONFIGURED SECTION START//
const char* ssid = "YOUR_WIRELESS_SSID";
const char* wifi_password = "YOUR_WIRELESS_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER_ADDRESS";
const int mqtt_port = YOUR_MQTT_SERVER_PORT;
const char *mqtt_user = "YOUR_MQTT_USERNAME";
const char *mqtt_pass = "YOUR_MQTT_PASSWORD";
const char *mqtt_client_name = "Roomba"; // Client connections can't have the same connection name
const char *ota_password = "YOUR_OTA_PASSWORD";
//USER CONFIGURED SECTION END//

Roomba roomba(&Serial, Roomba::Baud115200);
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
SimpleTimer timer;

// Constants
const char* host = "roomba";
const int PACKET_ID_CHARGING_STATE = 21;
const int PACKET_ID_VOLTAGE = 22;
const int PACKET_ID_CURRENT = 23;
const int PACKET_ID_BATTERY_TEMPERATURE = 24;
const int PACKET_ID_CURRENTLY_CHARGED = 25;
const int PACKET_ID_ESTIMATED_CAPACITY = 26;
const int PACKET_ID_OI_MODE = 35;
const char* TOPIC_COMMANDS = "roomba/commands";
const char* TOPIC_STATUS = "roomba/status";
const char* TOPIC_BATTERY_VOLTAGE = "roomba/battery/voltage";
const char* TOPIC_BATTERY_VOLTAGE_MV = "roomba/battery/voltage_mV";
const char* TOPIC_BATTERY_CURRENT = "roomba/battery/current";
const char* TOPIC_BATTERY_CURRENT_MA = "roomba/battery/current_mA";
const char* TOPIC_BATTERY_TEMPERATURE = "roomba/battery/temperature";
const char* TOPIC_BATTERY_TEMPERATURE_CELSIUS = "roomba/battery/temperature_celsius";
const char* TOPIC_BATTERY_CHARGED_AMOUNT = "roomba/battery/charged/amount";
const char* TOPIC_BATTERY_CHARGED_AMOUNT_MAH = "roomba/battery/charged/amount_mAh";
const char* TOPIC_BATTERY_CAPACITY = "roomba/battery/capacity";
const char* TOPIC_BATTERY_CAPACITY_MAH = "roomba/battery/capacity_mAh";
const char* TOPIC_BATTERY_CHARGED_RATE = "roomba/battery/charged/rate";
const char* TOPIC_BATTERY_CHARGED_RATE_PERCENT = "roomba/battery/charged/rate_percent";
const char* TOPIC_BATTERY_CHARGING = "roomba/battery/charging";
const char* TOPIC_MODE = "roomba/mode";
const char* TOPIC_RUNNING = "roomba/running";

// Variables
bool boot = true;
uint8_t battery_charging_state = 0;
long battery_voltage_mV = 0;
long battery_current_mA = 0;
long battery_temperature_celsius = 0;
long battery_currently_charged_amount_mAh = 0;
long battery_estimated_capacity_mAh = 0;
uint8_t oi_mode = 0;
long battery_currently_charged_rate_percent = 0;
char battery_charging_state_send[30];
char battery_voltage_send[50];
char battery_voltage_mV_send[50];
char battery_current_send[50];
char battery_current_mA_send[50];
char battery_temperature_send[50];
char battery_temperature_celsius_send[50];
char battery_currently_charged_amount_send[50];
char battery_currently_charged_amount_mAh_send[50];
char battery_estimated_capacity_send[50];
char battery_estimated_capacity_mAh_send[50];
char battery_currently_charged_rate_send[10];
char battery_currently_charged_rate_percent_send[5];
char oi_mode_send[10];
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

//Functions
void startCleaning() {
  Serial.write(128); // Start - set passive mode
  delay(50);
  Serial.write(131); // Safe - set safe mode
  delay(50);
  Serial.write(135); // Clean - start the default cleaning mode
  log("Cleaning requested");
}

void stopCleaning() {
  Serial.write(128); // Start - set passive mode
  delay(50);
  Serial.write(131); // Safe - set safe mode
  delay(50);
  Serial.write(143); // Seek Dock - drive onto the dock
  log("Docking requested");
}

void wakeUp() {
  Serial.write(128); // Start - set passive mode
  log("Wake up requested");
}

void sleep() {
  Serial.write(128); // Start - set passive mode
  delay(50);
  // Serial.write(173); // Stop - set off mode
  Serial.write(133); // Power - power down Roomba
  log("Sleep requested");
}

void sendInfoRoomba() {
  roomba.start(); // Serial.write(128)

  uint8_t battery_charging_state_buffer[1];
  roomba.getSensors(PACKET_ID_CHARGING_STATE, battery_charging_state_buffer, 1); // Serial.write(142) // Sensors - requests the OI to send a packet of sensor data bytes
  battery_charging_state = battery_charging_state_buffer[0];
  delay(50);

  uint8_t battery_voltage_buffer[2];
  roomba.getSensors(PACKET_ID_VOLTAGE, battery_voltage_buffer, 2);
  battery_voltage_mV = battery_voltage_buffer[1] + 256 * battery_voltage_buffer[0];
  delay(50);

  uint8_t battery_current_buffer[2];
  roomba.getSensors(PACKET_ID_CURRENT, battery_current_buffer, 2);
  battery_current_mA = int8(battery_current_buffer[1]) + 128 * int8(battery_current_buffer[0]);
  delay(50);

  uint8_t battery_temperature_buffer[1];
  roomba.getSensors(PACKET_ID_BATTERY_TEMPERATURE, battery_temperature_buffer, 1);
  battery_temperature_celsius = int8(battery_temperature_buffer[0]);
  delay(50);

  uint8_t battery_currently_charged_amount_buffer[2];
  roomba.getSensors(PACKET_ID_CURRENTLY_CHARGED, battery_currently_charged_amount_buffer, 2);
  battery_currently_charged_amount_mAh = battery_currently_charged_amount_buffer[1] + 256 * battery_currently_charged_amount_buffer[0];
  delay(50);

  uint8_t battery_estimated_capacity_buffer[2];
  roomba.getSensors(PACKET_ID_ESTIMATED_CAPACITY, battery_estimated_capacity_buffer, 2);
  battery_estimated_capacity_mAh = battery_estimated_capacity_buffer[1] + 256 * battery_estimated_capacity_buffer[0];
  delay(50);

  uint8_t battery_oi_mode_buffer[1];
  roomba.getSensors(PACKET_ID_OI_MODE, battery_oi_mode_buffer, 1);
  oi_mode = battery_oi_mode_buffer[0];
  delay(50);

  String battery_voltage_str;
  if (battery_voltage_mV < 1000) {
    battery_voltage_str = String(battery_voltage_mV) + " mV";  
  } else {
    battery_voltage_str = String(battery_voltage_mV / 1000) + " V";
  }
  battery_voltage_str.toCharArray(battery_voltage_send, battery_voltage_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_VOLTAGE, battery_voltage_send);
  
  String battery_voltage_mV_str = String(battery_voltage_mV);
  battery_voltage_mV_str.toCharArray(battery_voltage_mV_send, battery_voltage_mV_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_VOLTAGE_MV, battery_voltage_mV_send);
  
  String battery_current_str;
  if (battery_current_mA < 1000) {
    battery_current_str = String(battery_current_mA) + " mA";
  } else {
    battery_current_str = String(battery_current_mA / 1000) + " A";
  }
  battery_current_str.toCharArray(battery_current_send, battery_current_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_CURRENT, battery_current_send);
  
  String battery_current_mA_str = String(battery_current_mA);
  battery_current_mA_str.toCharArray(battery_current_mA_send, battery_current_mA_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_CURRENT_MA, battery_current_mA_send);

  if (battery_current_mA < -200) {
    mqtt_client.publish(TOPIC_RUNNING, "yes");
  } else {
    mqtt_client.publish(TOPIC_RUNNING, "no");
  }

  String battery_temperature_str = String(battery_temperature_celsius) + " C";
  battery_temperature_str.toCharArray(battery_temperature_send, battery_temperature_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_TEMPERATURE, battery_temperature_send);
  
  String battery_temperature_celsius_str = String(battery_temperature_celsius);
  battery_temperature_celsius_str.toCharArray(battery_temperature_celsius_send, battery_temperature_celsius_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_TEMPERATURE_CELSIUS, battery_temperature_celsius_send);

  String battery_currently_charged_amount_str = String(battery_currently_charged_amount_mAh / 1000) + " Ah";
  battery_currently_charged_amount_str.toCharArray(battery_currently_charged_amount_send, battery_currently_charged_amount_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_CHARGED_AMOUNT, battery_currently_charged_amount_send);
  
  String battery_currently_charged_amount_mAh_str = String(battery_currently_charged_amount_mAh);
  battery_currently_charged_amount_mAh_str.toCharArray(battery_currently_charged_amount_mAh_send, battery_currently_charged_amount_mAh_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_CHARGED_AMOUNT_MAH, battery_currently_charged_amount_mAh_send);

  String battery_estimated_capacity_str = String(battery_estimated_capacity_mAh / 1000) + " Ah";
  battery_estimated_capacity_str.toCharArray(battery_estimated_capacity_send, battery_estimated_capacity_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_CAPACITY, battery_estimated_capacity_send);
  
  String battery_estimated_capacity_mAh_str = String(battery_estimated_capacity_mAh);
  battery_estimated_capacity_mAh_str.toCharArray(battery_estimated_capacity_mAh_send, battery_estimated_capacity_mAh_str.length() + 1);
  mqtt_client.publish(TOPIC_BATTERY_CAPACITY_MAH, battery_estimated_capacity_mAh_send);
  
  if (battery_estimated_capacity_mAh != 0) {
    int battery_currently_charged_rate_percent = 100 * battery_currently_charged_amount_mAh / battery_estimated_capacity_mAh;
    
    String battery_currently_charged_rate_str = String(battery_currently_charged_rate_percent) + " %";
    battery_currently_charged_rate_str.toCharArray(battery_currently_charged_rate_send, battery_currently_charged_rate_str.length() + 1);
    mqtt_client.publish(TOPIC_BATTERY_CHARGED_RATE, battery_currently_charged_rate_send);

    String battery_currently_charged_rate_percent_str = String(battery_currently_charged_rate_percent);
    battery_currently_charged_rate_percent_str.toCharArray(battery_currently_charged_rate_percent_send, battery_currently_charged_rate_percent_str.length() + 1);
    mqtt_client.publish(TOPIC_BATTERY_CHARGED_RATE_PERCENT, battery_currently_charged_rate_percent_send);
  } else {
    mqtt_client.publish(TOPIC_BATTERY_CHARGED_RATE, "NO DATA");
    mqtt_client.publish(TOPIC_BATTERY_CHARGED_RATE_PERCENT, "-1");
  }

  if (battery_charging_state == 0) {
    mqtt_client.publish(TOPIC_BATTERY_CHARGING, "Not charging");
  } else if (battery_charging_state == 1) {
    mqtt_client.publish(TOPIC_BATTERY_CHARGING, "Reconditioning Charging");
  } else if (battery_charging_state == 2) {
    mqtt_client.publish(TOPIC_BATTERY_CHARGING, "Full Charging");
  } else if (battery_charging_state == 3) {
    mqtt_client.publish(TOPIC_BATTERY_CHARGING, "Trickle Charging");
  } else if (battery_charging_state == 4) {
    mqtt_client.publish(TOPIC_BATTERY_CHARGING, "Waiting");
  } else if (battery_charging_state == 5) {
    mqtt_client.publish(TOPIC_BATTERY_CHARGING, "Charging Fault Condition");
  } else {
    String error_message = "INVALID data: " + String(battery_charging_state);
    error_message.toCharArray(battery_charging_state_send, error_message.length() + 1);
    mqtt_client.publish(TOPIC_BATTERY_CHARGING, battery_charging_state_send);
  }

  if (oi_mode == 0) {
    mqtt_client.publish(TOPIC_MODE, "Off");
  } else if (oi_mode == 1) {
    mqtt_client.publish(TOPIC_MODE, "Passive");
  } else if (oi_mode == 2) {
    mqtt_client.publish(TOPIC_MODE, "Safe");
  } else if (oi_mode == 3) {
    mqtt_client.publish(TOPIC_MODE, "Full");
  } else {
    String error_message = "INVALID data: " + String(oi_mode);
    error_message.toCharArray(oi_mode_send, error_message.length() + 1);
    mqtt_client.publish(TOPIC_MODE, oi_mode_send);
  }  
}

void log(String message) {
  char* message_send;
  message.toCharArray(message_send, message.length() + 1);
  log(message_send);
}

void log(char* message_send) {
  mqtt_client.publish(TOPIC_STATUS, message_send);
}

void setupHttp() {
  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpServer.begin();
  
  MDNS.addService("http", "tcp", 80);
}

void setup() {
  Serial.begin(115200);
  Serial.write(129);
  delay(50);
  Serial.write(11);
  delay(50);
  setupWiFi();
  setupMqtt();
  setupOta();
  timer.setInterval(5000, sendInfoRoomba);
}

void loop() {
  loopMqtt();
  handleArduinoOTA();
  httpServer.handleClient();
  MDNS.update();
  timer.run();
}
