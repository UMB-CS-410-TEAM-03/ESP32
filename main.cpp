#include "at_client.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// #include "constants.h"
// #define LED 21

void setup()
{
    const auto* chip = new AtSign("@moralbearbanana");
    const auto* java = new AtSign("@batmanariesbanh");

    // // reads the keys on the chip
    const auto keys = keys_reader::read_keys(*chip);

    // // creates the AtClient object (allows us to run operations)
    auto* at_client = new AtClient(*chip, keys);

    // // pkam authenticate into our atServer
    at_client->pkam_authenticate("hotspot", "12345678");

    const auto* at_key = new AtKey("test", chip, java);
    const auto value = std::string("Hello World");

    at_client->put_ak(*at_key, value);
}

void loop()
{
    // put your main code here, to run repeatedly:
    // digitalWrite(LED, HIGH);
    // delay(500);
    // digitalWrite(LED, LOW);
    // delay(500);
}