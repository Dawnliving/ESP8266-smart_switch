#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>

const char* ssid = "Your wifi ssid";
const char* password = "wifi password";
const char* mqttServer = "MQTT server ip";
const int mqttPort = 1883;
const char* mqttUser = "MQTT server username";
const char* mqttPassword = "MQTT server password";
const char* controlTopic = "smart-switch/control";
const char* statusTopic = "smart-switch/status";

# define switchPin 0  // GPIO0 on ESP-01S

bool switchState = false;

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  pinMode(switchPin, OUTPUT);
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set up MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // Set up Web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/on", HTTP_GET, handleSwitchOn);
  server.on("/off", HTTP_GET, handleSwitchOff);

  server.begin();
}

void loop() {
  server.handleClient();

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, controlTopic) == 0) {
    // Control topic
    String message = "";
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }

    if (message == "ON") {
      switchState = true;
    } else if (message == "OFF") {
      switchState = false;
    }

    updateSwitch();
  } else if (strcmp(topic, statusTopic) == 0) {
    // Status topic
    String statusMessage = switchState ? "ON" : "OFF";
    client.publish(statusTopic, statusMessage.c_str());
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT");
      client.subscribe(controlTopic);
      client.subscribe(statusTopic);
      updateSwitch(); // Ensure the switch state is sent when MQTT is reconnected
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void handleRoot() {
  String statusMessage = switchState ? "ON" : "OFF";
  server.send(200, "text/plain", "Current switch state: " + statusMessage);
}

void handleSwitchOn() {
  switchState = true;
  updateSwitch();
  server.send(200, "text/plain", "Switch turned ON");
}

void handleSwitchOff() {
  switchState = false;
  updateSwitch();
  server.send(200, "text/plain", "Switch turned OFF");
}

void updateSwitch() {
  digitalWrite(switchPin, switchState ? HIGH : LOW);
  
  Serial.println("power：");
  Serial.println(switchState ? HIGH : LOW);
  Serial.println("foot：");
  Serial.println(digitalRead(switchPin));
  // Publish the current switch state to MQTT
  client.publish(statusTopic, switchState ? "ON" : "OFF");
}