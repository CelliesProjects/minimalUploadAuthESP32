/****************************************************************************

       POC to upload a file to esp32 with authorization

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
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);
    Serial.printf("Connecting to SSID %s with PSK %s...\n", ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    static AsyncWebServer server(80);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", upload_htm, upload_htm_len);
        response->addHeader("Server", "ESP Async Web Server");
        request->send(response);
    });

    // preflight cors check
    server.on("/api/upload", HTTP_OPTIONS, [](AsyncWebServerRequest * request)
    {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Accept, Content-Type, Authorization, FileSize");
        response->addHeader("Access-Control-Allow-Credentials", "true");
        request->send(response);
    });

    server.on("/api/upload", HTTP_POST, [](AsyncWebServerRequest * request)
    {
        if (request->authenticate(http_username, http_password))
            request->send(200);
        else {
            request->send(401);
            request->client()->close();
        }
    },
    [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final)
    {
        if (!request->authenticate(http_username, http_password)) {
            request->send(401);
            request->client()->close();
            return;
        }

        // https://javascript.info/formdata

        static time_t startTimer;
        if (!index) {
            startTimer = millis();
            const char* fileSizeHeader{"FileSize"};
            Serial.printf("UPLOAD: %s starting upload\n", filename.c_str());
            if (request->hasHeader(fileSizeHeader)) {
                Serial.printf("UPLOAD: fileSize:%s\n", request->header(fileSizeHeader));
                if (request->header(fileSizeHeader).toInt() >= MAX_FILESIZE) {
                    request->send(400, "text/html",
                                  "Too large. (" + humanReadableSize(request->header(fileSizeHeader).toInt()) +
                                  ") Max size is " + humanReadableSize(MAX_FILESIZE) + ".");

                    request->client()->close();
                    Serial.printf("UPLOAD: Aborted upload of '%s' because file too big.\n", filename.c_str());
                    return;
                }
            }
        }
        //Store or do something with the data...
        //Serial.printf("file: '%s' received %i bytes\ttotal: %i\n", filename.c_str(), len, index + len);

        if (final)
            Serial.printf("UPLOAD: %s Done. Received %i bytes in %.2fs which is %.2f kB/s.\n", filename.c_str(),
                          index + len,
                          (millis() - startTimer) / 1000.0,
                          1.0 * (index + len) / (millis() - startTimer));
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

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.begin();

    Serial.print("Upload files at ");
    Serial.println(WiFi.localIP());
}

void loop() {
    delay(1000);
}
