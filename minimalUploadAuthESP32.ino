/****************************************************************************

       POC to upload a file to esp32 with authorization

       Browse to your esp32 to upload files

 *****************************************************************************/
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "upload_htm.h"

const char* WIFI_SSID     = "----------";
const char* WIFI_PASSWORD = "----------";
const char* HTTP_USERNAME = "admin";
const char* HTTP_PASSWORD = "admin";
const size_t MAX_FILESIZE = 1024 * 1024 * 15;

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
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to WIFI_SSID %s with PSK %s...\n", WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    static AsyncWebServer server(80);
    static const char* MIMETYPE_HTML{"text/html"};

    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        AsyncWebServerResponse *response = request->beginResponse_P(200, MIMETYPE_HTML, upload_htm, upload_htm_len);
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
        if (request->authenticate(HTTP_USERNAME, HTTP_PASSWORD))
            request->send(200);
        else {
            request->send(401);
            request->client()->close();
        }
    },
    [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final)
    {
        if (!request->authenticate(HTTP_USERNAME, HTTP_PASSWORD)) {
            request->send(401);
            request->client()->close();
            return;
        }

        // https://javascript.info/formdata

        static time_t startTimer;
        if (!index) {
            startTimer = millis();
            const char* FILESIZE_HEADER{"FileSize"};
            Serial.printf("UPLOAD: Receiving: '%s'\n", filename.c_str());
            if (request->hasHeader(FILESIZE_HEADER)) {
                Serial.printf("UPLOAD: fileSize: %s\n", request->header(FILESIZE_HEADER));
                if (request->header(FILESIZE_HEADER).toInt() >= MAX_FILESIZE) {
                    request->send(400, MIMETYPE_HTML,
                                  "Too large. (" + humanReadableSize(request->header(FILESIZE_HEADER).toInt()) +
                                  ") Max size is " + humanReadableSize(MAX_FILESIZE) + ".");

                    request->client()->close();
                    Serial.printf("UPLOAD: Aborted upload because filesize limit.\n");
                    return;
                }
            } else {
                request->send(400, MIMETYPE_HTML, "No filesize header present!");
                request->client()->close();
                Serial.printf("UPLOAD: Aborted upload because missing filesize header.\n");
                return;
            }
        }
        //Store or do something with the data...
        //Serial.printf("file: '%s' received %i bytes\ttotal: %i\n", filename.c_str(), len, index + len);

        if (final)
            Serial.printf("UPLOAD: Done. Received %i bytes in %.2fs which is %.2f kB/s.\n",
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
        request->send(404, MIMETYPE_HTML, "404 - Not found.");
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.begin();

    Serial.print("Upload files at ");
    Serial.println(WiFi.localIP());
}

void loop() {
    delay(1000);
}
