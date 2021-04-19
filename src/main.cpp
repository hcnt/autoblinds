#include "blind.h"
#include "pass.h" //imports ssid and password to wifi
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

#define OPEN_BLINDS_HOUR 9
#define CLOSE_BLINDS_HOUR 20

int blind1upPin = D6;
int blind1downPin = D5;

int blind2upPin = D8;
int blind2downPin = D7;

int blind1speedPin = D4;
int blind2speedPin = D3;

int blind1encoderPin = D1;
int blind2encoderPin = D2;

Blind blind1(blind1upPin, blind1downPin, blind1speedPin, blind1encoderPin, 150);
Blind blind2(blind2upPin, blind2downPin, blind2speedPin, blind2encoderPin, 150);

void ICACHE_RAM_ATTR checkWheelSpeed1() { blind1.incrementWheelTic(); }
void ICACHE_RAM_ATTR checkWheelSpeed2() { blind2.incrementWheelTic(); }

BlindMovement blindMovement1(blind1, 27500);
BlindMovement blindMovement2(blind2, 27500);

int clientTriesCounter = 0;
ESP8266WebServer server(80);
int desiredSpeed = 150;
const int TEMPLATE_BUFFER_SIZE = 2048;
char templateBuffer[TEMPLATE_BUFFER_SIZE];

void blink() {
    pinMode(LED_BUILTIN, HIGH);
    delay(50);
    pinMode(LED_BUILTIN, LOW);
    delay(50);
    pinMode(LED_BUILTIN, HIGH);
    delay(50);
    pinMode(LED_BUILTIN, LOW);
}

void connectToWiFi() {
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        blink();
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
}
void setupTimeClient() {
    timeClient.begin();
    timeClient.setTimeOffset(7200);
    timeClient.update();
    String formattedTime = timeClient.getFormattedTime();
    Serial.print("Formatted Time: ");
    Serial.println(formattedTime);
}

void renderMainPage() {
    snprintf(templateBuffer, TEMPLATE_BUFFER_SIZE, "<html>\
                <head>\
                    <meta charset=\"utf-8\">\
                    <meta http-equiv='refresh' content='3'/>\
                </head>\
                <body>\
                    STEROWANIE SUPER ROLETAMI\
                    \
                    <br><br>\
                    prędkość: %d\
                    <br><br>\
                    \
                    ROLETA 1\
                    stan: %d\
                    <br>\
                    długość ruchu: %d\
                    <br><br>\
                    <a href=\"/move?dir=F\">OTWÓRZ</a><br>\
                    <a href=\"/move?dir=B\">ZAMKNIJ</a><br>\
                    <a href=\"/move?dir=S\">STOP</a><br>\
                    <br>\
                    \
                    ROLETA 2\
                    stan: %d\
                    <br>\
                    długość ruchu: %d\
                    <br><br>\
                    <a href=\"/move?dir=L\">OTWÓRZ</a><br>\
                    <a href=\"/move?dir=R\">ZAMKNIJ</a><br>\
                    <a href=\"/move?dir=S\">STOP</a><br>\
                </body>\
            </html>",
             desiredSpeed, blind1.getState(), blind1.getMovementDuration(),
             blind2.getState(), blind2.getMovementDuration());
}

void serverHandleMove() {
    String action = server.arg("dir");
    if (action == "F") {
        blind1.go(UP);
    } else if (action == "B") {
        blind1.go(DOWN);
    } else if (action == "S") {
        blind1.go(STOP);
    }
    if (action == "L") {
        blind2.go(UP);
    } else if (action == "R") {
        blind2.go(DOWN);
    } else if (action == "S") {
        blind2.go(STOP);
    }
    renderMainPage();
    server.send(200, "text/html", templateBuffer);
}

void serverHandleAction() {
    String action = server.arg("type");
    if (action == "4") {
        blindMovement1.move(UP);
    } else if (action == "8") {
        blindMovement1.move(DOWN);
    } else if (action == "3") {
        blind1.resetState();
    }
    if (action == "1") {
        blindMovement2.move(UP);
    } else if (action == "5") {
        blindMovement2.move(DOWN);
    } else if (action == "2") {
        blind2.resetState();
    }
    renderMainPage();
    server.send(200, "text/html", templateBuffer);
}

void setup() {
    Serial.begin(9600);

    connectToWiFi();

    // Start the server
    server.on("/move", serverHandleMove);
    server.on("/action", serverHandleAction);
    server.on("/", []() {
        renderMainPage();
        server.send(200, "text/html", templateBuffer);
    });
    server.begin();
    Serial.println("Server started");

    // Setup time client
    setupTimeClient();

    // Print the IP address
    Serial.print("Use this URL : ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");

    // init blinds
    blind1.init();
    blind2.init();
    attachInterrupt(blind1encoderPin, checkWheelSpeed1, CHANGE);
    attachInterrupt(blind2encoderPin, checkWheelSpeed2, CHANGE);

    pinMode(A0, INPUT);
}

void loop() {
    // Check time from web
    timeClient.update();
    blindMovement1.handleMovement();
    blindMovement2.handleMovement();
    blind1.handleMove();
    blind2.handleMove();

    // Auto blind control
    if (timeClient.getHours() == OPEN_BLINDS_HOUR && timeClient.getMinutes() == 0 &&
        timeClient.getSeconds() == 0) {
        blindMovement1.move(UP);
        blindMovement2.move(UP);
    }

    else if (timeClient.getHours() == CLOSE_BLINDS_HOUR && timeClient.getMinutes() == 0 &&
             timeClient.getSeconds() == 0) {
        blindMovement1.move(DOWN);
        blindMovement2.move(DOWN);
    }

    // int potentiometerInput = analogRead(A0);
    // int desiredSpeed = 150; // map(potentiometerInput, 0, 1024, 30, 150);
    // blind1.setSpeed(desiredSpeed);
    // blind2.setSpeed(desiredSpeed);

    server.handleClient();
}
