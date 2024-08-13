/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef APSSID
#define APSSID "ESPap"
#define APPSK "thereisnospoon"
#endif

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

const int led = 13;

const int CE = 16;
#define DMX_NB_SLOTS 513
#define DMX_DELAY 23   //44Hz

uint8_t DmxUniverseBuffer [DMX_NB_SLOTS] = {0};

ESP8266WebServer server(80);

bool TMP_SUP (unsigned long Last, uint32_t delay)
{
  unsigned long currTick = millis();
  if(currTick > Last)
    return (currTick - Last) > delay;
  else
  return (((unsigned long)-1)-Last +1 +currTick) > delay;

}


void DmxHandler()
{
  static unsigned long LastTick = 0;
  if(TMP_SUP(LastTick, DMX_DELAY))
  {
    LastTick = millis();
    //Write break line
    noInterrupts();
    digitalWrite(CE,LOW);
    delayMicroseconds(100);
    digitalWrite(CE,HIGH);
    delayMicroseconds(1);
    interrupts();

    Serial1.write(DmxUniverseBuffer,DMX_NB_SLOTS);
    //Serial1.flush();
//    for(int i = 0; i<3; i++)
  //    Serial1.print(0x00,HEX);
    
    
  }
}

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!\r\n");
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup() {
  pinMode(led, OUTPUT);
  pinMode(CE, OUTPUT);
  digitalWrite(CE, HIGH);
  delay(1000);
  Serial.begin(115200);
  Serial1.begin(250000,SERIAL_8N2);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  

  server.on("/gif", []() {
    static const uint8_t gif[] PROGMEM = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x10, 0x00, 0x10, 0x00, 0x80, 0x01,
      0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x10, 0x00, 0x00, 0x02, 0x19, 0x8c, 0x8f, 0xa9, 0xcb, 0x9d,
      0x00, 0x5f, 0x74, 0xb4, 0x56, 0xb0, 0xb0, 0xd2, 0xf2, 0x35, 0x1e, 0x4c,
      0x0c, 0x24, 0x5a, 0xe6, 0x89, 0xa6, 0x4d, 0x01, 0x00, 0x3b
    };
    char gif_colored[sizeof(gif)];
    memcpy_P(gif_colored, gif, sizeof(gif));
    // Set the background to a random set of colors
    gif_colored[16] = millis() % 256;
    gif_colored[17] = millis() % 256;
    gif_colored[18] = millis() % 256;

    DmxUniverseBuffer[3] = ~DmxUniverseBuffer[3];

    server.send(200, "image/gif", gif_colored, sizeof(gif_colored));
  });
  server.on("/toggle_red", []() {

    DmxUniverseBuffer[2] = ~DmxUniverseBuffer[2];

    server.send(200, "text/plain","Red Toggled !!");
  });
  
  server.on("/toggle_green", []() {

    DmxUniverseBuffer[3] = ~DmxUniverseBuffer[3];

    server.send(200, "text/plain","Green Toggled !!");
  });

  server.on("/toggle_blue", []() {

    DmxUniverseBuffer[4] = ~DmxUniverseBuffer[4];

    server.send(200, "text/plain","Blue Toggled !!");
  });

  server.on("/toggle_White", []() {

    DmxUniverseBuffer[5] = ~DmxUniverseBuffer[5];

    server.send(200, "text/plain","White Toggled !!");
  });
  
  server.on("/toggle_strobe", []() {
    static char buffer[256];
    static uint8_t steps [] = {0,51,101,151,201};
    static uint8_t index = 0;
    index ++;// DmxUniverseBuffer[6]==0?49:0;
    if(index>4)index = 0;
    DmxUniverseBuffer[6] = steps[index];
    snprintf(buffer,256,"Strobe Toggled !! New Value is %i",DmxUniverseBuffer[6]);
    server.send(200, "text/plain",buffer);
  });

    server.on("/toggle_speed", []() {
    static char buffer[256];

    if (DmxUniverseBuffer[7] == 255)
      DmxUniverseBuffer[7] = 0;
    else if(DmxUniverseBuffer[7] >= 250)
      DmxUniverseBuffer[7] = 0xFF;
    else
      DmxUniverseBuffer[7] +=10;// DmxUniverseBuffer[6]==0?49:0;

    snprintf(buffer,256,"Speed Toggled !! New Value is %i",DmxUniverseBuffer[7]);
    server.send(200, "text/plain",buffer);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  DmxUniverseBuffer[1] = 0xFF;
  //DmxUniverseBuffer[2] = 0xFF;

}

void loop() {
  server.handleClient();
  DmxHandler();


}
