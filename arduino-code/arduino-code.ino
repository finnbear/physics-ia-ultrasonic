#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SparkFunTSL2561.h>
#include <Wire.h>

ESP8266WebServer server(80);

SFE_TSL2561 sensor;

const int ledPin = 13;

void handleNotFound(){
  digitalWrite(ledPin, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(ledPin, 0);
}

boolean gain = 0;
unsigned char timing = 2;
unsigned int saturation = 0;

double sample() {
  unsigned int duration;
  
  sensor.setTiming(gain, timing, duration);
  sensor.setPowerUp();

  delay(duration + 10);
  
  unsigned int data0;
  unsigned int data1;
  
  if (sensor.getData(data0, data1)) {
    double lux;

    saturation = 1 - sensor.getLux(gain, duration, data0, data1, lux);
    
    if (saturation == 0) {
      Serial.println(lux);
      
      return lux;
    }
  }

  sensor.setPowerDown();

  return -1;
}

void setup(void){
  pinMode(ledPin, OUTPUT);
  
  digitalWrite(ledPin, 0);
  
  Serial.begin(9600);
  WiFi.begin();
  sensor.begin();
  
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  
  Serial.println("Connected to wifi");
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", [](){
    digitalWrite(ledPin, 1);
    
    float lux = sample();
    
    Serial.print(lux);
    Serial.println(" lux");
    
    server.send(200, "text/plain", String(lux) + " lux");
    
    digitalWrite(ledPin, 0);
  });

  server.on("/error", [](){
    String error;
    
    switch(sensor.getError()) {
      case 0:
        error = "OK";
        break;
      case 1:
        error = "BUFF";
        break;
      case 2:
        error = "CONN";
        break;
      case 3:
        error = "NACK";
        break;
      case 4:
        error = "ERR";
        break;
      default:
        error = "UNK";
    }
    
    server.send(200, "text/plain", error + ", " + String(saturation));
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
