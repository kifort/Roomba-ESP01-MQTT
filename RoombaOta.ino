#include <ArduinoOTA.h>

void setupOta() {
  ArduinoOTA.setHostname(host);
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    log("OTA - Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    log("OTA - End");
    log("ESP - Restarting");
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    String progress_percentage = String(progress / (total / 100)) + " %";
    log("OTA - Progress: " + progress_percentage);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {  
      log("OTA - Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      log("OTA - Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      log("OTA - Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      log("OTA - Receive Failed");
    } else if (error == OTA_END_ERROR) {
      log("OTA - End Failed");
    }
  });
  ArduinoOTA.begin();
  log("OTA - Ready");
  //log("OTA - IP address: " + WiFi.localIP());
}

void handleArduinoOTA() {
  ArduinoOTA.handle();
}
