#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

#define LED 2

// Параметры подключения
const char* ssid = "Smart-home";
const char* password = "12341234";
const String serverIP = "http://192.168.0.100";

ESP8266WebServer server(80);

HTTPClient http;
WiFiClient cli;

bool isOn = true;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, isOn);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");  
  }

  Serial.println("\n\rConnected");
  
  server.begin();

  // Пути клиента
  server.on("/whoami", []() {
    String IP = server.client().remoteIP().toString();
    
    server.send(200, "text/html", "LIGHT-MODULE");
  });

  server.on("/led/on", []() {
    isOn = true;
    server.send(200, "text/html", "OK");
  });
  server.on("/led/off", []() {
    isOn = false;
    server.send(200, "text/html", "OK");
  });
  server.on("/led/status", []() {
    server.send(200, "text/html", String(isOn));
  });
}

bool onStart = true;

void loop() {
  server.handleClient();
  digitalWrite(LED, isOn);

  // Инициирующий запрос
  if (onStart) {
    Serial.println("Send init request");
    
    onStart = false;

    http.begin(cli, serverIP + "/init");
    int respCode = http.GET();

    if (respCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(respCode);
      
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(respCode);
    }
    
    http.end();
    cli.stop();
  }
}
