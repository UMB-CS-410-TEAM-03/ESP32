/**
 * Author: Malav Patel
 * File: main.cpp
 * Purpose: This file contains the all the code that
 * is to be executed on the ESP32 device for the project
 * of Remote Controlled GarageDoor.
 *
 * The file "constants.h" contains Macro definations that
 * will tie the Pins on ESP32 to Pin Number form the ESP32 pinout.
 *
 */

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

// This import includes the Macros definations that maps the pins
// to the a number indicated by the Pin Manual of ESP32.
// It also includes the Wifi details
#include "constants.h"

#include "event_bus.h"

using std::deque;
using std::string;
using std::to_string;

//-------------- GLOBAL VARIABLES ------------------------------------------//

/**
 * An Enum mapping the events that can occur or are generated to
 * handle the input and output from various pin on EPS32 or from
 * reading the at_sign secondary server.
 */
enum Event {
    // Sync the status of Door with at_sign secondary server
    SYNC_DOOR,
    // Sync the RE_VALUE with at_sign secondary server
    SYNC_RE,
    // Occurs when we are required to open the door
    DOOR_OPEN,
    // Occurs when the door has been opened
    DOOR_OPENED,
    // Occurs when the door is required to be closed
    DOOR_CLOSE,
    // Occurs when the door has been closed
    DOOR_CLOSED,
    // Occurs when we are required to stop the door opening or closing
    DOOR_HALT,
    // Open 20% of the door
    DOOR_OPEN_BY_20,
    // Close 20% of the door
    DOOR_CLOSE_BY_20,
    // On LCD module show the Door Status
    LCD_SHOW_DOOR_STAT,
    // On LCD module show the RE_VALUE as percentage
    LCD_SHOW_RE_STAT,
    // Occurs when Rotary Encoder Module button is pressed to change the value
    RE_CHANGE,
    // Occurs when Rotary Encoder Module button is pressed to set the value
    RE_SET,
    // Occurs when Rotary Encoder Module is moved in positve side.
    RE_INC,
    // Occurs when Rotary Encoder Module is moved in negative side.
    RE_DEC,
};

// A static GLOBAL variable of an helper class EventBus
static EventBus<Event> events;

/**
 * An Enum tracking the state of an door to an int.
 * Door can only in one of the states mentioned below
 * at one time.
 */
enum DoorStatus {
    opened = 0,
    closed = 1,
    opening = 2,
    closing = 3
};

/**
 * An static GLOBAL variable tracking the state of the door.
 * Will be used by the various event handlers.
 */
static DoorStatus DOOR_STATUS
    = DoorStatus::closed;

/**
 * An Enum tracking the state of a Rotary Encoder to check if
 * it signals are ment to be read or its in a passive state
 * thus ignoring the input from the Signals.
 */
enum REStatus {
    change = 0,
    set = 1
};

/**
 * An static GLOBAL variable tracking the RE button state.
 * Will be used by various event handlers.
 */
static REStatus RE_STATUS = REStatus::set;

/**
 * An static GLOBAL variable tracking the amount of rotated on the Rotart Encoder.
 * Will be used by various event handlers.
 */
static int RE_VALUE = RE_VALUE_MAX;

/**
 * An static GLOBAL variable tracking the angle of the Servo Motor.
 * Will be used by various event handlers.
 */
static int SERVO_ANGLE = 0;

/**
 * The AtSign library client responsible for reading data and storing
 * data on the AtSign secondary server so that Client Application has
 * access to it.
 */
static AtClient* at_client;

/**
 *  Responsibe for reading events that has occured from the
 * client Application.
 */
static AtKey* app_events_key;
/**
 * Used to update the secondary server with random token that the
 * client Application has to read before an event is put in app_events_key.
 */
static AtKey* event_bus_key;
/**
 * Used to update the secondary server with the current status of the door.
 */
static AtKey* door_status_key;
/**
 * Used to update the secondary server with the current value of the Rotary Encoder.
 */
static AtKey* re_value_key;

Servo servo;

/**
 * Initialise the LCD with the pin in a 4-bit mode.
 */
LiquidCrystal lcd(RS, RW, ENABLE, D4, D5, D6, D7);

//-------------- Arduino Interrput Handlers ------------------------------------------//

/**
 * Description: TouchInterruptHandler() will be used as callback for
 * arduino attachInterruptHandler method, with RISING mode to handle
 * the touch event from Capacitve Touch Sensor Module more efficeintly.
 * The function will be adding the events to event bus based on the state
 * of constant DOOR_STATUS.
 * Pre: A static EventBus events and DoorStatus DOOR_STATUS should have been
 * declared and present in GLOBAL.
 * Post:
 * If door is open then event to close the door will be added.
 * If door is closed then event to open the door will be added.
 * If door is opening or closing then event to halt the door will be added.
 */

void TouchInterruptHandler();

/**
 * Description: REButtonHandler() will be used as callback for
 * arduino attachInterruptHandler method, with RISING mode to handle
 * the button press from Rotary Encoder Module more efficeintly.
 * The function will add events to event bus based on the state of
 * constant RE_STATUS.
 * Pre: A static EventBus events and RE_STATUS should have been declared
 * and present in GLOBAL.
 * Post:
 * On Press of the button
 * If RE is in CHANGE mode then RE_STAUS is set to RE_SET.
 * If RE is in SET mode then RE_STATUS is set to RE_CHANGE.
 */
void REButtonHandler();

/**
 * Description: RERotateHandler() will be used as callback for
 * arduino attachInterruptHandler method, with Falling mode to handle
 * the Output from CLK Pin of Rotary Encoder Module more efficeintly.
 * The function will add events to event bus if the Rotary Encoder dial
 * was moved in positive side or negative side.
 * Pre: A static EventBus events, RE_VALUE and RE_STATUS should have been
 * declared and present in GLOBAL.
 * Post:
 * If the Dial on Rotary Encoder was rotated in positive side
 * then RE_INC event will be added in EventBus else RE_DEC event will be
 * added on the event bus.
 */
void RERotateHandler();

//-------------- Event Handlers ------------------------------------------//

/**
 * Description: This function will add all the events in the event bus
 * that needs to performed for the action of door opening.
 * Pre: None
 * Post: Events are added to EventBus
 */
void door_will_open();
/**
 * Description: This function will add all the event in the event bus
 * that will needs to be performed when door is opened and will update
 * the RE_VALUE to RE_MIN to indicate the door has been opened.
 * Pre: None
 * Post: Events are added to EventBus
 */
void door_has_opened();
/**
 * Description: This function will add all the events in th event bus
 * that needs to be performed for the action of for door closing.
 * Pre: None
 * Post: Events are added to EventBus
 */
void door_will_close();
/**
 * Description: This function will add all the events in the event bus
 * that will needs to be performed when door is closed and will update
 * the RE_VALUE to RE_MAX to indicate the door has been closed.
 * Pre: None
 * Post: Events are added to EventBus
 */
void door_has_closed();
/**
 * Description: This function will remove all the events in event bus
 * that are responisble for the movement of door i.e. DOOR_OPEN_BY_20 and
 * DOOR_CLOSE_BY_20 and will add all the event in th event bus
 * that needs to be performed for the action of for door halting.
 * Pre: None
 * Post: Events are added and removed from EventBus
 */
void door_is_halted();

/**
 * Description: This function is responisble for the calling the procedures
 * that will change the servo motor module's angle so the door is opened by 20%.
 * Pre:
 * Post: Servo angle is update to accommodate the door opening byy 20%
 */
void door_open_by_20();
/**
 * Description: This function is responisble for the calling the procedures
 * that will change the servo motor module's angle so the door is closed by 20%.
 * Pre: None
 * Post: Servo angle is update to accommodate the door closing by 20%
 */
void door_close_by_20();

/**
 * Description: This function is responsible to show the message on LCD that
 * displays the current state of the door.
 * Pre:
 * Post: The LCD module will have the DOOR_STATUS displayed in a Formatted message.
 */
void lcd_show_door_stat();
/**
 * Description: This function is responsible to show the message on LCD that
 * displays the current percentage the door is Open when RE_STATUS is set to
 * REStatus:change
 * Pre:
 * Post:
 */
void lcd_show_re_stat();

/**
 * Description: This function is responisble to calling the proceduers that
 * will update the AtSign secondary server with the current status of the Door.
 * Pre:
 * Post: The remote AtSign secondart server is updated with the current status of
 * the door.
 */
void door_sync_status();
/**
 * Description: This function is responisble to calling the proceduers that
 * will update the AtSign secondary server with the current value of the Rotary Encoder.
 * Pre:
 * Post: The remote AtSign secondart server is updated with the current value of RE.
 */
void re_sync_status();

/**
 * Description: This function is responisble to update the GLOBAL variable RE_STATUS to
 * RE_CHANGE and will add event to display the RE_VALUE on LCD.
 * Pre: None
 * Post: The GLOBAL variable is updated, Event is added in EventBus
 */
void re_will_change();
/**
 * Description: This function is responisble to update the GLOBAL variable RE_STATUS to
 * RE_SET and will add event to display the RE_VALUE on LCD.
 * Pre: None
 * Post: The GLOBAL variable is updated, Event is added in EventBus
 */
void re_was_set();
/**
 * Description: This function is responisble to add events that will close the door by 20%
 * and sync the information with AtSign secondary server.
 * Pre: None
 * Post: Will add the events in EventBus neccsary to perform manual closing of door by 20%
 */
void re_value_increased();
/**
 * Description: This function is responisble to add events that will open the door by 20%
 * and sync the information with AtSign secondary server.
 * Pre: None
 * Post: Will add the events in EventBus neccsary to perform manual opening of door by 20%
 */
void re_value_decreased();

/**
 * The array that maps the EventHandler with the numeric value of the
 * enum Event so that is can be called with easily
 */
static void (*(Event_Handlers[15]))() = {
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

//-------------- Arduino Setup Handler ------------------------------------------//

void setup()
{
    // Initialize the AtSign's
    const auto* chip = new AtSign("@moralbearbanana");
    const auto* java = new AtSign("@batmanariesbanh");

    // Read the encryption and decryption keys
    const auto keys = keys_reader::read_keys(*chip);

    at_client = new AtClient(*chip, keys);

    // Wifi connect and pkam authenticate into AtSign secondary Server
    at_client->pkam_authenticate("hotspot", "12345678");

    app_events_key = new AtKey("app_e", java, chip);
    event_bus_key = new AtKey("event_bus", chip, java);
    door_status_key = new AtKey("door_status", chip, java);
    re_value_key = new AtKey("re_value", chip, java);

    // Configure the Arduino Pins and attach the Interrupt Handlers

    pinMode(TOUCH_SENSOR, INPUT);
    attachInterrupt(digitalPinToInterrupt(TOUCH_SENSOR), TouchInterruptHandler, RISING);

    pinMode(RE_BUTTON, INPUT);
    attachInterrupt(digitalPinToInterrupt(RE_BUTTON), REButtonHandler, RISING);

    pinMode(RE_CLK, INPUT);
    pinMode(RE_DAT, INPUT);
    attachInterrupt(digitalPinToInterrupt(RE_CLK), RERotateHandler, FALLING);

    // Start the LCD
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);
    // Start the Servo
    servo.attach(SERVO);

    // Default values on AtSign secondary server
    at_client->put_ak(*event_bus_key, "");
    at_client->put_ak(*door_status_key, to_string(DoorStatus::closed));
    at_client->put_ak(*re_value_key, to_string(RE_VALUE));

    // add event on EventBus to show the default door state
    events.add(Event::LCD_SHOW_DOOR_STAT);
}

// variables used to track Timers.

static volatile unsigned long APP_E_TIME = millis();
static volatile unsigned long TKN_TIME = millis();
static int tkn = -1;

//-------------- Event Handlers ------------------------------------------//

void loop()
{
    if (events.empty()) {

        // At every 30 second interval update the AtSign secondary server with
        // a new random token
        if (millis() - TKN_TIME > 30000) {
            tkn = rand() % 100;
            at_client->put_ak(*event_bus_key, std::to_string(tkn));

            TKN_TIME = millis();
        }

        // At every 15 second interval read the AtSign secondary server
        // to see if an Application event has occured and verify that the
        // token is within the timeframe
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

    // Read the current event that needs to be performed

    Event event = events.current();

    // Invoke the Handler

    Event_Handlers[event]();

    // Mark completed and remove

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

void door_will_open()
{
    std::cout << "DOOR IS OPENING\n";
    DOOR_STATUS = DoorStatus::opening;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    events.add(Event::SYNC_DOOR);

    for (int i = 0; i < RE_VALUE; i++) {
        events.add(Event::DOOR_OPEN_BY_20);
    }
    events.add(Event::DOOR_OPENED);
}

void door_has_opened()
{
    std::cout << "DOOR IS OPENED\n";
    DOOR_STATUS = DoorStatus::opened;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    events.add(Event::SYNC_DOOR);
    RE_VALUE = RE_VALUE_MIN;
    events.add(Event::SYNC_RE);
}

void door_will_close()
{
    std::cout << "DOOR IS CLOSING\n";
    DOOR_STATUS = DoorStatus::closing;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    events.add(Event::SYNC_DOOR);
    for (int i = RE_VALUE; i < RE_VALUE_MAX; i++) {
        events.add(Event::DOOR_CLOSE_BY_20);
    }
    events.add(Event::DOOR_CLOSED);
}

void door_has_closed()
{
    std::cout << "DOOR IS CLOSED\n";
    DOOR_STATUS = DoorStatus::closed;
    events.add(Event::LCD_SHOW_DOOR_STAT);
    events.add(Event::SYNC_DOOR);
    RE_VALUE = RE_VALUE_MAX;
    events.add(Event::SYNC_RE);
}

void door_is_halted()
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
}

void door_sync_status()
{
    at_client->put_ak(*door_status_key, std::to_string(DOOR_STATUS));
}

void re_sync_status()
{
    std::cout << "\n\n\n\nRE_VALUE: " << RE_VALUE << "\n\n\n\n";
    at_client->put_ak(*re_value_key, std::to_string(RE_VALUE));
}

void lcd_show_door_stat()
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
}

void lcd_show_re_stat()
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
}

void door_open_by_20()
{
    SERVO_ANGLE = SERVO_ANGLE + (int)(180 / RE_VALUE_MAX);
    servo.write(SERVO_ANGLE);
    delay(1000);
    RE_VALUE -= 1;
}

void door_close_by_20()
{
    SERVO_ANGLE = SERVO_ANGLE - (int)(180 / RE_VALUE_MAX);
    servo.write(SERVO_ANGLE);
    delay(1000);
    RE_VALUE += 1;
}

void re_will_change()
{
    RE_STATUS = REStatus::change;
    events.add(Event::LCD_SHOW_RE_STAT);
}

void re_was_set()
{
    RE_STATUS = REStatus::set;
    events.add(Event::LCD_SHOW_DOOR_STAT);
}

void re_value_increased()
{
    if (RE_VALUE < RE_VALUE_MAX) {
        events.add(Event::DOOR_CLOSE_BY_20);
        events.add(Event::LCD_SHOW_RE_STAT);
        events.add(Event::SYNC_RE);
    }
}

void re_value_decreased()
{
    if (RE_VALUE > RE_VALUE_MIN) {
        events.add(Event::DOOR_OPEN_BY_20);
        events.add(Event::LCD_SHOW_RE_STAT);
        events.add(Event::SYNC_RE);
    }
}