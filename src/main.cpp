#include <Arduino.h>
#include "DHTesp.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>

#include "../lib/WiFiAuth.h" //remove this line

//  Connect DHT sensor to GPIO 18
#define SENSORPIN 18
// wifi details
#ifndef STASSID
#define STASSID "Wifi name"
#define STAPSK "Password"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

// TCP server at port 80 will respond to HTTP requests
WiFiServer server(80);
DHTesp dht;

void setup()
{
  Serial.begin(115200);


  // Connect to WiFi network–
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up mDNS responder:
  if (!MDNS.begin("esptemp"))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  //temperature sensor setup
  dht.setup(SENSORPIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 18

}

void loop()
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client)
  {
    return;
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while (client.connected() && !client.available())
  {
    delay(1);
  }

  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');

  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1)
  {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return;
  }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);

  String s;
  if (req == "/")
  {
    delay(dht.getMinimumSamplingPeriod());
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    //if we have reading from sensor
    if (!isnan(temperature))
    {
      //send json
      s = F("HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\n\r\n");
      s += "{\"temperature\":";
      s += String(temperature, 3);
      s += ",\"humidity\":";
      s += String(humidity, 3);
      s += "}\r\n\r\n";
      Serial.println("Sending 200");
    }
    else
    {
      s = "503 Service Unavailable\r\n\r\n";
      Serial.println("Sending 503");
    }
  }
  else
  {
    s = "HTTP/1.1 404 Not Found\r\n\r\n";
    Serial.println("Sending 404");
  }
  client.print(s);

  Serial.println("Done with client");

  // delay(10);
}
