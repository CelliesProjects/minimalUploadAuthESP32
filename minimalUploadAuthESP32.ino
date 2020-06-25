/****************************************************************************

       Upload a file to esp32 with authorization

       Browse to your esp32 to upload files

 *****************************************************************************/
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "upload_htm.h"

const char* ssid          = "----------";
const char* password      = "----------";
const char* http_username = "admin";
const char* http_password = "admin";
const size_t MAX_FILESIZE = 1024 * 1024 * 5;

AsyncWebServer server(80);

/* format bytes as KB, MB or GB string */
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to SSID %s with PSK %s...\n", ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", upload_htm, upload_htm_len);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });

  server.on("/api/upload", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    if (request->authenticate(http_username, http_password))
      request->send(200);
    else
      request->requestAuthentication();
  },
  [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final)
  {
    if (!request->authenticate(http_username, http_password))
      request->requestAuthentication();

    static time_t startTimer;
    if (!index) {
      startTimer = millis();
      Serial.printf("UPLOAD: %s starting upload\n", filename.c_str());
      if (request->hasHeader("Content-Length")) {
        Serial.printf("UPLOAD: Content-Length:%s\n", request->header("Content-Length"));
        if (request->header("Content-Length").toInt() > MAX_FILESIZE) {
          char content[70];
          snprintf(content, sizeof(content), "File too large. (%s)<br>Max upload size is %s", humanReadableSize(request->header("Content-Length").toInt()), humanReadableSize(MAX_FILESIZE));
          request->send(400, "text/html", content);
          request->client()->close();
          Serial.printf("UPLOAD: Aborted upload of '%s' because file too big.\n", filename.c_str());
          return;
        }
      }
    }

    //Store or do something with the data...
    //Serial.printf("file: '%s' %i bytes\ttotal: %i\n", filename.c_str(), len, index + len);

    if (final)
      Serial.printf("UPLOAD: %s Done.\nReceived %.2f kBytes in %.2fs which is %.2f kB/s.\n", filename.c_str(), index / 1024.0, (millis() - startTimer) / 1000.0, 1.0 * index / (millis() - startTimer));
  });

  server.onNotFound([](AsyncWebServerRequest * request)
  {
    Serial.printf("NOT_FOUND: ");
    if (request->method() == HTTP_GET)
      Serial.printf("GET");
    else if (request->method() == HTTP_POST)
      Serial.printf("POST");
    else if (request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if (request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if (request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if (request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if (request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
    request->send(404, "text/plain", "Not found.");
  });

  server.begin();

  Serial.print("Upload files at ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}
