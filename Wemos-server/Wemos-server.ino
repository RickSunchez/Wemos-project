#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

#define MODULE_COUNT 3

struct SmartModule {
  bool isOnline;
  String whoami;
  String clientAddress;
  String output;
  int methodsCount;
  String methods[5][2];
};

// Статические параметры сервера
const char* ssid = "Smart-home";
const char* password = "12341234";

IPAddress ip(192,168,0,100);
IPAddress gw(192,168,0,1);
IPAddress sn(255,255,255,0);

ESP8266WebServer server(80);

// Параметры подключаемых модулей
SmartModule MODULES[MODULE_COUNT] = {
  (SmartModule) {false, "LIGHT-MODULE", "", "", 3, 
    { {"/led/on", "ON"}, {"/led/off", "OFF"}, {"/led/status", "STATUS"} } 
  }, 
  (SmartModule) {false, "DHT-MODULE", "", "", 2,
    { {"/get/humiduty", "HUMIDUTY"}, {"/get/temp", "TEMPERATURE"} }
  },   
  (SmartModule) {false, "DOOR-MODULE", "", "", 1,
    { {"/get/status", "STATUS"} }
  }
};

HTTPClient http;
WiFiClient cli;

void setup() {
// Раскомментировать в случае, если сервер рабоатал и перестал
//  SPIFFS.format();
//  ESP.reset();
  
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, gw, sn);
  WiFi.softAP(ssid, password);

  Serial.println("\n\rRunning");
  
  server.begin();

  // Пути веб-сервера
  server.on("/", []() {
    server.send(200, "text/html", rootPage());  
  });

  server.on("/do", []() {
    int id;
    int action;
    
    for (int i=0; i<server.args(); i++) {
      if (server.argName(i) == "id") id = server.arg(i).toInt();
      if (server.argName(i) == "action") action = server.arg(i).toInt();
    }

    if (MODULES[id].isOnline) {
      String clientIP = MODULES[id].clientAddress + MODULES[id].methods[action][0];

      http.begin(cli, clientIP);

      int respCode = http.GET();

      if (respCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(respCode);
        
        String payload = http.getString();
        payload.trim();

        MODULES[id].output = payload;
        
        Serial.println(payload);
      } else {
        Serial.print("Error code: ");
        Serial.println(respCode);
        
        MODULES[id].output = respCode;
      }
      
      http.end();
      cli.stop();
    }

    server.send(200, "text/html", rootPage());
  });

  server.on("/init", []() {
    String clientIP = "http://" + server.client().remoteIP().toString();

    http.begin(cli, clientIP + "/whoami");
    
    int respCode = http.GET();

    if (respCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(respCode);
      
      String payload = http.getString();
      payload.trim();

      for (int i=0; i<MODULE_COUNT; i++) {
        if (MODULES[i].whoami == payload) {
          MODULES[i].isOnline = true;
          MODULES[i].clientAddress = clientIP;
        }
      }
      
      Serial.println(payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(respCode);
    }
    
    http.end();
    cli.stop();
  });
}

void loop() {
  server.handleClient();
}
// Главная страница
String rootPage() {
  String table = "<table>";
  table += "<tr><th>ID</th><th>whoami</th><th>isOnline</th><th>clientIP</th><th>Commands</th><th>Output</th></tr>";

  for (int i=0; i<MODULE_COUNT; i++) {
    table += "<tr><td>" + String(i) + "</td>";
    table += "<td>" + String(MODULES[i].whoami) + "</td>";
    table += "<td>" + String(MODULES[i].isOnline) + "</td>";

    if (MODULES[i].isOnline) {
      table += "<td>" + String(MODULES[i].clientAddress) + "</td>";
    } else {
      table += "<td>-</td>";
    }

    table += "<td>";

    for (int j=0; j<MODULES[i].methodsCount; j++) {
      table += "<a href='do\?id=" + String(i);
      table += "&action=" + String(j) + "'>";
      table += "<button>" + MODULES[i].methods[j][1] + "</button></a>";
    }

    table += "</td><td>" + String(MODULES[i].output) + "</td>";
    table += "</tr>";
  }

  table += "</table>";

  return table;
}
