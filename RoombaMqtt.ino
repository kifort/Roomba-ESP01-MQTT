void callback(char* topic, byte* payload, unsigned int length) {
  String topic_str = topic;
  payload[length] = '\0';
  String payload_str = String((char *)payload);
  if (topic_str == TOPIC_COMMANDS) {
    if (payload_str == "start") {
      startCleaning();
    } else if (payload_str == "stop") {
      stopCleaning();
    } else if (payload_str == "wake up") {
      wakeUp();
    } else if (payload_str == "sleep") {
      sleep();
    } else {
      log("Unexpected command received: " + payload_str);
    }
  }
}

void reconnect()
{
  // Loop until we're reconnected
  int retries = 0;
  while (!mqtt_client.connected())
  {
    if (retries < 50) {
      // Attempt to connect
      if (mqtt_client.connect(mqtt_client_name, mqtt_user, mqtt_pass, TOPIC_STATUS, 0, 0, "Unknown"))
      {
        // Once connected, publish an announcement...
        if (boot == true) {
          boot = false;
          log("Rebooted");
        } else {
          log("Reconnected");
        }
        // ... and resubscribe
        mqtt_client.subscribe(TOPIC_COMMANDS);
      } else {
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    } else {
      ESP.restart();
    }
  }
}

void setupMqtt() {
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(callback);
}

void loopMqtt() {
  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();
}
