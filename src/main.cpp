#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <FS.h>
#include "secrets.h"

WebServer server(80);

String getContentType(const String &path)
{
  if (path.endsWith(".html"))
    return "text/html";
  else if (path.endsWith(".css"))
    return "text/css";
  else if (path.endsWith(".js"))
    return "application/javascript";
  else if (path.endsWith(".png"))
    return "image/png";
  else if (path.endsWith(".jpg"))
    return "image/jpeg";
  else if (path.endsWith(".ico"))
    return "image/x-icon";
  else if (path.endsWith(".svg"))
    return "image/svg+xml";
  else if (path.endsWith(".ttf"))
    return "font/ttf";
  return "text/plain";
}

void serveFile(const String &path)
{
  String filePath = path;
  if (filePath == "/")
    filePath = "/index.html";

  if (SPIFFS.exists(filePath))
  {
    File file = SPIFFS.open(filePath, "r");
    String contentType = getContentType(filePath);
    server.streamFile(file, contentType);
    file.close();
  }
  else
  {
    server.send(404, "text/plain", "404: Not Found");
  }
}

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []()
            { serveFile("/index.html"); });
  server.onNotFound([]()
                    { serveFile(server.uri()); });
  server.begin();

  HTTPClient http;
  http.begin("https://api.seniverse.com/v3/weather/now.json?key=" + String(SENIVERSE_API_KEY) + "&location=beijing");
  Serial.println("Requesting...");
  int httpResponseCode = http.GET();

  Serial.println("Response Code: " + String(httpResponseCode));
  if (httpResponseCode = 200)
  {
    String response = http.getString();
    JsonDocument content;
    DeserializationError error = deserializeJson(content, response);
    if (error)
    {
      Serial.print("JSON Parsing failed: ");
      Serial.println(error.c_str());
      return;
    }
    JsonDocument result = content["results"][0];
    String locationName = result["location"]["name"];
    String weatherText = result["now"]["text"];
    String temperature = result["now"]["temperature"];
    Serial.println("Got Weather:\n\tLocation: " + locationName + ", Weather: " + weatherText + ", Temperature: " + temperature + "°C");
  }
  else
  {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void loop()
{
  server.handleClient();
}