#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>
#include <iostream>
#include <queue>
#include <string>
#include <thread>

#include "at_client.h"
#include "constants.h"

using std::queue;

// TOUCH SENSOR
#define TOUCH_SENSOR 15 // INPUT
// LCD
#define LCD_WIDTH 8
#define LCD_HEIGHT 2
#define RS 4 // OUTPUT
#define RW 0 // OUTPUT
#define ENABLE 16 // OUTPUT
#define D4 17 // OUTPUT
#define D5 5 // OUTPUT
#define D6 18 // OUTPUT
#define D7 19 // OUTPUT

LiquidCrystal lcd(RS, RW, ENABLE, D4, D5, D6, D7);

enum Message {
    DOOR_OPEN,
    DOOR_CLOSED,
    DOOR_OPENING,
    DOOR_CLOSING,
    DOOR_STOP,
    DOOR_OPEN_LIMIT
};

enum DoorStatus {
    opened,
    closed,
    opening,
    closing
};

static DoorStatus DOOR_STATUS = DoorStatus::closed;

static queue<Message>
    operations;

static AtClient* at_client;
static AtKey* door_event_key;

void TouchInterruptHandler();

void setup()
{
    const auto* chip = new AtSign("@moralbearbanana");
    const auto* java = new AtSign("@batmanariesbanh");

    const auto keys = keys_reader::read_keys(*chip);
    at_client = new AtClient(*chip, keys);

    // pkam authenticate into our atServer
    at_client->pkam_authenticate("hotspot", "12345678");

    door_event_key = new AtKey("door_event", chip, java);

    pinMode(TOUCH_SENSOR, INPUT);
    attachInterrupt(digitalPinToInterrupt(TOUCH_SENSOR), TouchInterruptHandler, RISING);
    lcd.begin(8, 2);
}

void loop()
{
    if (!operations.empty()) {
        Message m = operations.front();
        if (m == Message::DOOR_OPENING) {
            std::cout << "DOOR IS OPENING\n";
            at_client->put_ak(*door_event_key, "DOOR_OPENING");

            delay(8000);
            DOOR_STATUS = DoorStatus::opened;
            operations.push(Message::DOOR_OPEN);
        }

        if (m == Message::DOOR_CLOSING) {
            std::cout << "DOOR IS CLOSING\n";
            at_client->put_ak(*door_event_key, "DOOR_CLOSING");

            delay(5000);
            DOOR_STATUS = DoorStatus::opened;
            operations.push(Message::DOOR_CLOSED);
        }

        if (m == Message::DOOR_OPEN) {
            std::cout << "DOOR IS OPEN\n";
            at_client->put_ak(*door_event_key, "DOOR_OPENED");
        }

        if (m == Message::DOOR_CLOSED) {
            std::cout << "DOOR IS OPEN\n";
            at_client->put_ak(*door_event_key, "DOOR_CLOSED");
        }
        operations.pop();
    }
}

void TouchInterruptHandler()
{
    switch (DOOR_STATUS) {
    case DoorStatus::closed: {
        operations.push(Message::DOOR_OPENING);
        DOOR_STATUS = DoorStatus::opening;
        break;
    }
    case DoorStatus::opened: {
        operations.push(Message::DOOR_CLOSING);
        DOOR_STATUS = DoorStatus::closing;
        break;
    }
    case DoorStatus::opening: {
        operations.push(Message::DOOR_STOP);
        DOOR_STATUS = DoorStatus::opened;
        break;
    }
    case DoorStatus::closing: {
        operations.push(Message::DOOR_STOP);
        DOOR_STATUS = DoorStatus::opened;
        break;
    }
    default:
        break;
    }
}