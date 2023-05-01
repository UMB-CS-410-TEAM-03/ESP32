#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SPIFFS.h>
#include <Servo.h>
#include <WiFiClientSecure.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <thread>

#include "at_client.h"
#include "constants.h"

using std::deque;
using std::string;
using std::to_string;

void TouchInterruptHandler();
void REButtonHandler();
void RERotateHandler();
void RECLKRotateHandler();
void REDATRotateHandler();

int door_will_open();
int door_has_opened();
int door_will_close();
int door_has_closed();
int door_is_halted();

int door_open_by_20();
int door_close_by_20();

int lcd_show_door_stat();
int lcd_show_re_stat();

int door_sync_status();
int re_sync_status();

int re_will_change();
int re_was_set();
int re_value_increased();
int re_value_decreased();

LiquidCrystal lcd(RS, RW, ENABLE, D4, D5, D6, D7);

Servo servo;

enum Event {
    SYNC_DOOR,
    SYNC_RE,
    DOOR_OPEN,
    DOOR_OPENED,
    DOOR_CLOSE,
    DOOR_CLOSED,
    DOOR_HALT,
    DOOR_OPEN_BY_20,
    DOOR_CLOSE_BY_20,
    LCD_SHOW_DOOR_STAT,
    LCD_SHOW_RE_STAT,
    RE_CHANGE,
    RE_SET,
    RE_INC,
    RE_DEC,
};

static int (*(Event_Handlers[15]))() = {
    door_sync_status,
    re_sync_status,
    door_will_open,
    door_has_opened,
    door_will_close,
    door_has_closed,
    door_is_halted,
    door_open_by_20,
    door_close_by_20,
    lcd_show_door_stat,
    lcd_show_re_stat,
    re_will_change,
    re_was_set,
    re_value_increased,
    re_value_decreased
};

class EventBus {
    deque<Event> events;

public:
    boolean empty()
    {
        return events.empty();
    }

    Event current()
    {
        return events.front();
    }

    void add(Event e)
    {
        events.push_back(e);
    }

    void sos(Event e)
    {
        events.push_front(e);
    }

    void current_completed()
    {
        events.pop_front();
    }
};

// static deque<Event> events;
static EventBus events;

enum DoorStatus {
    opened = 0,
    closed = 1,
    opening = 2,
    closing = 3
};
static DoorStatus DOOR_STATUS
    = DoorStatus::closed;

enum REStatus {
    change = 0,
    set = 1
};
static REStatus RE_STATUS = REStatus::set;
static int RE_VALUE = RE_VALUE_MAX;

static int SERVO_ANGLE = 0;

static AtClient* at_client;
static AtKey* app_events_key;
static AtKey* event_bus_key;
static AtKey* door_status_key;
static AtKey* re_value_key;
// static AtKey* door_event_updated_key;

void setup()
{
    const auto* chip = new AtSign("@moralbearbanana");
    const auto* java = new AtSign("@batmanariesbanh");

    const auto keys = keys_reader::read_keys(*chip);
    at_client = new AtClient(*chip, keys);

    // pkam authenticate into our atServer
    at_client->pkam_authenticate("hotspot", "12345678");

    app_events_key = new AtKey("app_e", java, chip);
    event_bus_key = new AtKey("event_bus", chip, java);
    door_status_key = new AtKey("door_status", chip, java);
    re_value_key = new AtKey("re_value", chip, java);

    pinMode(TOUCH_SENSOR, INPUT);
    attachInterrupt(digitalPinToInterrupt(TOUCH_SENSOR), TouchInterruptHandler, RISING);

    pinMode(RE_BUTTON, INPUT);
    attachInterrupt(digitalPinToInterrupt(RE_BUTTON), REButtonHandler, RISING);

    pinMode(RE_CLK, INPUT);
    pinMode(RE_DAT, INPUT);
    attachInterrupt(digitalPinToInterrupt(RE_CLK), RERotateHandler, FALLING);

    lcd.begin(LCD_WIDTH, LCD_HEIGHT);
    servo.attach(SERVO);

    at_client->put_ak(*event_bus_key, "");
    at_client->put_ak(*door_status_key, to_string(DoorStatus::closed));
    at_client->put_ak(*re_value_key, to_string(RE_VALUE));

      events.add(Event::LCD_SHOW_DOOR_STAT);
}

static volatile unsigned long APP_E_TIME = millis();
static volatile unsigned long TKN_TIME = millis();
static int tkn = -1;

void loop()
{
    if (events.empty()) {
        if (millis() - TKN_TIME > 30000) {
            tkn = rand() % 100;
            at_client->put_ak(*event_bus_key, std::to_string(tkn));

            TKN_TIME = millis();
        }

        if (millis() - APP_E_TIME > 15000) {
            string data = at_client->get_ak(*app_events_key);
            std::cout << "\n\n\n\nData: " << data << "\n\n\n\n";

            if (!data.empty() && data != " ") {
                int pos = data.find('z');
                int event_id = stoi(data.substr(0, pos));
                int r_tkn = stoi(data.substr(pos + 1, data.length()));

                std::cout << "\n\n\n\n\n";
                std::cout << "event_id: " << event_id << '\n';
                std::cout << "tkn: " << r_tkn << '\n';
                std::cout << "\n\n\n\n\n";

                if (r_tkn == tkn) {

                    if (event_id == 6) {
                        events.sos(Event::DOOR_HALT);
                    } else if (event_id == 2) {
                        events.add(Event::DOOR_OPEN);
                    } else if (event_id == 4) {
                        events.add(Event::DOOR_CLOSE);
                    }
                }
            }

            APP_E_TIME = millis();
        }

        return;
    }

    Event event = events.current();

    auto result = Event_Handlers[event]();

    // if (result) {
    //     boolean synced = false;
    //     at_client->put_ak(*event_bus_key, std::to_string(event));
    //     delay(2000);
    //     while (!synced) {
    //         string data = at_client->get_ak(*app_events_key);
    //         std::cout << "\n\n\n\nData: " << data << "\n\n\n\n";
    //         string value = "ok" + std::to_string(event);
    //         if (data == value) {
    //             synced = true;
    //             delay(2000);
    //             at_client->put_ak(*event_bus_key, "-1");
    //         }
    //     }
    // }

    events.current_completed();
}

void TouchInterruptHandler()
{
    switch (DOOR_STATUS) {
    case DoorStatus::closed: {
        events.add(Event::DOOR_OPEN);
        break;
    }
    case DoorStatus::opened: {
        events.add(Event::DOOR_CLOSE);
        break;
    }
    case DoorStatus::opening: {
        events.sos(Event::DOOR_HALT);
        break;
    }
    case DoorStatus::closing: {
        events.sos(Event::DOOR_HALT);
        break;
    }
    }
}

static volatile unsigned long RE_TIME = millis();

void REButtonHandler()
{
    switch (RE_STATUS) {
    case REStatus::set: {
        events.add(RE_CHANGE);
        break;
    }
    case REStatus::change: {
        events.add(RE_SET);
        break;
    }
    }
}

void RERotateHandler()
{
    if (RE_STATUS != REStatus::change) {
        return;
    }
    if (millis() - RE_TIME < 1000) {
        return;
    }

    RE_TIME = millis();

    if (digitalRead(RE_DAT)) {
        events.add(Event::RE_INC);
    } else {
        events.add(Event::RE_DEC);
    }
}

int door_will_open()
{
    std::cout << "DOOR IS OPENING\n";
    DOOR_STATUS = DoorStatus::opening;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    events.add(Event::SYNC_DOOR);

    for (int i = 0; i < RE_VALUE; i++) {
        events.add(Event::DOOR_OPEN_BY_20);
    }
    events.add(Event::DOOR_OPENED);
    return 0;
}

int door_has_opened()
{
    std::cout << "DOOR IS OPENED\n";
    DOOR_STATUS = DoorStatus::opened;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    events.add(Event::SYNC_DOOR);
    RE_VALUE = RE_VALUE_MIN;
    events.add(Event::SYNC_RE);

    return 0;
}

int door_will_close()
{
    std::cout << "DOOR IS CLOSING\n";
    DOOR_STATUS = DoorStatus::closing;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    events.add(Event::SYNC_DOOR);
    for (int i = RE_VALUE; i < RE_VALUE_MAX; i++) {
        events.add(Event::DOOR_CLOSE_BY_20);
    }
    events.add(Event::DOOR_CLOSED);
    return 0;
}

int door_has_closed()
{
    std::cout << "DOOR IS CLOSED\n";
    DOOR_STATUS = DoorStatus::closed;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    events.add(Event::SYNC_DOOR);
    RE_VALUE = RE_VALUE_MAX;
    events.add(Event::SYNC_RE);

    return 0;
}

int door_is_halted()
{
    std::cout << "DOOR HALTED\n";
    if (events.current() == Event::DOOR_HALT) {
        events.current_completed();
    }

    Event door_movement_type;

    if (DOOR_STATUS == DoorStatus::closing) {
        door_movement_type = Event::DOOR_CLOSE_BY_20;
    } else if (DOOR_STATUS == DoorStatus::opening) {
        door_movement_type = Event::DOOR_OPEN_BY_20;
    }

    while (events.current() == door_movement_type) {
        events.current_completed();
    }

    events.add(Event::SYNC_DOOR);
    events.add(Event::SYNC_RE);

    return 0;
}

int door_sync_status()
{
    at_client->put_ak(*door_status_key, std::to_string(DOOR_STATUS));
    return 0;
}

int re_sync_status()
{
    std::cout << "\n\n\n\nRE_VALUE: " << RE_VALUE << "\n\n\n\n";
    at_client->put_ak(*re_value_key, std::to_string(RE_VALUE));
    return 0;
}

int lcd_show_door_stat()
{
    const static std::map<int, std::string> DoorStatusStrings = {
        { DoorStatus::opened, " Opened " },
        { DoorStatus::closed, " Closed " },
        { DoorStatus::opening, "Opening " },
        { DoorStatus::closing, "Closing " }
    };

    lcd.setCursor(0, 0);
    lcd.write("  Door  ");
    std::string message = DoorStatusStrings.at(DOOR_STATUS);
    lcd.setCursor(0, 1);
    lcd.write(message.c_str());
    return 0;
}

int lcd_show_re_stat()
{
    lcd.setCursor(0, 0);
    lcd.write("DoorOpen");
    lcd.setCursor(0, 1);
    std::string amount = std::to_string(RE_VALUE * RE_STEP_SIZE);
    while (amount.size() <= 3) {
        amount = ' ' + amount;
    }
    std::string v = " " + amount + " %  ";
    lcd.write(v.c_str());
    return 0;
}

int door_open_by_20()
{
    SERVO_ANGLE = SERVO_ANGLE + (int)(180 / RE_VALUE_MAX);
    servo.write(SERVO_ANGLE);
    delay(1000);
    RE_VALUE -= 1;
    return 0;
}

int door_close_by_20()
{
    SERVO_ANGLE = SERVO_ANGLE - (int)(180 / RE_VALUE_MAX);
    servo.write(SERVO_ANGLE);
    delay(1000);
    RE_VALUE += 1;
    return 0;
}

int re_will_change()
{
    RE_STATUS = REStatus::change;
    events.add(Event::LCD_SHOW_RE_STAT);
    return 0;
}

int re_was_set()
{
    RE_STATUS = REStatus::set;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    return 0;
}

int re_value_increased()
{
    if (RE_VALUE < RE_VALUE_MAX) {
        events.add(Event::DOOR_CLOSE_BY_20);
        events.add(Event::LCD_SHOW_RE_STAT);
        events.add(Event::SYNC_RE);
    }
    return 0;
}

int re_value_decreased()
{
    if (RE_VALUE > RE_VALUE_MIN) {
        events.add(Event::DOOR_OPEN_BY_20);
        events.add(Event::LCD_SHOW_RE_STAT);
        events.add(Event::SYNC_RE);
    }
    return 0;
}