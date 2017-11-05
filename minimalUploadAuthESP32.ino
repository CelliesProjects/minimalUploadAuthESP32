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

AsyncWebServer server(80);
 
void setup() {
  Serial.begin( 115200 );

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf( "Connecting to SSID %s with PSK %s...\n", ssid, password  );
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  server.on( "/", HTTP_GET, []( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", upload_htm, upload_htm_len);
    response->addHeader("Server","ESP Async Web Server");
    request->send(response);
  });
  
  server.on( "/api/upload", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( request->authenticate( http_username, http_password ) )
    {
      request->send( 200 );
    }
    else
    {
      request->requestAuthentication();
    }
  },
  []( AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final )
  {
    static bool   _authenticated;
    static time_t startTimer;

    if ( !index )
    {
      _authenticated = false;
      if ( request->authenticate(http_username, http_password) )
      {
        startTimer = millis();
        Serial.printf( "UPLOAD: Started to receive '%s'.\n", filename.c_str() );
        _authenticated = true;
      }
      else
      {
        Serial.println( "Unauthorized access." );
        return request->send( 401, "text/plain", "Not logged in." );
      }      
    }

    if ( _authenticated )
    {
      //Serial.printf( "%i bytes received.\n", index );
      //Store or do something with the data...
    }
    
    if ( final && _authenticated )
    {
      Serial.printf( "UPLOAD: Done. Received %.2f kBytes in %.2fs which is %i kB/s.\n", index / 1024.0, ( millis() - startTimer ) / 1000.0, index / ( millis() - startTimer ) );
    }
  });

  server.onNotFound( []( AsyncWebServerRequest * request )
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
    request->send( 404, "text/plain", "Not found." );
  });

  server.begin();

  Serial.print( "Upload files at " );
  Serial.println( WiFi.localIP() );
}

void loop() {
  // put your main code here, to run repeatedly:

}
