/*
 * IRremoteESP8266: IRServer - demonstrates sending IR codes controlled from a webserver
 * Version 0.3 May, 2019
 * Version 0.2 June, 2017
 * Copyright 2015 Mark Szabo
 * Copyright 2019 David Conran
 *
 * An IR LED circuit *MUST* be connected to the ESP on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */
#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif  // ESP8266
#if defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#endif  // ESP32
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WiFiClient.h>
#include "settings.hpp"

MDNSResponder mdns;
ESP8266WebServer server(80);

const uint16_t kIrLed = 4;  // ESP GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

void handleRoot(uint32_t addr, uint32_t cmd) {
  server.send(200, "text/html",
              "<html>" \
                "<head><title>" HOSTNAME " Demo </title>" \
                "<meta http-equiv=\"Content-Type\" " \
                    "content=\"text/html;charset=utf-8\">" \
                "<meta name=\"viewport\" content=\"width=device-width," \
                    "initial-scale=1.0,minimum-scale=1.0," \
                    "maximum-scale=5.0\">" \
                "</head>" \
                "<body>" \
                  "<h1>Hello from " HOSTNAME ", you can send NEC encoded IR" \
                      "signals from here!</h1>" \
                      "Sent addr: " + String(addr, HEX) + "; cmd: " + String(cmd, HEX) + \
                      "<p><a href=\"ir?addr=A55C&cmd=A5C0\">BD - A55C A5C0</a></p>" \
                      "<p><a href=\"ir?cmd=A585\">DVD - A585</a></p>" \
                      "<p><a href=\"ir?cmd=A589\">DVR/BDR - A589</a></p>" \
                      "<p><a href=\"ir?cmd=A516\">Video 1 - A516</a></p>" \
                      "<p><a href=\"ir?cmd=A51C\">Power Toggle - A51C</a></p>" \
                      "<p><a href=\"ir?cmd=A51A\">Power On - A51A?</a></p>" \
                      "<p><a href=\"ir?cmd=A51B\">Power Off - A51B?</a></p>" \
                      "<p><a href=\"ir?cmd=A512\">Mute Toggle - A512</a></p>" \
                      "<p><a href=\"ir?cmd=A551\">Mute On - A551?</a></p>" \
                      "<p><a href=\"ir?cmd=A552\">Mute Off - A552?</a></p>" \
                      "<p><a href=\"ir?cmd=A50A\">Volume Up - A50A?</a></p>" \
                      "<p><a href=\"ir?cmd=A50B\">Volume Down - A50B?</a></p>" \
                "</body>" \
              "</html>");
}

void handleIr() {
  uint32_t addr = 0;
  uint32_t cmd = 0;
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "addr") {
      addr = strtoul(server.arg(i).c_str(), NULL, 16);
    }

    if(server.argName(i) == "cmd") {
      cmd = strtoul(server.arg(i).c_str(), NULL, 16);
    }
  }

  if(cmd > 0) {
    if(addr > 0) {
      irsend.sendPioneer(irsend.encodePioneer(addr, cmd), 64, 0);
    }
    else {
      irsend.sendPioneer(irsend.encodePioneer(addr, cmd), 32, 1);
    }
  }

  handleRoot(addr, cmd);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}

void setup(void) {
  irsend.begin();

  Serial.begin(115200);
  WiFi.begin(kSsid, kPassword);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(kSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());

#if defined(ESP8266)
  if (mdns.begin(HOSTNAME, WiFi.localIP())) {
#else  // ESP8266
  if (mdns.begin(HOSTNAME)) {
#endif  // ESP8266
    Serial.println("MDNS responder started");
    // Announce http tcp service on port 80
    mdns.addService("http", "tcp", 80);
  }

  server.on("/", []() { handleRoot(0, 0); });
  server.on("/ir", handleIr);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
#if defined(ESP8266)
  mdns.update();
#endif
  server.handleClient();
}
